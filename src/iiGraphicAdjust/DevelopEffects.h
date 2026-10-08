#pragma once
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {
struct DevelopEffectsOptions final {
    double texture = 0, clarity = 0, dehaze = 0;
}; // [-100,100].
enum class VignetteStyle { HighlightPriority, ColorPriority, PaintOverlay };
struct PostCropVignetteOptions final {
    double amount = 0, midpoint = 50, roundness = 0, feather = 50, highlights = 0;
    VignetteStyle style = VignetteStyle::HighlightPriority;
}; // amount/roundness [-100,100], other sliders [0,100].
struct FilmGrainOptions final {
    double amount = 0, size = 25, roughness = 50; // [0,100].
    std::uint32_t seed = 0;
};
struct DevelopDetailOptions final {
    double sharpenAmount = 0, sharpenMasking = 0; // [0,150], [0,100].
    double luminanceNoise = 0, luminanceDetail = 50;
    double colorNoise = 0, colorDetail = 50, colorSmoothness = 50; // [0,100].
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
developEffects(ImageView image, const DevelopEffectsOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
postCropVignette(ImageView image, const PostCropVignetteOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> filmGrain(ImageView image,
                                                             const FilmGrainOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
developDetail(ImageView image, const DevelopDetailOptions &options = {});
} // namespace iiGraphicAdjust
