#pragma once
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {

struct BrightnessContrastOptions final {
    double brightness = 0; // Encoded RGB offset, [-1,1].
    double contrast = 0;   // Slope 1+contrast around 0.5, [-1,1].
};

struct LevelChannel final {
    double inputBlack = 0, inputWhite = 1; // [0,1], inputBlack < inputWhite.
    double gamma = 1;                      // Power 1/gamma, [0.01,100].
    double outputBlack = 0, outputWhite = 1; // [0,1]; reversal is supported.
};
struct LevelsOptions final { LevelChannel master, red, green, blue; };

struct CurvePoint final { double input = 0, output = 0; };
struct CurveChannel final {
    // Two or more finite normalized points in strictly increasing input order.
    std::vector<CurvePoint> points{{0,0},{1,1}};
};
struct CurvesOptions final { CurveChannel master, red, green, blue; };

struct ExposureOptions final {
    double stops = 0; // Linear-light exposure stops, [-16,16].
    double offset = 0; // Linear-light additive offset, [-1,1].
    double gamma = 1; // Linear-light power 1/gamma, [0.01,100].
};

struct ShadowsHighlightsOptions final {
    double shadows = 0, highlights = 0; // Lift/darken amounts, [0,1].
    double shadowWidth = .5, highlightWidth = .5; // Tonal widths, (0,1].
};

struct AutoToneOptions final {
    double blackClip = .005, whiteClip = .005; // Fractions, [0,1), sum < 1.
};

// All operations preserve alpha, preserve hidden RGB at alpha zero, and return
// detached storage. Except Exposure, tonal operations use encoded sRGB.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> brightnessContrast(
    ImageView image, const BrightnessContrastOptions& options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> levels(
    ImageView image, const LevelsOptions& options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> curves(
    ImageView image, const CurvesOptions& options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> exposure(
    ImageView image, const ExposureOptions& options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> shadowsHighlights(
    ImageView image, const ShadowsHighlightsOptions& options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> invert(ImageView image);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> posterize(ImageView image, unsigned levelCount);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> threshold(ImageView image, double level = .5);
// Equalize applies a common 256-bin pooled RGB CDF to the three channels.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> equalize(ImageView image);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> autoLevels(
    ImageView image, const AutoToneOptions& options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> autoContrast(
    ImageView image, const AutoToneOptions& options = {});
// Per-channel stretch followed by median-targeted gamma neutralization.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> autoColor(
    ImageView image, const AutoToneOptions& options = {});

} // namespace iiGraphicAdjust
