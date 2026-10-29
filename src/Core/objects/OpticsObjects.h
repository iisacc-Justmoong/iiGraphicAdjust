#pragma once
#include <iiGraphicAdjust/Optics.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct CorrectOptics final {
    inline static constexpr OperationInfo info{"optics.correctOptics", "CorrectOptics", "optics"};
    OpticsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::correctOptics(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
