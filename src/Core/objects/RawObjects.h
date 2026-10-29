#pragma once
#include <iiGraphicAdjust/RawProcessing.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {
struct RawDevelopment final {
    inline static constexpr OperationInfo info{"raw.developRaw","RawDevelopment","raw"};
    RawDevelopmentOptions options;
    [[nodiscard]] Result<LinearRgbImage> apply(BayerImageView image) const { return developRaw(image,options); }
};
struct RawRender final {
    inline static constexpr OperationInfo info{"raw.renderRaw","RawRender","raw"};
    RawDevelopmentOptions options;
    HdrToningOptions toning;
    [[nodiscard]] Result<Image> apply(BayerImageView image) const { return renderRaw(image,options,toning); }
};
} // namespace iiGraphicAdjust::objects
