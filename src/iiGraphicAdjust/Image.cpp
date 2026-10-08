#include <iiGraphicAdjust/Image.h>
#include "Internal.h"

namespace iiGraphicAdjust {
Result<Image> fromRgba8(std::size_t width,std::size_t height,std::span<const std::uint8_t> bytes) {
    const auto count=detail::pixelCount(width,height);
    if (!count) return std::unexpected(count.error());
    if (bytes.size()/4 != *count || bytes.size()%4)
        return std::unexpected(Error{ErrorCode::InvalidBuffer,"RGBA8 buffer must contain four bytes per pixel"});
    return detail::allocated([&] {
        Image result{width,height,std::vector<Rgba>(*count)};
        for (std::size_t i=0;i<*count;++i)
            result.pixels[i]={bytes[4*i]/255.F,bytes[4*i+1]/255.F,bytes[4*i+2]/255.F,bytes[4*i+3]/255.F};
        return result;
    });
}
Result<std::vector<std::uint8_t>> toRgba8(ImageView image) {
    const auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    return detail::allocated([&] {
        std::vector<std::uint8_t> bytes(image.pixels.size()*4);
        for (std::size_t i=0;i<image.pixels.size();++i) {
            const auto p=image.pixels[i];
            bytes[4*i]=detail::byte(p.r); bytes[4*i+1]=detail::byte(p.g);
            bytes[4*i+2]=detail::byte(p.b); bytes[4*i+3]=detail::byte(p.a);
        }
        return bytes;
    });
}
Result<Image> fromArgb32(std::size_t width,std::size_t height,std::span<const std::uint32_t> pixels) {
    const auto count=detail::pixelCount(width,height);
    if (!count) return std::unexpected(count.error());
    if (pixels.size() != *count)
        return std::unexpected(Error{ErrorCode::InvalidBuffer,"ARGB32 buffer must contain one word per pixel"});
    return detail::allocated([&] {
        Image result{width,height,std::vector<Rgba>(*count)};
        for (std::size_t i=0;i<*count;++i) {
            const auto p=pixels[i];
            result.pixels[i]={float((p>>16)&255)/255,float((p>>8)&255)/255,float(p&255)/255,float(p>>24)/255};
        }
        return result;
    });
}
Result<std::vector<std::uint32_t>> toArgb32(ImageView image) {
    const auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    return detail::allocated([&] {
        std::vector<std::uint32_t> pixels(image.pixels.size());
        for (std::size_t i=0;i<pixels.size();++i) {
            const auto p=image.pixels[i];
            pixels[i]=(std::uint32_t(detail::byte(p.a))<<24)|(std::uint32_t(detail::byte(p.r))<<16)|
                      (std::uint32_t(detail::byte(p.g))<<8)|detail::byte(p.b);
        }
        return pixels;
    });
}
} // namespace iiGraphicAdjust
