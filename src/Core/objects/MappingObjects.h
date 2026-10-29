#pragma once
#include <iiGraphicAdjust/Mapping.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct GradientMap final {
    inline static constexpr OperationInfo info{"mapping.gradientMap", "GradientMap", "mapping"};
    GradientMapOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::gradientMap(image, options);
    }
};

struct ColorLookup final {
    inline static constexpr OperationInfo info{"mapping.colorLookup", "ColorLookup", "mapping"};
    ColorLut lut;
    ColorLookupOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::colorLookup(image, lut, options);
    }
};

} // namespace iiGraphicAdjust::objects
