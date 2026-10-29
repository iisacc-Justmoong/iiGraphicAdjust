#pragma once
#include <iiGraphicAdjust/LocalCorrections.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct ApplyMaskedCorrection final {
    inline static constexpr OperationInfo info{"local.applyMaskedCorrection", "ApplyMaskedCorrection", "local"};
    Image corrected;
    Mask mask;
    MaskedCorrectionOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::applyMaskedCorrection(image, corrected.view(), mask.view(), options);
    }
};

} // namespace iiGraphicAdjust::objects
