#include <iiGraphicAdjust/Hdr.h>
#include "Internal.h"

namespace iiGraphicAdjust {
namespace {
Result<void> settings(const HdrToningOptions& options) {
    if (!detail::inRange(options.exposure,-16,16) || !detail::inRange(options.gamma,.01,100) ||
        (options.method!=HdrToningMethod::ExposureGamma && options.method!=HdrToningMethod::HighlightCompression))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Unsupported HDR method or invalid exposure/gamma"});
    return {};
}
Rgba mapped(double r,double g,double b,float alpha,const HdrToningOptions& options) {
    const double multiplier=std::exp2(options.exposure);
    r*=multiplier; g*=multiplier; b*=multiplier;
    if (options.method==HdrToningMethod::HighlightCompression) {
        const double luminance=.2126*r+.7152*g+.0722*b;
        const double scale=1/(1+luminance);
        r*=scale; g*=scale; b*=scale;
    }
    // Clamp before pow: normalized output clipping commutes with a positive power,
    // and avoids overflowing pow on huge HDR input with small gamma.
    return {detail::encoded(std::pow(detail::clamp(r),1/options.gamma)),
            detail::encoded(std::pow(detail::clamp(g),1/options.gamma)),
            detail::encoded(std::pow(detail::clamp(b),1/options.gamma)),alpha};
}
} // namespace
Result<Image> hdrToning(HdrImageView image,const HdrToningOptions& options) {
    const auto count=detail::pixelCount(image.width,image.height);
    if (!count) return std::unexpected(count.error());
    if (image.pixels.size()!=*count)
        return std::unexpected(Error{ErrorCode::InvalidBuffer,"HDR buffer must match width times height"});
    for (const auto p : image.pixels)
        if (!std::isfinite(p.r) || p.r<0 || !std::isfinite(p.g) || p.g<0 || !std::isfinite(p.b) || p.b<0 ||
            !detail::inRange(p.a,0,1))
            return std::unexpected(Error{ErrorCode::InvalidPixel,"HDR RGB must be finite nonnegative and alpha normalized"});
    const auto valid=settings(options);
    if (!valid) return std::unexpected(valid.error());
    return detail::allocated([&] {
        Image result{image.width,image.height,std::vector<Rgba>(*count)};
        for (std::size_t i=0;i<*count;++i) {
            const auto p=image.pixels[i];
            result.pixels[i]=p.a==0 ? Rgba{0,0,0,0} : mapped(p.r,p.g,p.b,p.a,options);
        }
        return result;
    });
}
Result<Image> hdrToning(ImageView image,const HdrToningOptions& options) {
    auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    valid=settings(options);
    if (!valid) return std::unexpected(valid.error());
    return detail::allocated([&] {
        auto result=detail::copy(image);
        // An encoded SDR source and neutral ExposureGamma are exactly unchanged.
        if (options.method==HdrToningMethod::ExposureGamma && options.exposure==0 && options.gamma==1) return result;
        for (auto& p : result.pixels)
            if (p.a!=0) p=mapped(detail::linear(p.r),detail::linear(p.g),detail::linear(p.b),p.a,options);
        return result;
    });
}
} // namespace iiGraphicAdjust
