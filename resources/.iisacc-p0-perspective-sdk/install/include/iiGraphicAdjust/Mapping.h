#pragma once
#include <iiGraphicAdjust/Image.h>
#include <array>

namespace iiGraphicAdjust {

struct GradientStop final {
    double position = 0;
    std::array<double,3> color{0,0,0};
};
struct GradientMapOptions final {
    std::vector<GradientStop> stops{{0,{0,0,0}},{1,{1,1,1}}};
    bool reverse = false;
    bool dither = false;
    double amount = 1;
};

enum class LutKind { OneDimensional, ThreeDimensional };
enum class LutInterpolation { Trilinear, Tetrahedral };
struct ColorLut final {
    LutKind kind = LutKind::ThreeDimensional;
    std::size_t size = 0;
    // One-dimensional: size RGB rows. Three-dimensional: size^3 rows, red fastest.
    std::vector<std::array<float,3>> values;
    std::array<double,3> domainMin{0,0,0};
    std::array<double,3> domainMax{1,1,1};
};
struct ColorLookupOptions final {
    LutInterpolation interpolation = LutInterpolation::Tetrahedral;
    double amount = 1;
    bool dither = false;
};

[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> gradientMap(
    ImageView image, const GradientMapOptions& options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<ColorLut> identityLut(std::size_t size = 2);
// Strict text parser: standalone LUT_1D_SIZE or LUT_3D_SIZE, TITLE, DOMAIN_MIN/MAX,
// comments, finite RGB rows. Combined shaper/3D files and other formats are rejected.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<ColorLut> parseCubeLut(std::string_view text);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> colorLookup(
    ImageView image, const ColorLut& lut, const ColorLookupOptions& options = {});
} // namespace iiGraphicAdjust
