#pragma once
#include <array>
#include <cstdint>
#include <iiGraphicAdjust/Image.h>
#include <vector>

namespace iiGraphicAdjust {
struct CameraRawWheel final {
    double hue = 0, saturation = 0, luminance = 0; // [0,360], [0,100], [-100,100].
};
enum class CameraRawVignetteStyle { HighlightPriority, ColorPriority, PaintOverlay };
struct CameraRawOptions final {
    // Relative rendered-image white balance, not Kelvin.
    double temperature = 0, tint = 0;
    double exposure = 0; // [-5,5] stops.
    double contrast = 0, highlights = 0, shadows = 0, whites = 0, blacks = 0;
    double texture = 0, clarity = 0, dehaze = 0, vibrance = 0, saturation = 0; // [-100,100].
    double curveHighlights = 0, curveLights = 0, curveDarks = 0, curveShadows = 0;
    std::array<double, 3> curveSplits{25, 50, 75}; // [5,95], strictly increasing.
    // Empty = linear; otherwise 2..16 [input,output] pairs in [0,255], input gaps >= 1.
    std::vector<std::array<double, 2>> pointCurve, pointCurveRed, pointCurveGreen, pointCurveBlue;
    // Red, orange, yellow, green, aqua, blue, purple, magenta; [-100,100].
    std::array<double, 8> hslHue{}, hslSaturation{}, hslLuminance{};
    CameraRawWheel gradeShadows, gradeMidtones, gradeHighlights, gradeGlobal;
    double gradeBlending = 50, gradeBalance = 0;
    double sharpenAmount = 0, sharpenRadius = 1, sharpenDetail = 25, sharpenMasking = 0;
    double noiseLuminance = 0, noiseLuminanceDetail = 50, noiseColor = 0, noiseColorDetail = 50;
    double grainAmount = 0, grainSize = 25, grainRoughness = 50;
    double vignetteAmount = 0, vignetteMidpoint = 50, vignetteRoundness = 0, vignetteFeather = 50,
           vignetteHighlights = 0;
    CameraRawVignetteStyle vignetteStyle = CameraRawVignetteStyle::HighlightPriority;
    std::uint32_t seed = 0;
    double pixelScale = 1; // [.01,1], reference preview radius/grain scale.
};
// Photocraft's rendered RGB Camera Raw filter, not a RAW decoder or Kelvin-WB adapter.
// Intermediate values remain extended; final RGB is normalized SDR. Alpha is preserved.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
cameraRawFilter(ImageView image, const CameraRawOptions &options = {});
} // namespace iiGraphicAdjust
