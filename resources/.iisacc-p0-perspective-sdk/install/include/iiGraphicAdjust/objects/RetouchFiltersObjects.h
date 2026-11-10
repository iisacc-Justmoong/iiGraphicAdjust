#pragma once
#include <iiGraphicAdjust/RetouchFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct CloneSpot final {
    inline static constexpr OperationInfo info{"retouch.cloneSpot", "CloneSpot", "retouch"};
    SpotOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::cloneSpot(image, options);
    }
};

struct HealSpot final {
    inline static constexpr OperationInfo info{"retouch.healSpot", "HealSpot", "retouch"};
    SpotOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::healSpot(image, options);
    }
};

struct RemoveSpot final {
    inline static constexpr OperationInfo info{"retouch.removeSpot", "RemoveSpot", "retouch"};
    SpotOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::removeSpot(image, options);
    }
};

struct RedEye final {
    inline static constexpr OperationInfo info{"retouch.redEye", "RedEye", "retouch"};
    EyeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::redEye(image, options);
    }
};

struct PetEye final {
    inline static constexpr OperationInfo info{"retouch.petEye", "PetEye", "retouch"};
    EyeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::petEye(image, options);
    }
};

struct VisualizeSpots final {
    inline static constexpr OperationInfo info{"retouch.visualizeSpots", "VisualizeSpots", "retouch"};
    VisualizeSpotsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::visualizeSpots(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
