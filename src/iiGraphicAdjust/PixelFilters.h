#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {
struct ColorHalftoneOptions final {
    double maximumRadius = 8;                   // Pixels [4,127].
    std::array<double, 3> angles{108, 162, 90}; // RGB screen angles [-360,360] degrees.
};
struct CrystallizeOptions final {
    double cellSize = 10;
    std::uint32_t seed = 0;
};
enum class MezzotintPattern {
    FineDots,
    MediumDots,
    GrainyDots,
    CoarseDots,
    ShortLines,
    MediumLines,
    LongLines,
    ShortStrokes,
    MediumStrokes,
    LongStrokes
};
struct MezzotintOptions final {
    MezzotintPattern pattern = MezzotintPattern::FineDots;
    std::uint32_t seed = 0;
};
struct PointillizeOptions final {
    double cellSize = 5;
    std::uint32_t seed = 0;
    Rgba background{1, 1, 1, 1};
};
struct EmbossOptions final {
    double angle = 135, height = 3, amount = 100;
};
enum class DiffuseMode { Normal, DarkenOnly, LightenOnly, Anisotropic };
struct DiffuseOptions final {
    DiffuseMode mode = DiffuseMode::Normal;
    std::uint32_t seed = 0;
};
enum class ExtrudeShape { Blocks, Pyramids };
struct ExtrudeOptions final {
    ExtrudeShape shape = ExtrudeShape::Blocks;
    double size = 30, depth = 30;
    bool levelBased = false, solidFrontFaces = false, maskIncompleteBlocks = false;
    std::uint32_t seed = 0;
};
struct OilPaintOptions final {
    double stylization = 4, cleanliness = 5, scale = 1, bristleDetail = 5;
    bool lighting = true;
    double angle = -60, shine = 1;
};
enum class TileFill { Background, Foreground, Inverse, Unaltered };
struct TilesOptions final {
    std::uint32_t count = 10;
    double maximumOffset = 10;
    TileFill fill = TileFill::Background;
    Rgba foreground{0, 0, 0, 1}, background{1, 1, 1, 1};
    std::uint32_t seed = 0;
};
struct TraceContourOptions final {
    double level = 128;
    bool upper = false;
};
enum class WindMethod { Wind, Blast, Stagger };
struct WindOptions final {
    WindMethod method = WindMethod::Wind;
    bool fromRight = true;
    std::uint32_t seed = 0;
};

// Encoded sRGB filters; spatial colors exclude hidden RGB using alpha weighting.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> colorHalftone(ImageView,
                                                                 const ColorHalftoneOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> crystallize(ImageView,
                                                               const CrystallizeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> facet(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> fragment(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> mezzotint(ImageView,
                                                             const MezzotintOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> pointillize(ImageView,
                                                               const PointillizeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> mosaic(ImageView, double cellSize = 10);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> emboss(ImageView, const EmbossOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> findEdges(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> solarize(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> diffuse(ImageView, const DiffuseOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> extrude(ImageView, const ExtrudeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> oilPaint(ImageView,
                                                            const OilPaintOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> tiles(ImageView, const TilesOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> traceContour(ImageView,
                                                                const TraceContourOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> wind(ImageView, const WindOptions & = {});
} // namespace iiGraphicAdjust
