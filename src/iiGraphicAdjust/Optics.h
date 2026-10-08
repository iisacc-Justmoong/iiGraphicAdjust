#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>
#include <optional>

namespace iiGraphicAdjust {
struct EmbeddedLensWarp final {
    std::array<std::array<double, 6>, 3> planes{
        {{{1, 0, 0, 0, 0, 0}}, {{1, 0, 0, 0, 0, 0}}, {{1, 0, 0, 0, 0, 0}}}};
    double centerX = .5, centerY = .5, radius = .5; // Normalized center and radius / long edge.
};
struct EmbeddedLensVignette final {
    std::array<double, 5> coefficients{};
    double centerX = .5, centerY = .5, radius = .5;
};
struct OpticsOptions final {
    double distortion = 0, vignetting = 0,
           vignettingMidpoint = 50; // [-100,100], [-100,100], [0,100].
    double redChromaticAberration = 0, blueChromaticAberration = 0; // [-100,100].
    bool autoChromaticAberration = false;
    double purpleDefringe = 0, greenDefringe = 0; // [0,20].
    double purpleHueLow = 30, purpleHueHigh = 70, greenHueLow = 40,
           greenHueHigh = 60; // Ordered [0,100].
    std::optional<EmbeddedLensWarp> embeddedWarp;
    std::optional<EmbeddedLensVignette> embeddedVignette;
    double profileDistortion = 100,
           profileVignetting = 100; // [0,200], applied only when metadata is supplied.
};
// Return R/B slider values for alignment to green; weak evidence returns zero.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<std::array<double, 2>>
estimateChromaticAberration(ImageView image);
// Bilinear linear-light alpha-aware inverse warp; transparent fill outside input.
// Dimensions are unchanged. Warp moves alpha; newly invisible output is black.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> correctOptics(ImageView image,
                                                                 const OpticsOptions &options = {});
} // namespace iiGraphicAdjust
