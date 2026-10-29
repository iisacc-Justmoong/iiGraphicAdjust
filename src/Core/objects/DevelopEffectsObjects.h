#pragma once
#include <iiGraphicAdjust/DevelopEffects.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct DevelopEffects final {
    inline static constexpr OperationInfo info{"develop.developEffects", "DevelopEffects", "develop"};
    DevelopEffectsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::developEffects(image, options);
    }
};

struct PostCropVignette final {
    inline static constexpr OperationInfo info{"develop.postCropVignette", "PostCropVignette", "develop"};
    PostCropVignetteOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::postCropVignette(image, options);
    }
};

struct FilmGrain final {
    inline static constexpr OperationInfo info{"develop.filmGrain", "FilmGrain", "develop"};
    FilmGrainOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::filmGrain(image, options);
    }
};

struct DevelopDetail final {
    inline static constexpr OperationInfo info{"develop.developDetail", "DevelopDetail", "develop"};
    DevelopDetailOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::developDetail(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
