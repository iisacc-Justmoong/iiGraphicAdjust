#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>
#include <optional>

namespace iiGraphicAdjust {
// Coordinates are normalized to image width/height. Size is a radius divided by the long edge.
struct SpotOptions final {
    std::vector<std::array<double, 2>> points{{.5, .5}};
    double size = .02, feather = 50, opacity = 100;
    // Offset uses fractions of width/height; empty requests the reference ring-matching search.
    std::optional<std::array<double, 2>> sourceOffset;
};
struct SpotSourceOptions final {
    std::array<double, 2> target{.5, .5};
    double size = .02;
    // Refresh excludes offsets within one spot radius of this offset.
    std::optional<std::array<double, 2>> avoidOffset;
};
struct SpotSource final {
    std::array<double, 2> offset{};
    double score = 0;
    bool operator==(const SpotSource &) const = default;
};
enum class EyeKind { Red, Pet };
struct EyeOptions final {
    std::array<double, 2> center{.5, .5};
    double radiusX = .02, radiusY = .015;
    double pupilSize = 50, darken = 50;
    // Pet eye only, expressed in pupil radii; validated for both correction APIs.
    std::optional<std::array<double, 2>> catchlight;
};
struct DetectedPupil final {
    std::array<double, 2> center{};
    double radius = 0; // Fraction of the long edge, before pupilSize is applied.
    bool found = false;
    bool operator==(const DetectedPupil &) const = default;
};
struct VisualizeSpotsOptions final {
    double sensitivity = 50;
};
struct DustDetectionOptions final {
    double sensitivity = 50;
    std::size_t maxSpots = 60; // [1,60].
};
struct DustSpot final {
    std::array<double, 2> center{};
    double radius = 0, strength = 0;
    bool operator==(const DustSpot &) const = default;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> cloneSpot(ImageView, const SpotOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> healSpot(ImageView, const SpotOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> removeSpot(ImageView, const SpotOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<std::vector<SpotSource>>
rankSpotSources(ImageView, const SpotSourceOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<DetectedPupil>
detectPupil(ImageView, const EyeOptions & = {}, EyeKind = EyeKind::Red);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> redEye(ImageView, const EyeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> petEye(ImageView, const EyeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
visualizeSpots(ImageView, const VisualizeSpotsOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<std::vector<DustSpot>>
detectDustSpots(ImageView, const DustDetectionOptions & = {});
} // namespace iiGraphicAdjust
