#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>
#include <optional>
namespace iiGraphicAdjust {
enum class LensFilterEdge { Extension, Transparency, Color };
struct LensFilterProfile final {
    double k1 = 0, k2 = 0, p1 = 0, p2 = 0;
    std::array<double, 3> vignette{};
    double caRed = 0, caBlue = 0;
};
struct LensCorrectionOptions final {
    std::optional<LensFilterProfile> profile;
    bool correctDistortion = true, correctVignette = true, correctCa = true;
    double distortion = 0, redCyan = 0, blueYellow = 0, vignetteAmount = 0, vignetteMidpoint = 50;
    double vertical = 0, horizontal = 0, angle = 0, scale = 100;
    LensFilterEdge edge = LensFilterEdge::Transparency;
    Rgba edgeColor{0, 0, 0, 1};
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<LensFilterProfile> genericLensProfile(double focal35mm);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
lensCorrection(ImageView, const LensCorrectionOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<double>
autoLensScale(ImageView, const LensCorrectionOptions & = {});
} // namespace iiGraphicAdjust
