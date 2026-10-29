#pragma once
#include <array>
#include <iiGraphicAdjust/Effects.h>
#include <iiGraphicAdjust/Image.h>
namespace iiGraphicAdjust {
enum class MorphologyShape { Square, Round };
enum class FilterColorModel { Rgb, Hsb, Hsl };
struct FilterNoiseOptions final {
    double amount = 12.5;
    NoiseDistribution distribution = NoiseDistribution::Uniform;
    bool monochromatic = true;
    std::uint32_t seed = 0;
};
struct DustAndScratchesOptions final {
    double radius = 1, threshold = 0;
};
struct MorphologyOptions final {
    double radius = 1;
    MorphologyShape shape = MorphologyShape::Square;
    bool affectAlpha = true;
};
struct ReduceNoiseOptions final {
    double strength = 6, preserveDetails = 60, reduceColorNoise = 45, sharpenDetails = 25;
    bool removeJpegArtifact = false;
};
struct CustomFilterOptions final {
    std::array<double, 25> kernel{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    double scale = 1, offset = 0;
};
struct GuidedBlurOptions final {
    double radius = 4, epsilon = .01;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> average(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> filterAddNoise(ImageView,
                                                                  const FilterNoiseOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> median(ImageView, double radius = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
dustAndScratches(ImageView, const DustAndScratchesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> minimum(ImageView,
                                                           const MorphologyOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> maximum(ImageView,
                                                           const MorphologyOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> reduceNoise(ImageView,
                                                               const ReduceNoiseOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> customFilter(ImageView,
                                                                const CustomFilterOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> hsbHsl(ImageView, FilterColorModel input,
                                                          FilterColorModel output);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
deInterlace(ImageView, bool eliminateEven = false, bool interpolate = true);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> ntscColors(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> guidedBlur(ImageView,
                                                              const GuidedBlurOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> crossGuidedBlur(ImageView, ImageView guide,
                                                                   const GuidedBlurOptions & = {});
} // namespace iiGraphicAdjust
