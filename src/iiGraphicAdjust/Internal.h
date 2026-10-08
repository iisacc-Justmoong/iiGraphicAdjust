#pragma once
#include <iiGraphicAdjust/Image.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace iiGraphicAdjust::detail {
inline bool inRange(double value, double lower, double upper) noexcept {
    return std::isfinite(value) && value >= lower && value <= upper;
}
inline double clamp(double value) noexcept { return std::clamp(value,0.,1.); }
inline double linear(double value) noexcept {
    return value <= .04045 ? value/12.92 : std::pow((value+.055)/1.055,2.4);
}
inline float encoded(double value) noexcept {
    value=clamp(value);
    return static_cast<float>(value <= .0031308 ? 12.92*value : 1.055*std::pow(value,1/2.4)-.055);
}
inline Result<std::size_t> pixelCount(std::size_t width,std::size_t height) {
    if (!width || !height || width > std::numeric_limits<std::size_t>::max()/height ||
        width*height > std::vector<Rgba>{}.max_size())
        return std::unexpected(Error{ErrorCode::InvalidDimensions,"Image dimensions are empty or overflow storage"});
    return width*height;
}
inline Result<void> validate(ImageView image) {
    const auto count=pixelCount(image.width,image.height);
    if (!count) return std::unexpected(count.error());
    if (*count != image.pixels.size())
        return std::unexpected(Error{ErrorCode::InvalidBuffer,"Pixel count must equal width times height"});
    for (auto p : image.pixels)
        if (!inRange(p.r,0,1) || !inRange(p.g,0,1) || !inRange(p.b,0,1) || !inRange(p.a,0,1))
            return std::unexpected(Error{ErrorCode::InvalidPixel,"RGBA channels must be finite and normalized"});
    return {};
}
template<class F> auto allocated(F&& operation) -> Result<decltype(operation())> {
    try { return std::forward<F>(operation)(); }
    catch (const std::bad_alloc&) {
        return std::unexpected(Error{ErrorCode::AllocationFailure,"Could not allocate processing storage"});
    } catch (const std::length_error&) {
        return std::unexpected(Error{ErrorCode::AllocationFailure,"Requested processing storage exceeds container limits"});
    }
}
inline Image copy(ImageView image) {
    return {image.width,image.height,{image.pixels.begin(),image.pixels.end()}};
}
inline Image output(ImageView image) {
    return {image.width,image.height,std::vector<Rgba>(image.pixels.size())};
}
inline std::uint8_t byte(float channel) noexcept {
    return static_cast<std::uint8_t>(std::lround(double(channel)*255));
}
} // namespace iiGraphicAdjust::detail
