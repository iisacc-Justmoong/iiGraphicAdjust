#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>
#include <variant>

namespace iiGraphicAdjust {
// Filter Gallery works in normalized encoded sRGB. Colors contain RGB only; alpha is retained.
using GalleryColor = std::array<double, 3>;
enum class GalleryBrushType { Simple, LightRough, DarkRough, WideSharp, WideBlurry, Sparkle };
enum class GalleryTexture { Canvas, Brick, Burlap, Sandstone };
enum class GalleryLightDirection {
    Bottom,
    BottomLeft,
    Left,
    TopLeft,
    Top,
    TopRight,
    Right,
    BottomRight
};
enum class GalleryStrokeDirection { RightDiagonal, Horizontal, LeftDiagonal, Vertical };
enum class GalleryGlassTexture { Frosted, Blocks, Canvas, TinyLens };
enum class GalleryPatternType { Dot, Circle, Line };
enum class GalleryGrainType {
    Regular,
    Soft,
    Sprinkles,
    Clumped,
    Contrasty,
    Enlarged,
    Stippled,
    Horizontal,
    Vertical,
    Speckle
};

struct GalleryColoredPencilOptions final {
    double pencilWidth = 4;              // [1,24]
    double strokePressure = 8;           // [0,15]
    double paperBrightness = 25;         // [0,50]
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryCutoutOptions final {
    double numberOfLevels = 4; // [2,8]
    double edgeSimplicity = 4; // [0,10]
    double edgeFidelity = 2;   // [1,3]
};

struct GalleryDryBrushOptions final {
    double brushSize = 2;   // [0,10]
    double brushDetail = 8; // [0,10]
    double texture = 1;     // [1,3]
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GalleryFilmGrainOptions final {
    double grain = 4;         // [0,20]
    double highlightArea = 0; // [0,20]
    double intensity = 10;    // [0,10]
    std::uint32_t seed = 0;   // XOR salt for coordinate-stable procedural noise.
};

struct GalleryFrescoOptions final {
    double brushSize = 2;   // [0,10]
    double brushDetail = 8; // [0,10]
    double texture = 1;     // [1,3]
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GalleryNeonGlowOptions final {
    double glowSize = 5;                  // [-24,24]
    double glowBrightness = 15;           // [0,50]
    GalleryColor glowColor = {0, .62, 1}; // Normalized encoded sRGB.
    GalleryColor foreground = {0, 0, 0};  // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1};  // Normalized encoded sRGB.
};

struct GalleryPaintDaubsOptions final {
    double brushSize = 8; // [1,50]
    double sharpness = 7; // [0,40]
    GalleryBrushType brushType = GalleryBrushType::Simple;
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GalleryPaletteKnifeOptions final {
    double strokeSize = 25;  // [1,50]
    double strokeDetail = 3; // [1,3]
    double softness = 0;     // [0,10]
};

struct GalleryPlasticWrapOptions final {
    double highlightStrength = 15; // [0,20]
    double detail = 9;             // [1,15]
    double smoothness = 7;         // [1,15]
};

struct GalleryPosterEdgesOptions final {
    double edgeThickness = 2; // [0,10]
    double edgeIntensity = 1; // [0,10]
    double posterization = 2; // [0,6]
};

struct GalleryRoughPastelsOptions final {
    double strokeLength = 6; // [0,40]
    double strokeDetail = 4; // [1,20]
    GalleryTexture texture = GalleryTexture::Canvas;
    double scaling = 100; // [50,200]
    double relief = 20;   // [0,50]
    GalleryLightDirection light = GalleryLightDirection::Bottom;
    bool invert = false;
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GallerySmudgeStickOptions final {
    double strokeLength = 2;  // [0,10]
    double highlightArea = 0; // [0,20]
    double intensity = 10;    // [0,10]
};

struct GallerySpongeOptions final {
    double brushSize = 2;   // [0,10]
    double definition = 12; // [0,25]
    double smoothness = 5;  // [1,15]
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GalleryUnderpaintingOptions final {
    double brushSize = 6;        // [0,40]
    double textureCoverage = 16; // [0,40]
    GalleryTexture texture = GalleryTexture::Canvas;
    double scaling = 100; // [50,200]
    double relief = 4;    // [0,50]
    GalleryLightDirection light = GalleryLightDirection::Top;
    bool invert = false;
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GalleryWatercolorOptions final {
    double brushDetail = 9;     // [1,14]
    double shadowIntensity = 1; // [0,10]
    double texture = 1;         // [1,3]
    std::uint32_t seed = 0;     // XOR salt for coordinate-stable procedural noise.
};

struct GalleryAccentedEdgesOptions final {
    double edgeWidth = 2;       // [1,14]
    double edgeBrightness = 38; // [0,50]
    double smoothness = 5;      // [1,15]
};

struct GalleryAngledStrokesOptions final {
    double directionBalance = 50; // [0,100]
    double strokeLength = 15;     // [3,50]
    double sharpness = 3;         // [0,10]
    std::uint32_t seed = 0;       // XOR salt for coordinate-stable procedural noise.
};

struct GalleryCrosshatchOptions final {
    double strokeLength = 9; // [3,50]
    double sharpness = 6;    // [0,20]
    double strength = 1;     // [1,3]
    std::uint32_t seed = 0;  // XOR salt for coordinate-stable procedural noise.
};

struct GalleryDarkStrokesOptions final {
    double balance = 5;        // [0,10]
    double blackIntensity = 6; // [0,10]
    double whiteIntensity = 2; // [0,10]
};

struct GalleryInkOutlinesOptions final {
    double strokeLength = 4;    // [1,50]
    double darkIntensity = 20;  // [0,50]
    double lightIntensity = 10; // [0,50]
};

struct GallerySpatterOptions final {
    double sprayRadius = 10; // [0,25]
    double smoothness = 5;   // [1,15]
    std::uint32_t seed = 0;  // XOR salt for coordinate-stable procedural noise.
};

struct GallerySprayedStrokesOptions final {
    double strokeLength = 12; // [0,20]
    double sprayRadius = 7;   // [0,25]
    GalleryStrokeDirection strokeDirection = GalleryStrokeDirection::RightDiagonal;
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GallerySumiEOptions final {
    double strokeWidth = 10;   // [3,15]
    double strokePressure = 2; // [0,15]
    double contrast = 16;      // [0,40]
};

struct GalleryDiffuseGlowOptions final {
    double graininess = 6;               // [0,10]
    double glowAmount = 10;              // [0,20]
    double clearAmount = 15;             // [0,20]
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryGlassOptions final {
    double distortion = 5; // [0,20]
    double smoothness = 3; // [1,15]
    GalleryGlassTexture texture = GalleryGlassTexture::Frosted;
    double scaling = 100; // [50,200]
    bool invert = false;
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GalleryOceanRippleOptions final {
    double rippleSize = 9;      // [1,15]
    double rippleMagnitude = 9; // [0,20]
    std::uint32_t seed = 0;     // XOR salt for coordinate-stable procedural noise.
};

struct GalleryBasReliefOptions final {
    double detail = 13;    // [1,15]
    double smoothness = 3; // [1,15]
    GalleryLightDirection light = GalleryLightDirection::Bottom;
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
};

struct GalleryChalkCharcoalOptions final {
    double charcoalArea = 6;             // [0,20]
    double chalkArea = 6;                // [0,20]
    double strokePressure = 1;           // [0,5]
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryCharcoalOptions final {
    double charcoalThickness = 1;        // [1,7]
    double detail = 5;                   // [0,5]
    double lightDarkBalance = 50;        // [0,100]
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryChromeOptions final {
    double detail = 4;     // [0,10]
    double smoothness = 7; // [0,10]
};

struct GalleryConteCrayonOptions final {
    double foregroundLevel = 11; // [1,15]
    double backgroundLevel = 7;  // [1,15]
    GalleryTexture texture = GalleryTexture::Canvas;
    double scaling = 100; // [50,200]
    double relief = 4;    // [0,50]
    GalleryLightDirection light = GalleryLightDirection::Top;
    bool invert = false;
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryGraphicPenOptions final {
    double strokeLength = 15;     // [1,15]
    double lightDarkBalance = 50; // [0,100]
    GalleryStrokeDirection strokeDirection = GalleryStrokeDirection::RightDiagonal;
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryHalftonePatternOptions final {
    double size = 1;     // [1,12]
    double contrast = 5; // [0,50]
    GalleryPatternType patternType = GalleryPatternType::Dot;
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
};

struct GalleryNotePaperOptions final {
    double imageBalance = 25;            // [0,50]
    double graininess = 10;              // [0,20]
    double relief = 11;                  // [0,25]
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryPhotocopyOptions final {
    double detail = 7;                   // [1,24]
    double darkness = 8;                 // [1,50]
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
};

struct GalleryPlasterOptions final {
    double imageBalance = 20; // [0,50]
    double smoothness = 2;    // [1,15]
    GalleryLightDirection light = GalleryLightDirection::Top;
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
};

struct GalleryReticulationOptions final {
    double density = 12;                 // [0,50]
    double foregroundLevel = 40;         // [0,50]
    double backgroundLevel = 5;          // [0,50]
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryStampOptions final {
    double lightDarkBalance = 25;        // [0,50]
    double smoothness = 5;               // [1,50]
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
};

struct GalleryTornEdgesOptions final {
    double imageBalance = 25;            // [0,50]
    double smoothness = 11;              // [1,15]
    double contrast = 17;                // [1,25]
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryWaterPaperOptions final {
    double fiberLength = 15; // [3,50]
    double brightness = 60;  // [0,100]
    double contrast = 80;    // [0,100]
    std::uint32_t seed = 0;  // XOR salt for coordinate-stable procedural noise.
};

struct GalleryGlowingEdgesOptions final {
    double edgeWidth = 2;      // [1,14]
    double edgeBrightness = 6; // [0,20]
    double smoothness = 5;     // [1,15]
};

struct GalleryCraquelureOptions final {
    double crackSpacing = 15;   // [2,100]
    double crackDepth = 6;      // [0,10]
    double crackBrightness = 9; // [0,10]
    std::uint32_t seed = 0;     // XOR salt for coordinate-stable procedural noise.
};

struct GalleryGrainOptions final {
    double intensity = 40; // [0,100]
    double contrast = 50;  // [0,100]
    GalleryGrainType grainType = GalleryGrainType::Regular;
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    GalleryColor background = {1, 1, 1}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryMosaicTilesOptions final {
    double tileSize = 12;    // [2,100]
    double groutWidth = 3;   // [1,15]
    double lightenGrout = 9; // [0,10]
    std::uint32_t seed = 0;  // XOR salt for coordinate-stable procedural noise.
};

struct GalleryPatchworkOptions final {
    double squareSize = 4;  // [0,10]
    double relief = 8;      // [0,25]
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

struct GalleryStainedGlassOptions final {
    double cellSize = 10;                // [2,50]
    double borderThickness = 4;          // [1,20]
    double lightIntensity = 3;           // [0,10]
    GalleryColor foreground = {0, 0, 0}; // Normalized encoded sRGB.
    std::uint32_t seed = 0;              // XOR salt for coordinate-stable procedural noise.
};

struct GalleryTexturizerOptions final {
    GalleryTexture texture = GalleryTexture::Canvas;
    double scaling = 100; // [50,200]
    double relief = 4;    // [0,50]
    GalleryLightDirection light = GalleryLightDirection::Top;
    bool invert = false;
    std::uint32_t seed = 0; // XOR salt for coordinate-stable procedural noise.
};

// Every function returns detached storage, preserves source alpha and zero-alpha RGB.
// Spatial samples ignore hidden RGB and normalize their weights by alpha.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
coloredPencil(ImageView, const GalleryColoredPencilOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> cutout(ImageView,
                                                          const GalleryCutoutOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> dryBrush(ImageView,
                                                            const GalleryDryBrushOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
galleryFilmGrain(ImageView, const GalleryFilmGrainOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> fresco(ImageView,
                                                          const GalleryFrescoOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> neonGlow(ImageView,
                                                            const GalleryNeonGlowOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
paintDaubs(ImageView, const GalleryPaintDaubsOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
paletteKnife(ImageView, const GalleryPaletteKnifeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
plasticWrap(ImageView, const GalleryPlasticWrapOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
posterEdges(ImageView, const GalleryPosterEdgesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
roughPastels(ImageView, const GalleryRoughPastelsOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
smudgeStick(ImageView, const GallerySmudgeStickOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> sponge(ImageView,
                                                          const GallerySpongeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
underpainting(ImageView, const GalleryUnderpaintingOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
watercolor(ImageView, const GalleryWatercolorOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
accentedEdges(ImageView, const GalleryAccentedEdgesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
angledStrokes(ImageView, const GalleryAngledStrokesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
crosshatch(ImageView, const GalleryCrosshatchOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
darkStrokes(ImageView, const GalleryDarkStrokesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
inkOutlines(ImageView, const GalleryInkOutlinesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> spatter(ImageView,
                                                           const GallerySpatterOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
sprayedStrokes(ImageView, const GallerySprayedStrokesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> sumiE(ImageView,
                                                         const GallerySumiEOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
diffuseGlow(ImageView, const GalleryDiffuseGlowOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> glass(ImageView,
                                                         const GalleryGlassOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
oceanRipple(ImageView, const GalleryOceanRippleOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> basRelief(ImageView,
                                                             const GalleryBasReliefOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
chalkCharcoal(ImageView, const GalleryChalkCharcoalOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> charcoal(ImageView,
                                                            const GalleryCharcoalOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> chrome(ImageView,
                                                          const GalleryChromeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
conteCrayon(ImageView, const GalleryConteCrayonOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
graphicPen(ImageView, const GalleryGraphicPenOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
halftonePattern(ImageView, const GalleryHalftonePatternOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> notePaper(ImageView,
                                                             const GalleryNotePaperOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> photocopy(ImageView,
                                                             const GalleryPhotocopyOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> plaster(ImageView,
                                                           const GalleryPlasterOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
reticulation(ImageView, const GalleryReticulationOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> stamp(ImageView,
                                                         const GalleryStampOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> tornEdges(ImageView,
                                                             const GalleryTornEdgesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
waterPaper(ImageView, const GalleryWaterPaperOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
glowingEdges(ImageView, const GalleryGlowingEdgesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
craquelure(ImageView, const GalleryCraquelureOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> grain(ImageView,
                                                         const GalleryGrainOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
mosaicTiles(ImageView, const GalleryMosaicTilesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> patchwork(ImageView,
                                                             const GalleryPatchworkOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
stainedGlass(ImageView, const GalleryStainedGlassOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
texturizer(ImageView, const GalleryTexturizerOptions & = {});

// Stack effects use the same typed options as their individual operations.
using GalleryEffect =
    std::variant<GalleryColoredPencilOptions, GalleryCutoutOptions, GalleryDryBrushOptions,
                 GalleryFilmGrainOptions, GalleryFrescoOptions, GalleryNeonGlowOptions,
                 GalleryPaintDaubsOptions, GalleryPaletteKnifeOptions, GalleryPlasticWrapOptions,
                 GalleryPosterEdgesOptions, GalleryRoughPastelsOptions, GallerySmudgeStickOptions,
                 GallerySpongeOptions, GalleryUnderpaintingOptions, GalleryWatercolorOptions,
                 GalleryAccentedEdgesOptions, GalleryAngledStrokesOptions, GalleryCrosshatchOptions,
                 GalleryDarkStrokesOptions, GalleryInkOutlinesOptions, GallerySpatterOptions,
                 GallerySprayedStrokesOptions, GallerySumiEOptions, GalleryDiffuseGlowOptions,
                 GalleryGlassOptions, GalleryOceanRippleOptions, GalleryBasReliefOptions,
                 GalleryChalkCharcoalOptions, GalleryCharcoalOptions, GalleryChromeOptions,
                 GalleryConteCrayonOptions, GalleryGraphicPenOptions, GalleryHalftonePatternOptions,
                 GalleryNotePaperOptions, GalleryPhotocopyOptions, GalleryPlasterOptions,
                 GalleryReticulationOptions, GalleryStampOptions, GalleryTornEdgesOptions,
                 GalleryWaterPaperOptions, GalleryGlowingEdgesOptions, GalleryCraquelureOptions,
                 GalleryGrainOptions, GalleryMosaicTilesOptions, GalleryPatchworkOptions,
                 GalleryStainedGlassOptions, GalleryTexturizerOptions>;
// Up to 64 effects are applied in order; an empty stack returns detached source storage.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> filterGallery(ImageView,
                                                                 std::span<const GalleryEffect>);
} // namespace iiGraphicAdjust
