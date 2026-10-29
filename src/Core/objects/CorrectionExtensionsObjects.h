#pragma once
#include <iiGraphicAdjust/CorrectionExtensions.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct ColorToAlpha final {
    inline static constexpr OperationInfo info{"correction.colorToAlpha", "ColorToAlpha", "correction"};
    ColorToAlphaOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::colorToAlpha(image, options);
    }
};

struct FlatFieldCorrection final {
    inline static constexpr OperationInfo info{"correction.flatFieldCorrection", "FlatFieldCorrection", "correction"};
    Image reference;
    FlatFieldOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::flatFieldCorrection(image, reference.view(), options);
    }
};

struct SolidFill final {
    inline static constexpr OperationInfo info{"correction.solidFill", "SolidFill", "correction"};
    SolidFillOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::solidFill(image, options);
    }
};

struct GradientFill final {
    inline static constexpr OperationInfo info{"correction.gradientFill", "GradientFill", "correction"};
    GradientFillOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::gradientFill(image, options);
    }
};

struct PatternFill final {
    inline static constexpr OperationInfo info{"correction.patternFill", "PatternFill", "correction"};
    Image pattern;
    PatternFillOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::patternFill(image, pattern.view(), options);
    }
};

} // namespace iiGraphicAdjust::objects
