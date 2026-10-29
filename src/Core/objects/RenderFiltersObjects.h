#pragma once
#include <iiGraphicAdjust/RenderFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct Clouds final {
    inline static constexpr OperationInfo info{"render.clouds", "Clouds", "render"};
    CloudsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::clouds(image, options);
    }
};

struct DifferenceClouds final {
    inline static constexpr OperationInfo info{"render.differenceClouds", "DifferenceClouds", "render"};
    CloudsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::differenceClouds(image, options);
    }
};

struct Fibers final {
    inline static constexpr OperationInfo info{"render.fibers", "Fibers", "render"};
    FibersOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::fibers(image, options);
    }
};

struct LensFlare final {
    inline static constexpr OperationInfo info{"render.lensFlare", "LensFlare", "render"};
    LensFlareOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lensFlare(image, options);
    }
};

struct LightingEffects final {
    inline static constexpr OperationInfo info{"render.lightingEffects", "LightingEffects", "render"};
    LightingEffectsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightingEffects(image, options);
    }
};

struct Relight final {
    inline static constexpr OperationInfo info{"render.relight", "Relight", "render"};
    RelightOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::relight(image, options);
    }
};

struct Flame final {
    inline static constexpr OperationInfo info{"render.flame", "Flame", "render"};
    FlameOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::flame(image, options);
    }
};

struct Tree final {
    inline static constexpr OperationInfo info{"render.tree", "Tree", "render"};
    TreeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::tree(image, options);
    }
};

struct PictureFrame final {
    inline static constexpr OperationInfo info{"render.pictureFrame", "PictureFrame", "render"};
    PictureFrameOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::pictureFrame(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
