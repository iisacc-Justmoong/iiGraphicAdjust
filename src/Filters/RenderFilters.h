#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>
#include <optional>

namespace iiGraphicAdjust {
struct CloudsOptions final {
    Rgba foreground{0, 0, 0, 1}, background{1, 1, 1, 1};
    std::uint32_t seed = 0;
};
struct FibersOptions final {
    double variance = 16, strength = 4;
    std::uint32_t seed = 0;
    Rgba foreground{0, 0, 0, 1}, background{1, 1, 1, 1};
};
enum class FlareLens { Zoom, Prime35, Prime105, MoviePrime };
struct LensFlareOptions final {
    double brightness = 100, centerX = .5, centerY = .5;
    FlareLens lens = FlareLens::Zoom;
};
enum class LightKind { Spot, Point, Infinite };
struct FilterLight final {
    LightKind kind = LightKind::Spot;
    double x = .25, y = .2, z = .6, targetX = .5, targetY = .55;
    double angle = 135, elevation = 45;
    std::array<double, 3> color{1, 1, 1};
    double intensity = 75, cone = 45, hotspot = 50, radius = .8;
};
enum class TextureChannel { None, Red, Green, Blue, Alpha, Luminance };
struct LightingEffectsOptions final {
    std::vector<FilterLight> lights{FilterLight{}}; // At most 64 lights.
    double gloss = 0, metallic = 0, exposure = 0, ambience = 8;
    TextureChannel texture = TextureChannel::None;
    double height = 50;
    bool whiteIsHigh = true;
};
struct RelightOptions final {
    double angle = 45, elevation = 40, intensity = 40, ambient = 55, warmth = 0, softness = 25;
};
enum class FlameType {
    OneAlongPath,
    MultipleAlongPath,
    PathDirections,
    VariousLengths,
    CandleLight,
    OneDirection
};
enum class FlameStyle { Normal, Violent, Flat };
enum class FlameShape { Parallel, ToCenter, Spread, Oval, Pointed };
struct FlameOptions final {
    FlameType type = FlameType::OneAlongPath;
    double length = 150, width = 40, angle = 0, interval = 60;
    bool randomizeLength = false, adjustIntervalForLoops = true;
    std::optional<Rgba> color;
    double turbulence = 25, jaggedness = 25, opacity = 75, flameLines = 20, bottomAlignment = 20;
    FlameStyle style = FlameStyle::Normal;
    FlameShape shape = FlameShape::Parallel;
    bool randomizeShapes = false;
    std::uint32_t seed = 0, quality = 2; // Quality [0,4].
    // Flattened paths in pixel coordinates; empty selects the reference default path.
    std::vector<std::vector<std::array<double, 2>>> paths;
};
struct TreeOptions final {
    std::uint32_t baseTreeType = 1, lightDirection = 3; // Type [1,34], light [1,5].
    double leavesAmount = 50, leavesSize = 100, branchesHeight = 100, branchesThickness = 100;
    bool defaultLeaves = true, customBranchColor = false, flatShading = false;
    Rgba leavesColor{.24F, .52F, .16F, 1}, branchesColor{.36F, .25F, .16F, 1};
    std::uint32_t seed = 0;
    double x = .5, y = .95, size = .8;
};
enum class PictureFrameStyle {
    VineWithFlowers,
    VineWithLeaves,
    Ivy,
    Roses,
    Daisies,
    Berries,
    Bamboo,
    Waves,
    Zigzag,
    Dots,
    Rope,
    Scallops,
    DoubleLine,
    Snowflakes,
    Stars,
    Hearts
};
struct PictureFrameOptions final {
    PictureFrameStyle style = PictureFrameStyle::VineWithFlowers;
    double margin = 4, size = 50, arrangement = 50, thickness = 30, fade = 0;
    std::uint32_t lines = 1;
    Rgba vineColor{.25F, .4F, .15F, 1}, flowerColor{.9F, .3F, .45F, 1}, leafColor{.3F, .6F, .2F, 1};
    std::uint32_t seed = 0;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> clouds(ImageView, const CloudsOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> differenceClouds(ImageView,
                                                                    const CloudsOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> fibers(ImageView, const FibersOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lensFlare(ImageView,
                                                             const LensFlareOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
lightingEffects(ImageView, const LightingEffectsOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> relight(ImageView, const RelightOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> flame(ImageView, const FlameOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> tree(ImageView, const TreeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> pictureFrame(ImageView,
                                                                const PictureFrameOptions & = {});
} // namespace iiGraphicAdjust
