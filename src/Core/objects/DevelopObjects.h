#pragma once
#include <iiGraphicAdjust/Develop.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct Develop final {
    inline static constexpr OperationInfo info{"develop.develop", "Develop", "develop"};
    DevelopOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::develop(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
