#pragma once
#include <iiGraphicAdjust/Blend.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct Blend final {
    inline static constexpr OperationInfo info{"composite.blend", "Blend", "composite"};
    Image source;
    BlendOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::blend(image, source.view(), options);
    }
};

} // namespace iiGraphicAdjust::objects
