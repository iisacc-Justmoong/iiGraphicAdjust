#include <iiGraphicAdjust/Color.h>
#include "Internal.h"

namespace iiGraphicAdjust {
namespace {
using Triple = std::array<double,3>;
struct Hsl final { double h, s, l; };
constexpr Error invalid{ErrorCode::InvalidParameter,"Color settings are outside documented ranges"};
double hueWrap(double h) { h=std::fmod(h,360.); return h<0 ? h+360 : h; }
double smooth(double x) { x=detail::clamp(x); return x*x*(3-2*x); }
double luma(const Triple& c) { return .2126*c[0]+.7152*c[1]+.0722*c[2]; }
Triple rgb(Rgba p) { return {p.r,p.g,p.b}; }
void store(Rgba& p,const Triple& c) {
    p.r=static_cast<float>(detail::clamp(c[0]));
    p.g=static_cast<float>(detail::clamp(c[1]));
    p.b=static_cast<float>(detail::clamp(c[2]));
}
Hsl hsl(const Triple& c) {
    const auto [lo,hi]=std::minmax_element(c.begin(),c.end());
    const double delta=*hi-*lo,l=(*hi+*lo)/2;
    if (delta==0) return {0,0,l};
    double h=0;
    if (*hi==c[0]) h=60*((c[1]-c[2])/delta);
    else if (*hi==c[1]) h=60*((c[2]-c[0])/delta+2);
    else h=60*((c[0]-c[1])/delta+4);
    return {hueWrap(h),delta/(l<=.5 ? *hi+*lo : 2-*hi-*lo),l};
}
Triple fromHsl(Hsl c) {
    // Equivalent HSL expression without subtracting almost-one values when
    // lightness is tiny: retain chroma down to float subnormal inputs.
    const double chroma=(c.l<=.5 ? 2*c.l : 2*(1-c.l))*c.s;
    const double sector=hueWrap(c.h)/60,x=chroma*(1-std::abs(std::fmod(sector,2.)-1));
    Triple result{};
    if (sector<1) result={chroma,x,0}; else if (sector<2) result={x,chroma,0};
    else if (sector<3) result={0,chroma,x}; else if (sector<4) result={0,x,chroma};
    else if (sector<5) result={x,0,chroma}; else result={chroma,0,x};
    for (auto& v:result) v+=c.l-chroma/2;
    return result;
}
double offset(double v,double amount) { return v+amount*(amount>=0 ? 1-v : v); }
Triple preserveLuma(Triple c,double target) {
    const double current=luma(c); double scale=1;
    for (auto v:c) {
        const double d=v-current;
        if (d>0) scale=std::min(scale,(1-target)/d);
        else if (d<0) scale=std::min(scale,-target/d);
    }
    for (auto& v:c) v=target+(v-current)*scale;
    return c;
}
template<class F> Result<Image> transform(ImageView image,F&& operation) {
    return detail::allocated([&] {
        auto result=detail::copy(image);
        for (auto& p:result.pixels) if (p.a!=0) operation(p);
        return result;
    });
}
bool tripletRange(const Triple& c,double lo,double hi) {
    return std::all_of(c.begin(),c.end(),[&](double v){return detail::inRange(v,lo,hi);});
}
bool hueRangeValid(HueRange range) { return range>=HueRange::Master && range<=HueRange::Magentas; }
double hueWeight(Hsl c,HueRange range,double falloff) {
    if (range==HueRange::Master) return 1;
    if (c.s==0) return 0;
    const double center=(static_cast<int>(range)-1)*60.;
    const double distance=std::abs(std::remainder(c.h-center,360.));
    if (distance<=30) return 1;
    if (falloff==0 || distance>=30+falloff) return 0;
    return 1-smooth((distance-30)/falloff);
}
Triple interpolate(const Triple& a,const Triple& b,double weight) {
    Triple c{}; for (std::size_t i=0;i<3;++i) c[i]=a[i]+weight*(b[i]-a[i]); return c;
}
bool balanceValid(const ToneBalance& b) {
    return detail::inRange(b.cyanRed,-1,1) && detail::inRange(b.magentaGreen,-1,1) &&
           detail::inRange(b.yellowBlue,-1,1);
}
bool mixValid(const ChannelMix& mix) {
    return tripletRange(mix.coefficients,-2,2) && detail::inRange(mix.constant,-1,1);
}
double mixChannel(const Triple& c,const ChannelMix& mix) {
    return c[0]*mix.coefficients[0]+c[1]*mix.coefficients[1]+c[2]*mix.coefficients[2]+mix.constant;
}
std::array<double,9> rangeWeights(const Triple& c) {
    const double lo=*std::min_element(c.begin(),c.end()),hi=*std::max_element(c.begin(),c.end());
    const double chroma=hi-lo,lightness=(hi+lo)/2;
    return {std::max(0.,c[0]-std::max(c[1],c[2])),std::max(0.,std::min(c[0],c[1])-c[2]),
            std::max(0.,c[1]-std::max(c[0],c[2])),std::max(0.,std::min(c[1],c[2])-c[0]),
            std::max(0.,c[2]-std::max(c[0],c[1])),std::max(0.,std::min(c[0],c[2])-c[1]),
            std::max(0.,2*lo-1),(1-chroma)*(1-std::abs(2*lightness-1)),std::max(0.,1-2*hi)};
}
struct Statistics final { Triple mean{},deviation{}; };
Result<Statistics> statistics(ImageView image) {
    Statistics s; double weight=0;
    for (const auto p:image.pixels) {
        const double alpha=p.a; if (alpha==0) continue;
        weight+=alpha;
        const Triple c{detail::linear(p.r),detail::linear(p.g),detail::linear(p.b)};
        for (std::size_t j=0;j<3;++j) s.mean[j]+=alpha*c[j];
    }
    if (weight==0) return std::unexpected(Error{ErrorCode::InvalidParameter,
                                              "Match Color requires visible pixels in both images"});
    for (auto& v:s.mean) v/=weight;
    for (const auto p:image.pixels) {
        if (p.a==0) continue;
        const Triple c{detail::linear(p.r),detail::linear(p.g),detail::linear(p.b)};
        for (std::size_t j=0;j<3;++j) {
            const double d=c[j]-s.mean[j]; s.deviation[j]+=p.a*d*d;
        }
    }
    for (auto& v:s.deviation) v=std::sqrt(v/weight);
    return s;
}
} // namespace

Result<Image> hueSaturation(ImageView image,const HueSaturationOptions& o) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(o.hue,-180,180) || !detail::inRange(o.saturation,o.colorize?0:-1,1) ||
        !detail::inRange(o.lightness,-1,1) || !hueRangeValid(o.range) || !detail::inRange(o.falloff,0,60))
        return std::unexpected(invalid);
    return transform(image,[&](Rgba& p) {
        if (!o.colorize && o.hue==0 && o.saturation==0 && o.lightness==0) return;
        const auto original=rgb(p); auto c=hsl(original); const double w=hueWeight(c,o.range,o.falloff);
        if (w==0) return;
        c.h=o.colorize ? hueWrap(o.hue) : hueWrap(c.h+o.hue);
        c.s=o.colorize ? o.saturation : (c.s==0 ? 0 : offset(c.s,o.saturation));
        c.l=offset(c.l,o.lightness);
        store(p,interpolate(original,fromHsl(c),w));
    });
}
Result<Image> vibrance(ImageView image,const VibranceOptions& o) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(o.vibrance,-1,1) || !detail::inRange(o.saturation,-1,1)) return std::unexpected(invalid);
    return transform(image,[&](Rgba& p) {
        if (o.vibrance==0 && o.saturation==0) return;
        auto c=hsl(rgb(p)); if (c.s==0) return;
        c.s=detail::clamp(c.s+o.vibrance*c.s*(1-c.s)); c.s=offset(c.s,o.saturation);
        store(p,fromHsl(c));
    });
}
Result<Image> colorBalance(ImageView image,const ColorBalanceOptions& o) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    if (!balanceValid(o.shadows) || !balanceValid(o.midtones) || !balanceValid(o.highlights))
        return std::unexpected(invalid);
    return transform(image,[&](Rgba& p) {
        const auto nonzero=[](const ToneBalance& b) { return b.cyanRed!=0 || b.magentaGreen!=0 || b.yellowBlue!=0; };
        if (!nonzero(o.shadows) && !nonzero(o.midtones) && !nonzero(o.highlights)) return;
        auto c=rgb(p); const double y=luma(c),ws=(1-y)*(1-y),wm=2*y*(1-y),wh=y*y;
        c[0]+=.5*(ws*o.shadows.cyanRed+wm*o.midtones.cyanRed+wh*o.highlights.cyanRed);
        c[1]+=.5*(ws*o.shadows.magentaGreen+wm*o.midtones.magentaGreen+wh*o.highlights.magentaGreen);
        c[2]+=.5*(ws*o.shadows.yellowBlue+wm*o.midtones.yellowBlue+wh*o.highlights.yellowBlue);
        if (o.preserveLuminosity) c=preserveLuma(c,y);
        store(p,c);
    });
}
Result<Image> blackAndWhite(ImageView image,const BlackWhiteOptions& o) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    if (!std::all_of(o.weights.begin(),o.weights.end(),[](double v){return detail::inRange(v,-2,3);}) ||
        !detail::inRange(o.tintHue,0,360) || !detail::inRange(o.tintSaturation,0,1)) return std::unexpected(invalid);
    return transform(image,[&](Rgba& p) {
        const auto c=rgb(p); const auto h=hsl(c); const double sector=h.h/60;
        const auto i=static_cast<std::size_t>(sector)%6; const double f=sector-std::floor(sector);
        const double coefficient=o.weights[i]*(1-f)+o.weights[(i+1)%6]*f;
        const double lo=*std::min_element(c.begin(),c.end()),hi=*std::max_element(c.begin(),c.end());
        const double gray=detail::clamp(lo+(hi-lo)*coefficient);
        store(p,o.tint ? fromHsl({o.tintHue,o.tintSaturation,gray}) : Triple{gray,gray,gray});
    });
}
Result<Image> photoFilter(ImageView image,const PhotoFilterOptions& o) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    if (!tripletRange(o.color,0,1) || !detail::inRange(o.density,0,1)) return std::unexpected(invalid);
    return transform(image,[&](Rgba& p) {
        if (o.density==0) return;
        auto c=rgb(p); const double y=luma(c);
        for (std::size_t j=0;j<3;++j) c[j]*=1-o.density+o.density*o.color[j];
        if (o.preserveLuminosity) c=preserveLuma(c,y);
        store(p,c);
    });
}
Result<Image> channelMixer(ImageView image,const ChannelMixerOptions& o) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    if (!mixValid(o.red) || !mixValid(o.green) || !mixValid(o.blue) || !mixValid(o.gray)) return std::unexpected(invalid);
    return transform(image,[&](Rgba& p) {
        const auto c=rgb(p);
        if (o.monochrome) { const double v=mixChannel(c,o.gray); store(p,{v,v,v}); }
        else store(p,{mixChannel(c,o.red),mixChannel(c,o.green),mixChannel(c,o.blue)});
    });
}
Result<Image> selectiveColor(ImageView image,const SelectiveColorOptions& o) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    if (o.method!=SelectiveColorMethod::Relative && o.method!=SelectiveColorMethod::Absolute)
        return std::unexpected(invalid);
    bool nonzero=false;
    for (const auto& r:o.ranges) {
        if (!detail::inRange(r.cyan,-1,1) || !detail::inRange(r.magenta,-1,1) ||
            !detail::inRange(r.yellow,-1,1) || !detail::inRange(r.black,-1,1)) return std::unexpected(invalid);
        nonzero|=r.cyan!=0 || r.magenta!=0 || r.yellow!=0 || r.black!=0;
    }
    return transform(image,[&](Rgba& p) {
        if (!nonzero) return;
        const auto c=rgb(p); const auto weights=rangeWeights(c); Triple ink{}; double black=0;
        for (std::size_t i=0;i<9;++i) {
            ink[0]+=weights[i]*o.ranges[i].cyan; ink[1]+=weights[i]*o.ranges[i].magenta;
            ink[2]+=weights[i]*o.ranges[i].yellow; black+=weights[i]*o.ranges[i].black;
        }
        Triple out{};
        for (std::size_t j=0;j<3;++j) {
            const double inkScale=o.method==SelectiveColorMethod::Relative ? 1-c[j] : 1;
            const double keyScale=o.method==SelectiveColorMethod::Relative ? c[j] : 1;
            out[j]=c[j]-ink[j]*inkScale-black*keyScale;
        }
        store(p,out);
    });
}
Result<Image> desaturate(ImageView image) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    return transform(image,[&](Rgba& p) { const double l=hsl(rgb(p)).l; store(p,{l,l,l}); });
}
Result<Image> replaceColor(ImageView image,const ReplaceColorOptions& o) {
    const auto valid=detail::validate(image); if (!valid) return std::unexpected(valid.error());
    if (!tripletRange(o.target,0,1) || !detail::inRange(o.fuzziness,0,1) ||
        !detail::inRange(o.softness,0,1) || !detail::inRange(o.hue,-180,180) ||
        !detail::inRange(o.saturation,-1,1) || !detail::inRange(o.lightness,-1,1)) return std::unexpected(invalid);
    return transform(image,[&](Rgba& p) {
        if (o.hue==0 && o.saturation==0 && o.lightness==0) return;
        const auto original=rgb(p); double distance=0;
        for (std::size_t i=0;i<3;++i) distance+=(original[i]-o.target[i])*(original[i]-o.target[i]);
        distance=std::sqrt(distance/3);
        double weight=0;
        if (o.fuzziness==0) weight=distance<=1e-12 ? 1 : 0;
        else if (distance<o.fuzziness) {
            const double core=o.fuzziness*(1-o.softness);
            weight=distance<=core ? 1 : 1-smooth((distance-core)/(o.fuzziness-core));
        }
        if (weight==0) return;
        auto c=hsl(original); c.h=hueWrap(c.h+o.hue); c.s=c.s==0 ? 0 : offset(c.s,o.saturation); c.l=offset(c.l,o.lightness);
        store(p,interpolate(original,fromHsl(c),weight));
    });
}
Result<Image> matchColor(ImageView source,ImageView reference,const MatchColorOptions& o) {
    const auto valid=detail::validate(source); if (!valid) return std::unexpected(valid.error());
    const auto referenceValid=detail::validate(reference); if (!referenceValid) return std::unexpected(referenceValid.error());
    if (!detail::inRange(o.luminance,0,2) || !detail::inRange(o.colorIntensity,0,2) || !detail::inRange(o.fade,0,1))
        return std::unexpected(invalid);
    const auto from=statistics(source),to=statistics(reference);
    if (!from) return std::unexpected(from.error()); if (!to) return std::unexpected(to.error());
    return transform(source,[&](Rgba& p) {
        if (o.fade==1) return;
        const auto original=rgb(p); Triple c{};
        const double targetGray=luma(to->mean);
        for (std::size_t j=0;j<3;++j) {
            const double ratio=from->deviation[j]>1e-12 ? to->deviation[j]/from->deviation[j] : 1;
            const double target=o.neutralize ? targetGray : to->mean[j];
            c[j]=target+(detail::linear(original[j])-from->mean[j])*ratio;
        }
        const double y=luma(c);
        for (auto& v:c) v=detail::encoded(y*o.luminance+(v-y)*o.colorIntensity);
        store(p,interpolate(c,original,o.fade));
    });
}
} // namespace iiGraphicAdjust
