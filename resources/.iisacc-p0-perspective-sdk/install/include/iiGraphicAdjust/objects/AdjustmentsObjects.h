#pragma once
#include <iiGraphicAdjust/Adjustments.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct Adjust final {
    inline static constexpr OperationInfo info{"adjustment.adjust", "Adjust", "adjustment"};
    ColorAdjustment options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::adjust(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
