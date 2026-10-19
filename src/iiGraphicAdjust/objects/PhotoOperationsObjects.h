#pragma once
#include <iiGraphicAdjust/PhotoOperations.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct Orientation final {
    inline static constexpr OperationInfo info{"geometry.applyOrientation", "Orientation", "geometry"};
    ExifOrientation orientation = ExifOrientation::TopLeft;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::applyOrientation(image, orientation);
    }
};

struct Crop final {
    inline static constexpr OperationInfo info{"geometry.crop", "Crop", "geometry"};
    PixelRect rectangle{};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::crop(image, rectangle);
    }
};

} // namespace iiGraphicAdjust::objects
