#pragma once
#include <iiGraphicAdjust/Effects.h>
#include <iiGraphicAdjust/Image.h>
#include <optional>

namespace iiGraphicAdjust {
enum class RadialBlurMethod { Spin, Zoom };
enum class FilterBlurQuality { Low, Medium, High };
enum class SmartBlurMode { Normal, EdgeOnly, OverlayEdge };
enum class BlurShape { Circle, Ring, Square, Diamond, Triangle, Hexagon, Star, Heart, Cross };
enum class LensDepthSource { None, Transparency, Map };
struct MotionBlurOptions final {
    double angle = 0, distance = 10;
};
struct RadialBlurOptions final {
    double amount = 10;
    RadialBlurMethod method = RadialBlurMethod::Spin;
    double centerX = .5, centerY = .5;
};
struct SurfaceBlurOptions final {
    double radius = 5, threshold = 15;
};
struct UnsharpMaskOptions final {
    double amount = 50, radius = 1, threshold = 0;
};
struct SmartSharpenOptions final {
    double amount = 100, radius = 1, reduceNoise = 10;
};
struct SmartBlurOptions final {
    double radius = 3, threshold = 25;
    FilterBlurQuality quality = FilterBlurQuality::Medium;
    SmartBlurMode mode = SmartBlurMode::Normal;
};
struct ShapeBlurOptions final {
    double radius = 10;
    BlurShape shape = BlurShape::Circle;
};
struct LensBlurOptions final {
    double radius = 15;
    unsigned blades = 6;
    double curvature = 0, rotation = 0;
    LensDepthSource depth = LensDepthSource::None;
    double focalDistance = 0;
    bool invertDepth = false;
    double brightness = 0, threshold = 255, noise = 0;
    NoiseDistribution distribution = NoiseDistribution::Uniform;
    bool monochromatic = true;
    std::uint32_t seed = 0;
    std::optional<ImageView> depthMap;
};
struct TiltShiftOptions final {
    double blur = 15, centerX = .5, centerY = .5, angle = 0, focus = .1, transition = .15;
};
struct IrisBlurPin final {
    double x = .5, y = .5, radiusX = .35, radiusY = .25, angle = 0, roundness = 0, feather = .5,
           blur = 15;
};
struct FieldBlurPin final {
    double x = .5, y = .5, blur = 15;
};
struct SpinBlurPin final {
    double x = .5, y = .5, radiusX = .3, radiusY = .3, angle = 0, blurAngle = 15;
};
struct BlurPathPoint final {
    double x = 0, y = 0;
};
struct BlurPath final {
    std::vector<BlurPathPoint> points{{.2, .5}, {.8, .5}};
    double speed = 50, taper = 0;
};

// Photoshop-menu units; encoded-sRGB premultiplied sampling. Bounds and alpha rules in
// docs/Filters.md.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> filterGaussianBlur(ImageView, double sigma = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> boxBlur(ImageView, double radius = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> blur(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> blurMore(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> motionBlur(ImageView,
                                                              const MotionBlurOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> radialBlur(ImageView,
                                                              const RadialBlurOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> surfaceBlur(ImageView,
                                                               const SurfaceBlurOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> unsharpMask(ImageView,
                                                               const UnsharpMaskOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> filterSharpen(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> sharpenMore(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> sharpenEdges(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> smartSharpen(ImageView,
                                                                const SmartSharpenOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> highPass(ImageView, double radius = 10);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> despeckle(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> smartBlur(ImageView,
                                                             const SmartBlurOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> shapeBlur(ImageView,
                                                             const ShapeBlurOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lensBlur(ImageView,
                                                            const LensBlurOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> tiltShift(ImageView,
                                                             const TiltShiftOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> irisBlur(ImageView,
                                                            std::span<const IrisBlurPin>);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> fieldBlur(ImageView,
                                                             std::span<const FieldBlurPin>);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> spinBlur(ImageView,
                                                            std::span<const SpinBlurPin>);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> pathBlur(ImageView, std::span<const BlurPath>);
} // namespace iiGraphicAdjust
