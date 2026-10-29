#include <iiGraphicAdjust/Blend.h>
#include "Internal.h"

namespace iiGraphicAdjust {
namespace {
bool validMode(BlendMode mode) noexcept {
    switch(mode) {
        case BlendMode::Normal: case BlendMode::Multiply: case BlendMode::Screen:
        case BlendMode::Overlay: case BlendMode::Darken: case BlendMode::Lighten:
        case BlendMode::SoftLight: case BlendMode::Difference: return true;
    }
    return false;
}
double mix(double b,double s,BlendMode mode) noexcept {
    switch(mode) {
        case BlendMode::Normal: return s;
        case BlendMode::Multiply: return b*s;
        case BlendMode::Screen: return b+s-b*s;
        case BlendMode::Overlay: return b<=.5 ? 2*b*s : 1-2*(1-b)*(1-s);
        case BlendMode::Darken: return std::min(b,s);
        case BlendMode::Lighten: return std::max(b,s);
        case BlendMode::Difference: return std::abs(b-s);
        case BlendMode::SoftLight:
            if (s<=.5) return b-(1-2*s)*b*(1-b);
            return b+(2*s-1)*((b<=.25 ? ((16*b-12)*b+4)*b : std::sqrt(b))-b);
    }
    return 0; // Unreachable after public validation.
}
} // namespace
Result<Image> blend(ImageView backdrop,ImageView source,const BlendOptions& options) {
    auto valid=detail::validate(backdrop);
    if (!valid) return std::unexpected(valid.error());
    valid=detail::validate(source);
    if (!valid) return std::unexpected(valid.error());
    if (backdrop.width!=source.width || backdrop.height!=source.height)
        return std::unexpected(Error{ErrorCode::SizeMismatch,"Blend images must have identical dimensions"});
    if (!validMode(options.mode) || !detail::inRange(options.opacity,0,1) ||
        (options.space!=BlendSpace::Srgb && options.space!=BlendSpace::LinearSrgb))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Invalid blend mode, color space or opacity"});
    return detail::allocated([&] {
        auto result=detail::output(backdrop);
        for (std::size_t i=0;i<result.pixels.size();++i) {
            const auto b=backdrop.pixels[i],s=source.pixels[i];
            const double sa=s.a*options.opacity,ba=b.a,alpha=sa+ba*(1-sa);
            if (sa==0) { result.pixels[i]=b; continue; }
            const auto channel=[&](double bc,double sc) {
                if (options.space==BlendSpace::LinearSrgb) { bc=detail::linear(bc); sc=detail::linear(sc); }
                // W3C blend then Porter-Duff source-over, in premultiplied form.
                const double value=(sa*(1-ba)*sc+sa*ba*mix(bc,sc,options.mode)+(1-sa)*ba*bc)/alpha;
                return options.space==BlendSpace::LinearSrgb ? detail::encoded(value) : float(detail::clamp(value));
            };
            result.pixels[i]={channel(b.r,s.r),channel(b.g,s.g),channel(b.b,s.b),float(alpha)};
        }
        return result;
    });
}
} // namespace iiGraphicAdjust
