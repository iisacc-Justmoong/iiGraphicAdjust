#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {
enum class MixerBand { Red, Orange, Yellow, Green, Aqua, Blue, Purple, Magenta };
struct MixerAdjustment final {
    double hue = 0, saturation = 0, luminance = 0;
}; // [-100,100].
struct ColorWheel final {
    double hue = 0, saturation = 0, luminance = 0;
}; // [0,360], [0,100], [-100,100].
struct ColorGradingOptions final {
    ColorWheel shadows, midtones, highlights, global;
    double blending = 50, balance = 0; // [0,100], [-100,100].
};
struct PointColorOptions final {
    double lightness = .5, chroma = .1, hue = 0; // OkLCh [0,1], [0,.5], degrees [-180,360].
    double hueShift = 0, saturationShift = 0, luminanceShift = 0, variance = 0;  // [-100,100].
    double range = 50, hueRange = 50, saturationRange = 50, luminanceRange = 50; // [0,100].
};
struct DevelopColorOptions final {
    double vibrance = 0, saturation = 0;    // [-100,100]. Positive vibrance protects skin hues.
    std::array<MixerAdjustment, 8> mixer{}; // Red .. Magenta in MixerBand order.
    std::vector<PointColorOptions> points;  // At most eight, applied in order.
    bool blackAndWhite = false;
    std::array<double, 8> blackAndWhiteMix{}; // [-100,100].
    ColorGradingOptions grading;
};
struct CalibrationOptions final {
    double shadowsTint = 0;                                  // [-100,100], positive magenta.
    std::array<double, 3> primaryHue{}, primarySaturation{}; // R/G/B, [-100,100].
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<PointColorOptions> samplePointColor(Rgba sample);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<std::array<double, 8>>
autoBlackAndWhiteMix(ImageView image);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
developColor(ImageView image, const DevelopColorOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
calibrate(ImageView image, const CalibrationOptions &options = {});
} // namespace iiGraphicAdjust
