#include <iiGraphicAdjust/CorrectionExtensions.h>
#include "FilterInternal.h"

namespace iiGraphicAdjust {
namespace {
Error invalid(const char *text) { return {ErrorCode::InvalidParameter,text}; }
}
Result<Image> colorToAlpha(ImageView image,const ColorToAlphaOptions &o) {
    if (auto c=detail::validate(image); !c) return std::unexpected(c.error());
    for (const auto c:o.color) if (!detail::inRange(c,0,1)) return std::unexpected(invalid("Invalid key color"));
    if (!detail::inRange(o.transparencyThreshold,0,1) || !detail::inRange(o.opacityThreshold,0,1))
        return std::unexpected(invalid("Invalid color-to-alpha thresholds"));
    return detail::allocated([&] {
        auto out=detail::copy(image);
        for (auto &p:out.pixels) {
            if (p.a==0) continue;
            const std::array<double,3> channels{p.r,p.g,p.b};
            double opacity=0;
            for (std::size_t k=0;k<3;++k) {
                const auto c=o.color[k],v=channels[k];
                if (v>c) opacity=std::max(opacity,(v-c)/(1-c));
                else if (v<c) opacity=std::max(opacity,(c-v)/c);
            }
            const auto a=opacity<=o.transparencyThreshold?0.:
                         opacity>=o.opacityThreshold?1.:
                         (opacity-o.transparencyThreshold)/(o.opacityThreshold-o.transparencyThreshold);
            if (a==1) continue;
            std::array<float,3> color{};
            for (std::size_t k=0;k<3;++k)
                color[k]=static_cast<float>(a==0?o.color[k]:detail::clamp(o.color[k]+(channels[k]-o.color[k])/a));
            p={color[0],color[1],color[2],static_cast<float>(p.a*a)};
        }
        return out;
    });
}
Result<Image> flatFieldCorrection(ImageView image,ImageView reference,const FlatFieldOptions &o) {
    if (auto c=detail::validate(image); !c) return std::unexpected(c.error());
    if (auto c=detail::validate(reference); !c) return std::unexpected(c.error());
    if (image.width!=reference.width || image.height!=reference.height)
        return std::unexpected(Error{ErrorCode::SizeMismatch,"Flat reference must match source dimensions"});
    if (!detail::inRange(o.amount,0,1) || !detail::inRange(o.minimum,1e-8,1))
        return std::unexpected(invalid("Invalid flat-field settings"));
    std::array<double,3> mean{}; double weight=0;
    for (std::size_t i=0;i<reference.pixels.size();++i) {
        const auto p=reference.pixels[i];
        if (image.pixels[i].a>0 && p.a==0)
            return std::unexpected(Error{ErrorCode::InvalidPixel,"Flat reference has holes over visible source pixels"});
        mean[0]+=detail::linear(p.r)*p.a; mean[1]+=detail::linear(p.g)*p.a;
        mean[2]+=detail::linear(p.b)*p.a; weight+=p.a;
    }
    if (weight==0) return std::unexpected(invalid("Flat reference contains no visible samples"));
    for (auto &v:mean) v/=weight;
    const auto meanLuma=.2126*mean[0]+.7152*mean[1]+.0722*mean[2];
    return detail::allocated([&] {
        auto out=detail::copy(image);
        if (o.amount==0) return out;
        for (std::size_t i=0;i<out.pixels.size();++i) {
            auto &p=out.pixels[i]; if (p.a==0) continue;
            const auto f=reference.pixels[i];
            const std::array<double,3> flat{detail::linear(f.r),detail::linear(f.g),detail::linear(f.b)};
            const auto flatLuma=.2126*flat[0]+.7152*flat[1]+.0722*flat[2];
            const auto apply=[&](float v,std::size_t k) {
                const auto gain=o.perChannel?mean[k]/std::max(o.minimum,flat[k]):
                                             meanLuma/std::max(o.minimum,flatLuma);
                if (gain==1) return v;
                return detail::encoded(detail::linear(v)*(1+o.amount*(gain-1)));
            };
            p={apply(p.r,0),apply(p.g,1),apply(p.b,2),p.a};
        }
        return out;
    });
}
Result<Image> solidFill(ImageView image,const SolidFillOptions &o) {
    if (auto c=detail::validate(image); !c) return std::unexpected(c.error());
    if (!detail::inRange(o.color.r,0,1) || !detail::inRange(o.color.g,0,1) ||
        !detail::inRange(o.color.b,0,1) || !detail::inRange(o.color.a,0,1))
        return std::unexpected(invalid("Fill color must be finite and normalized"));
    return detail::allocated([&] {
        auto out=detail::output(image);
        for (std::size_t i=0;i<out.pixels.size();++i) {
            out.pixels[i]=o.color;
            if (o.preserveAlpha) out.pixels[i].a*=image.pixels[i].a;
        }
        return out;
    });
}
Result<Image> gradientFill(ImageView image,const GradientFillOptions &o) {
    if (auto c=detail::validate(image); !c) return std::unexpected(c.error());
    if (o.stops.size()<2 || o.stops.size()>65536 || !detail::inRange(o.centerX,-2,3) ||
        !detail::inRange(o.centerY,-2,3) || !detail::inRange(o.angle,-360,360) ||
        !detail::inRange(o.scale,.001,100) ||
        (o.space!=CorrectionSpace::Srgb && o.space!=CorrectionSpace::LinearSrgb) ||
        o.style<GradientFillStyle::Linear || o.style>GradientFillStyle::Diamond)
        return std::unexpected(invalid("Invalid gradient fill geometry or interpolation"));
    double previous=-1;
    for (const auto &stop:o.stops) {
        if (!detail::inRange(stop.position,0,1) || stop.position<=previous)
            return std::unexpected(invalid("Gradient stops must be normalized and strictly ordered"));
        previous=stop.position;
        for (const auto c:stop.color) if (!detail::inRange(c,0,1))
            return std::unexpected(invalid("Invalid gradient stop color"));
    }
    return detail::allocated([&] {
        auto out=detail::output(image);
        const auto radians=o.angle*std::numbers::pi/180,cs=std::cos(radians),sn=std::sin(radians),
                   extent=double(std::max(image.width,image.height))*o.scale;
        for (std::size_t y=0;y<image.height;++y) for (std::size_t x=0;x<image.width;++x) {
            const auto dx=(x+.5-o.centerX*image.width)/extent,
                       dy=(y+.5-o.centerY*image.height)/extent,
                       u=cs*dx+sn*dy,v=-sn*dx+cs*dy;
            double t=0;
            switch(o.style) {
                case GradientFillStyle::Linear:t=u+.5;break;
                case GradientFillStyle::Radial:t=2*std::hypot(u,v);break;
                case GradientFillStyle::Angle:t=std::atan2(v,u)/(2*std::numbers::pi);t-=std::floor(t);break;
                case GradientFillStyle::Reflected:t=2*std::abs(u);break;
                case GradientFillStyle::Diamond:t=2*(std::abs(u)+std::abs(v));break;
            }
            t=detail::clamp(t); if(o.reverse) t=1-t;
            auto right=std::upper_bound(o.stops.begin(),o.stops.end(),t,
                [](double value,const GradientStop &s){return value<s.position;});
            const auto &b=right==o.stops.end()?o.stops.back():*right;
            const auto &a=right==o.stops.begin()?o.stops.front():*(right-1);
            const auto fraction=a.position==b.position?0:detail::clamp((t-a.position)/(b.position-a.position));
            std::array<float,3> color{};
            for (std::size_t k=0;k<3;++k) {
                if (fraction==0 || a.color[k]==b.color[k]) color[k]=static_cast<float>(a.color[k]);
                else if (fraction==1) color[k]=static_cast<float>(b.color[k]);
                else if(o.space==CorrectionSpace::LinearSrgb)
                    color[k]=detail::encoded((1-fraction)*detail::linear(a.color[k])+fraction*detail::linear(b.color[k]));
                else color[k]=static_cast<float>((1-fraction)*a.color[k]+fraction*b.color[k]);
            }
            const auto i=y*image.width+x;
            out.pixels[i]={color[0],color[1],color[2],o.preserveAlpha?image.pixels[i].a:1.F};
        }
        return out;
    });
}
Result<Image> patternFill(ImageView image,ImageView pattern,const PatternFillOptions &o) {
    if (auto c=detail::validate(image); !c) return std::unexpected(c.error());
    if (auto c=detail::validate(pattern); !c) return std::unexpected(c.error());
    if (!detail::inRange(o.scale,.01,100) || !detail::inRange(o.angle,-360,360) ||
        !detail::inRange(o.offsetX,-1e9,1e9) || !detail::inRange(o.offsetY,-1e9,1e9))
        return std::unexpected(invalid("Invalid pattern fill transform"));
    return detail::allocated([&] {
        auto out=detail::output(image);
        const auto angle=o.angle*std::numbers::pi/180,cs=std::cos(angle),sn=std::sin(angle);
        for (std::size_t y=0;y<image.height;++y) for (std::size_t x=0;x<image.width;++x) {
            const auto dx=x+.5-o.offsetX,dy=y+.5-o.offsetY;
            auto p=filter_detail::straight(filter_detail::sample(pattern,(cs*dx+sn*dy)/o.scale,
                                      (-sn*dx+cs*dy)/o.scale,filter_detail::Edge::Wrap));
            const auto i=y*image.width+x;
            if(o.preserveAlpha) p.a*=image.pixels[i].a;
            out.pixels[i]=p;
        }
        return out;
    });
}
} // namespace iiGraphicAdjust
