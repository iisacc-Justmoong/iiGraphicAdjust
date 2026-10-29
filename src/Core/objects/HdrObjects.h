#pragma once
#include <iiGraphicAdjust/Hdr.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct HdrToning final {
    inline static constexpr OperationInfo info{"hdr.hdrToning", "HdrToning", "hdr"};
    HdrToningOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::hdrToning(image, options);
    }
    [[nodiscard]] Result<Image> apply(HdrImageView image) const {
        return iiGraphicAdjust::hdrToning(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
