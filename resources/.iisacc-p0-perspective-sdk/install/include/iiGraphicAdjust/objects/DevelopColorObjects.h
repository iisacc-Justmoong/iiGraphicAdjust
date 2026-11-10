#pragma once
#include <iiGraphicAdjust/DevelopColor.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct DevelopColor final {
    inline static constexpr OperationInfo info{"develop.developColor", "DevelopColor", "develop"};
    DevelopColorOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::developColor(image, options);
    }
};

struct Calibrate final {
    inline static constexpr OperationInfo info{"develop.calibrate", "Calibrate", "develop"};
    CalibrationOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::calibrate(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
