#pragma once

#include <iiGraphicAdjust/Export.h>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <vector>

namespace iiGraphicAdjust {

// Row-major, normalized sRGB with straight (unassociated) alpha.
struct Rgba final {
    float r = 0, g = 0, b = 0, a = 1;
    bool operator==(const Rgba&) const = default;
};

struct ImageView final {
    std::size_t width = 0, height = 0;
    std::span<const Rgba> pixels;
};

struct Image final {
    std::size_t width = 0, height = 0;
    std::vector<Rgba> pixels;
    [[nodiscard]] ImageView view() const noexcept { return {width, height, pixels}; }
};

enum class ErrorCode { InvalidDimensions, InvalidBuffer, InvalidPixel, InvalidParameter,
                       SizeMismatch, AllocationFailure };
struct Error final {
    ErrorCode code;
    std::string_view message; // SDK errors refer to static strings.
};
template<class T> using Result = std::expected<T, Error>;

// Converters use straight alpha. ARGB32 denotes the numeric value 0xAARRGGBB,
// independent of host byte order; RGBA8 is the byte sequence R,G,B,A.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> fromRgba8(
    std::size_t width, std::size_t height, std::span<const std::uint8_t> bytes);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<std::vector<std::uint8_t>> toRgba8(ImageView image);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> fromArgb32(
    std::size_t width, std::size_t height, std::span<const std::uint32_t> pixels);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<std::vector<std::uint32_t>> toArgb32(ImageView image);

} // namespace iiGraphicAdjust
