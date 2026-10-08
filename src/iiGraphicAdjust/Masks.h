#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {
struct MaskView final {
    std::size_t width = 0, height = 0;
    std::span<const float> coverage;
};
struct Mask final {
    std::size_t width = 0, height = 0;
    std::vector<float> coverage;
    [[nodiscard]] MaskView view() const noexcept {
        return {width, height, coverage};
    }
};
struct MaskPoint final {
    double x = 0, y = 0;
}; // Normalized image coordinates [-2,3].
struct LinearMaskOptions final {
    MaskPoint start{0, 0}, end{1, 0};
};
struct RadialMaskOptions final {
    MaskPoint center{.5, .5};
    double radiusX = .25, radiusY = .25, angle = 0, feather = 50;
    bool invert = false;
}; // Radii [1e-6,2] long-edge units, angle [-180,180], feather [0,100].
struct BrushStroke final {
    std::vector<MaskPoint> points;
    double radius = .03, feather = 50, flow = 100, density = 100;
    bool erase = false, autoMask = false;
};
struct LuminanceRangeOptions final {
    double low = 0, high = 1, lowFeather = .1, highFeather = .1, exposure = 0;
}; // Bounds/feather [0,1]; bounds map log luminance [-8,+4] EV; exposure [-5,5].
struct ColorRangeOptions final {
    std::vector<Rgba> samples;       // 1..8 visible normalized colors.
    double range = 50, exposure = 0; // [0,100], [-5,5].
};
enum class MaskCombine { Add, Subtract, Intersect };
struct LocalAdjustmentOptions final {
    double exposure = 0, temperature = 0, tint = 0, contrast = 0, highlights = 0, shadows = 0,
           whites = 0, blacks = 0;
    double texture = 0, clarity = 0, dehaze = 0, hue = 0, saturation = 0, sharpness = 0, noise = 0,
           moire = 0, defringe = 0;
    double colorHue = 0, colorSaturation = 0, amount = 100;
}; // Exposure [-5,5]; signed sliders [-100,100]; defringe/colorSaturation [0,100], colorHue
   // [0,360], amount [0,200].
struct LocalAdjustment final {
    MaskView mask;
    LocalAdjustmentOptions options;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask> linearMask(std::size_t width, std::size_t height,
                                                             const LinearMaskOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask> radialMask(std::size_t width, std::size_t height,
                                                             const RadialMaskOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask> brushMask(ImageView image,
                                                            std::span<const BrushStroke> strokes);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask>
luminanceRangeMask(ImageView image, const LuminanceRangeOptions &options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask> colorRangeMask(ImageView image,
                                                                 const ColorRangeOptions &options);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask> combineMasks(MaskView base, MaskView operand,
                                                               MaskCombine operation);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask> invertMask(MaskView mask);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask> refineMask(ImageView image, MaskView mask,
                                                             double amount);
// Up to 32 borrowed masks. Slider contributions accumulate before image processing.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
applyLocalAdjustments(ImageView image, std::span<const LocalAdjustment> adjustments);
} // namespace iiGraphicAdjust
