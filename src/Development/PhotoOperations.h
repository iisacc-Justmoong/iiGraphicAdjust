#pragma once

#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {

// Numeric values match the EXIF Orientation tag. Operations transform stored
// pixels into their displayed top-left orientation, preserving every RGBA value.
enum class ExifOrientation : std::uint8_t {
    TopLeft = 1,
    TopRight = 2,
    BottomRight = 3,
    BottomLeft = 4,
    LeftTop = 5,
    RightTop = 6,
    RightBottom = 7,
    LeftBottom = 8
};

struct PixelRect final {
    std::size_t x = 0, y = 0, width = 0, height = 0;
};

enum class HistogramAlphaPolicy {
    CoverageWeighted, // Each pixel has weight alpha. Zero-alpha pixels are excluded.
    VisiblePixels,    // Every positive-alpha pixel has weight one.
    AllPixels         // Every pixel has weight one, including hidden RGB.
};
struct HistogramOptions final {
    std::size_t bins = 256; // Number of equal-width bins, [2,65536].
    HistogramAlphaPolicy alphaPolicy = HistogramAlphaPolicy::CoverageWeighted;
};
struct ChannelHistogram final {
    std::vector<double> bins; // Weighted counts; the last bin includes the value one.
    double minimum = 0, maximum = 0, mean = 0; // Zero when no pixel contributes.
};
struct ImageHistogram final {
    ChannelHistogram red, green, blue; // Encoded, normalized sRGB channel values.
    ChannelHistogram luminance;        // Linear-light Rec709 luminance.
    std::size_t pixelCount = 0;
    std::size_t contributingPixels = 0;
    double totalWeight = 0;
};

// Result owns detached storage, even for TopLeft. Orientations 5-8 swap width
// and height. Callers must reset any separately stored EXIF orientation to one.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> applyOrientation(ImageView, ExifOrientation);
// Rectangle uses source pixel coordinates and must be nonempty and fully inside
// the source. No implicit clamping, padding or resampling occurs.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> crop(ImageView, PixelRect);
// RGB statistics use encoded sRGB; luminance first decodes sRGB, then computes
// 0.2126 R + 0.7152 G + 0.0722 B. Each channel uses the same alpha weight.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<ImageHistogram> imageHistogram(
    ImageView, const HistogramOptions& = {});

} // namespace iiGraphicAdjust
