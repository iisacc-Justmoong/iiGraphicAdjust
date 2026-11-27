#include <iiGraphicAdjust/LayerCompositing.h>
#include "Internal.h"
#include <climits>

namespace iiGraphicAdjust::objects {
namespace {
Result<void> matchingMask(ImageView image,MaskView mask) {
    const auto count=detail::pixelCount(mask.width,mask.height);
    if (!count) return std::unexpected(count.error());
    if (mask.width!=image.width || mask.height!=image.height)
        return std::unexpected(Error{ErrorCode::SizeMismatch,"Mask dimensions must match image"});
    if (mask.coverage.size()!=*count)
        return std::unexpected(Error{ErrorCode::InvalidBuffer,"Mask buffer must match dimensions"});
    for (float v:mask.coverage)
        if (!detail::inRange(v,0,1))
            return std::unexpected(Error{ErrorCode::InvalidParameter,"Mask coverage must be normalized"});
    return {};
}
Result<void> matchingImages(ImageView a,ImageView b) {
    if (auto valid=detail::validate(a);!valid) return valid;
    if (auto valid=detail::validate(b);!valid) return valid;
    if (a.width!=b.width || a.height!=b.height)
        return std::unexpected(Error{ErrorCode::SizeMismatch,"Image dimensions must match"});
    return {};
}
Rgba interpolate(Rgba a,Rgba b,double t) {
    if (t==0) return a;
    if (t==1) return b;
    const double alpha=a.a*(1-t)+b.a*t;
    if (alpha==0) return {0,0,0,0};
    auto mix=[&](float x,float y) {
        return static_cast<float>((x*a.a*(1-t)+y*b.a*t)/alpha);
    };
    return {mix(a.r,b.r),mix(a.g,b.g),mix(a.b,b.b),static_cast<float>(alpha)};
}
} // namespace

Result<LayerTranslation> LayerRegistrationSolver::execute(ImageView reference,ImageView moving) const {
    if (auto valid=matchingImages(reference,moving);!valid) return std::unexpected(valid.error());
    if (maxShiftX>1024 || maxShiftY>1024 || !detail::inRange(minOverlap,0,1) || minOverlap==0 ||
        reference.width>INT_MAX || reference.height>INT_MAX)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Invalid registration search bounds"});
    const int w=int(reference.width),h=int(reference.height);
    const int sx=std::min<int>(maxShiftX,w-1),sy=std::min<int>(maxShiftY,h-1);
    bool found=false; LayerTranslation best;
    double bestError=std::numeric_limits<double>::infinity();
    for (int dy=-sy;dy<=sy;++dy) for (int dx=-sx;dx<=sx;++dx) {
        double error=0,weight=0; std::size_t visible=0;
        const int x0=std::max(0,dx),x1=w+std::min(0,dx);
        const int y0=std::max(0,dy),y1=h+std::min(0,dy);
        for (int y=y0;y<y1;++y) for (int x=x0;x<x1;++x) {
            const auto a=reference.pixels[std::size_t(y)*reference.width+std::size_t(x)];
            const auto b=moving.pixels[std::size_t(y-dy)*moving.width+std::size_t(x-dx)];
            const double alpha=double(a.a)*b.a;
            if (alpha==0) continue;
            const double r=double(a.r)-b.r,g=double(a.g)-b.g,bl=double(a.b)-b.b;
            error+=alpha*(r*r+g*g+bl*bl); weight+=alpha; ++visible;
        }
        if (!weight || double(visible)<double(reference.pixels.size())*minOverlap) continue;
        error/=weight;
        const auto distance=dx*dx+dy*dy,bestDistance=best.dx*best.dx+best.dy*best.dy;
        if (!found || error<bestError || (error==bestError && distance<bestDistance)) {
            found=true; bestError=error; best={dx,dy,error};
        }
    }
    if (!found) return std::unexpected(Error{ErrorCode::InvalidParameter,"No translation has enough visible overlap"});
    return best;
}

Result<Image> LayerStackBlendSolver::execute(std::span<const LayerView> layers) const {
    if (layers.empty() || layers.size()>256)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Layer stack requires one to 256 layers"});
    if (auto valid=detail::validate(layers.front().image);!valid) return std::unexpected(valid.error());
    auto guarded=detail::allocated([&]() -> Result<Image> {
        const auto first=layers.front().image;
        Image current{first.width,first.height,std::vector<Rgba>(first.pixels.size(),{0,0,0,0})};
        for (const auto& layer:layers) {
            auto next=blend(current.view(),layer.image,layer.blend);
            if (!next) return std::unexpected(next.error());
            current=std::move(*next);
        }
        return current;
    });
    if (!guarded) return std::unexpected(guarded.error());
    return std::move(*guarded);
}

Result<Image> SkyReplacementCompositor::execute(ImageView foreground,ImageView sky,MaskView mask) const {
    if (auto valid=matchingImages(foreground,sky);!valid) return std::unexpected(valid.error());
    if (auto valid=matchingMask(foreground,mask);!valid) return std::unexpected(valid.error());
    return detail::allocated([&] {
        auto out=detail::copy(foreground);
        for (std::size_t i=0;i<out.pixels.size();++i)
            out.pixels[i]=interpolate(foreground.pixels[i],sky.pixels[i],mask.coverage[i]);
        return out;
    });
}

Result<Mask> SubjectEdgeMatteRefiner::execute(ImageView image,MaskView mask) const {
    return refineMask(image,mask,amount);
}

Result<Image> ColorDecontaminationPass::execute(ImageView observed,ImageView background,MaskView mask) const {
    if (auto valid=matchingImages(observed,background);!valid) return std::unexpected(valid.error());
    if (auto valid=matchingMask(observed,mask);!valid) return std::unexpected(valid.error());
    if (!detail::inRange(amount,0,1) || !detail::inRange(minimumCoverage,0,1) || minimumCoverage==0)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Invalid decontamination settings"});
    return detail::allocated([&] {
        auto out=detail::copy(observed);
        for (std::size_t i=0;i<out.pixels.size();++i) {
            const auto coverage=double(mask.coverage[i]);
            if (coverage<=0 || coverage>=1 || amount==0 || observed.pixels[i].a==0) continue;
            const auto a=observed.pixels[i],b=background.pixels[i];
            const auto alpha=std::max(coverage,minimumCoverage);
            auto unmix=[&](float pixel,float bg) {
                const auto current=detail::linear(pixel);
                const auto corrected=detail::clamp((current-(1-alpha)*detail::linear(bg))/alpha);
                return detail::encoded(current+(corrected-current)*amount);
            };
            out.pixels[i]={unmix(a.r,b.r),unmix(a.g,b.g),unmix(a.b,b.b),a.a};
        }
        return out;
    });
}
} // namespace iiGraphicAdjust::objects
