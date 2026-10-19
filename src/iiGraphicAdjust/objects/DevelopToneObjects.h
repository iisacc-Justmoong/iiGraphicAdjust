#pragma once
#include <iiGraphicAdjust/DevelopTone.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct WhiteBalance final {
    inline static constexpr OperationInfo info{"develop.whiteBalance", "WhiteBalance", "develop"};
    WhiteBalanceOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::whiteBalance(image, options);
    }
};

struct DevelopLight final {
    inline static constexpr OperationInfo info{"develop.developLight", "DevelopLight", "develop"};
    DevelopLightOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::developLight(image, options);
    }
};

struct DevelopCurve final {
    inline static constexpr OperationInfo info{"develop.developCurve", "DevelopCurve", "develop"};
    DevelopCurveOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::developCurve(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
