#pragma once
#include <iiGraphicAdjust/Tone.h>

namespace iiGraphicAdjust {
// Lightroom slider units. The input is rendered sRGB, already balanced to asShot.
struct WhiteBalanceOptions final {
    double temperature = 6500, tint = 0; // Kelvin [2000,50000], tint [-150,150].
    double asShotTemperature = 6500, asShotTint = 0;
};
enum class WhiteBalancePreset { Daylight, Cloudy, Shade, Tungsten, Fluorescent, Flash };
struct DevelopLightOptions final {
    double exposure = 0; // Stops [-5,5]. Other sliders [-100,100].
    double contrast = 0, highlights = 0, shadows = 0, whites = 0, blacks = 0;
};
struct DevelopCurveOptions final {
    double shadows = 0, darks = 0, lights = 0, highlights = 0;     // [-100,100].
    double splitShadows = 25, splitMid = 50, splitHighlights = 75; // Strictly ordered in (0,100).
    CurveChannel master, red, green, blue; // Cubic Hermite, no segment overshoot.
    double refineSaturation = 100;         // [0,100]: retain pre-curve chroma at zero.
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<WhiteBalanceOptions>
whiteBalancePreset(WhiteBalancePreset preset);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<WhiteBalanceOptions>
autoWhiteBalance(ImageView image, const WhiteBalanceOptions &source = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<WhiteBalanceOptions>
sampledWhiteBalance(Rgba sample, const WhiteBalanceOptions &source = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<DevelopLightOptions> autoDevelopLight(ImageView image);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
whiteBalance(ImageView image, const WhiteBalanceOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
developLight(ImageView image, const DevelopLightOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
developCurve(ImageView image, const DevelopCurveOptions &options = {});
} // namespace iiGraphicAdjust
