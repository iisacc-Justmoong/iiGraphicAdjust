#pragma once
#include <iiGraphicAdjust/LensFilter.h>
#include <iiGraphicAdjust/Operation.h>
namespace iiGraphicAdjust::objects {
struct LensCorrection final {
    inline static constexpr OperationInfo info{"optics.lensCorrection","LensCorrection","optics"};
    LensCorrectionOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const { return lensCorrection(image,options); }
};
}
