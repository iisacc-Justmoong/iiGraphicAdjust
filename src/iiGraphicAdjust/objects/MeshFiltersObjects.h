#pragma once
#include <iiGraphicAdjust/MeshFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct Liquify final {
    inline static constexpr OperationInfo info{"geometry.liquify", "Liquify", "geometry"};
    LiquifyOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::liquify(image, options);
    }
};

struct AdaptiveWideAngle final {
    inline static constexpr OperationInfo info{"geometry.adaptiveWideAngle", "AdaptiveWideAngle", "geometry"};
    AdaptiveWideAngleOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::adaptiveWideAngle(image, options);
    }
};

struct PuppetWarp final {
    inline static constexpr OperationInfo info{"geometry.puppetWarp", "PuppetWarp", "geometry"};
    PuppetWarpOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::puppetWarp(image, options);
    }
};

struct PerspectiveWarp final {
    inline static constexpr OperationInfo info{"geometry.perspectiveWarp", "PerspectiveWarp", "geometry"};
    PerspectiveWarpOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::perspectiveWarp(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
