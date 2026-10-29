#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>
#include <optional>

namespace iiGraphicAdjust {
using MeshPoint = std::array<double, 2>;   // Image pixels; pixel centers are x+.5, y+.5.
using MeshQuad = std::array<MeshPoint, 4>; // Clockwise from top-left.
enum class MeshInterpolation { Nearest, Bilinear, Bicubic };
struct MeshGeometry final {
    std::vector<MeshPoint> source, destination;
    std::vector<std::array<std::size_t, 3>> triangles;    // Back-to-front drawing order.
    std::vector<std::vector<MeshPoint>> constraintCurves; // Wide-angle source curves.
    double constraintResidual = 0;                        // Max mapped line deviation, pixels.
};

enum class LiquifyTool {
    ForwardWarp,
    Reconstruct,
    Smooth,
    TwirlClockwise,
    TwirlCounterclockwise,
    Pucker,
    Bloat,
    PushLeft,
    Freeze,
    Thaw,
    LassoMask,
    ReconstructAll,
    FreezeAll,
    ThawAll,
    InvertFreeze
};
struct LiquifyPoint final {
    MeshPoint position{};
    double pressure = 1;
}; // [0,1].
struct LiquifyStroke final {
    LiquifyTool tool = LiquifyTool::ForwardWarp;
    double size = 100;                              // Brush diameter [1,15000] pixels.
    double density = 50, pressure = 100, rate = 80; // Each [0,100].
    std::vector<LiquifyPoint> points;
    std::optional<double> amount; // ReconstructAll [0,100]; LassoMask [0,1].
};
struct LiquifyOptions final {
    double cell = 0; // 0 selects 2 pixels/node up to 4.2 MP, 4 beyond; otherwise [1,64].
    std::vector<LiquifyStroke> strokes; // <=64 strokes, <=4096 points, <=16384 interpolated dabs.
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> liquify(ImageView, const LiquifyOptions & = {});

enum class WideAngleModel { Auto, Fisheye, Perspective, FullSpherical };
enum class WideAngleOrientation { Free, Horizontal, Vertical };
struct WideAngleConstraint final {
    MeshPoint a{}, b{};
    WideAngleOrientation orientation = WideAngleOrientation::Free;
};
struct AdaptiveWideAngleOptions final {
    WideAngleModel model = WideAngleModel::Auto;
    double focalLength = 0; // [0,1000] millimeters; 0 selects projection-specific default.
    double cropFactor = 1;  // [.1,10].
    double scale = 100;     // [10,1000] percent, about image center.
    std::vector<WideAngleConstraint> constraints; // <=32 distinct-endpoint scene lines.
    unsigned meshCells = 48;                      // Long-side grid cells [2,96].
    unsigned solverIterations = 1000;             // CGLS cap [1,2000] per outer round, <=4 rounds.
    MeshInterpolation interpolation = MeshInterpolation::Bicubic;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<MeshGeometry>
solveAdaptiveWideAngle(ImageView, const AdaptiveWideAngleOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
adaptiveWideAngle(ImageView, const AdaptiveWideAngleOptions & = {});

enum class PuppetMode { Rigid, Normal, Distort };
enum class PuppetDensity { Fewer, Normal, More };
struct PuppetPin final {
    MeshPoint source{}, destination{};
    std::optional<double> rotation;
    std::int32_t depth = 0;
};
struct PuppetWarpOptions final {
    std::vector<PuppetPin> pins; // <=64. Rotation [-360,360] degrees; depth [-10000,10000].
    PuppetMode mode = PuppetMode::Normal;
    PuppetDensity density = PuppetDensity::Normal;
    double expansion = 2;             // [-200,200] pixels of round coverage growth/erosion.
    unsigned iterations = 40;         // ARAP local/global rounds [1,100].
    unsigned solverIterations = 2000; // Jacobi-preconditioned CG cap [1,2000].
    MeshInterpolation interpolation = MeshInterpolation::Bicubic;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<MeshGeometry>
solvePuppetWarp(ImageView, const PuppetWarpOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> puppetWarp(ImageView,
                                                              const PuppetWarpOptions & = {});

struct PerspectivePlane final {
    MeshQuad source{}, destination{};
};
enum class PerspectiveStraighten { None, Horizontal, Vertical, Auto };
struct PerspectiveWarpOptions final {
    std::vector<PerspectivePlane> planes; // <=16 convex nondegenerate source/destination quads.
    PerspectiveStraighten straighten = PerspectiveStraighten::None;
    double cell = 4; // Source-pixel tessellation size [1,64], capped at 512 cells/axis.
    MeshInterpolation interpolation = MeshInterpolation::Bicubic;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<MeshPoint>
mapPerspectiveWarpPoint(MeshPoint, const PerspectiveWarpOptions &);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
perspectiveWarp(ImageView, const PerspectiveWarpOptions & = {});

struct VanishingPlane final {
    MeshQuad corners{};
};
enum class VanishingEdge { Top, Right, Bottom, Left };
struct VanishingTearOff final {
    std::size_t from = 0;
    VanishingEdge edge = VanishingEdge::Top;
    double angle = 90, depth = 1;
}; // [1,179] deg, [.05,10].
struct VanishingPaste final {
    ImageView image; // Borrowed pixels remain alive throughout the call.
    std::size_t plane = 0;
    MeshPoint at{0, 0}; // Plane coordinates [-2,3].
    double width = 1;   // Fraction of metric plane width [.001,10].
    double opacity = 1; // [0,1].
};
struct VanishingCloneStroke final {
    MeshPoint source{};
    std::vector<MeshPoint> points; // Recorded dabs, <=4096 across all clone strokes.
    double size = 100, hardness = .5, opacity = 1; // Diameter [1,4096] pixels; other fields [0,1].
};
struct VanishingPointOptions final {
    std::vector<VanishingPlane> planes;     // <=16; tear-offs may extend to <=32 total.
    std::vector<VanishingTearOff> tearOffs; // <=16, applied in order.
    double focalLength =
        0; // 0 estimates from first plane then falls back to image long side; otherwise [1,100000].
    std::vector<VanishingPaste>
        pastes; // <=32, ordered source-over composites clipped to target plane.
    std::vector<VanishingCloneStroke>
        clones; // <=64, after pastes; each uses its own frozen source snapshot.
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<std::vector<VanishingPlane>>
vanishingPointPlanes(ImageView, const VanishingPointOptions &);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
vanishingPoint(ImageView, const VanishingPointOptions & = {});
} // namespace iiGraphicAdjust
