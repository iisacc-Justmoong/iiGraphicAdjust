#pragma once
#include <iiGraphicAdjust/Effects.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct GaussianBlur final {
    inline static constexpr OperationInfo info{"effects.gaussianBlur", "GaussianBlur", "effects"};
    double sigma = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::gaussianBlur(image, sigma);
    }
};

struct Sharpen final {
    inline static constexpr OperationInfo info{"effects.sharpen", "Sharpen", "effects"};
    SharpenOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::sharpen(image, options);
    }
};

struct AddNoise final {
    inline static constexpr OperationInfo info{"effects.addNoise", "AddNoise", "effects"};
    NoiseOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::addNoise(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
