#pragma once
#include <iiGraphicAdjust/Tone.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct BrightnessContrast final {
    inline static constexpr OperationInfo info{"tone.brightnessContrast", "BrightnessContrast", "tone"};
    BrightnessContrastOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::brightnessContrast(image, options);
    }
};

struct Levels final {
    inline static constexpr OperationInfo info{"tone.levels", "Levels", "tone"};
    LevelsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::levels(image, options);
    }
};

struct Curves final {
    inline static constexpr OperationInfo info{"tone.curves", "Curves", "tone"};
    CurvesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::curves(image, options);
    }
};

struct Exposure final {
    inline static constexpr OperationInfo info{"tone.exposure", "Exposure", "tone"};
    ExposureOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::exposure(image, options);
    }
};

struct ShadowsHighlights final {
    inline static constexpr OperationInfo info{"tone.shadowsHighlights", "ShadowsHighlights", "tone"};
    ShadowsHighlightsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::shadowsHighlights(image, options);
    }
};

struct Invert final {
    inline static constexpr OperationInfo info{"tone.invert", "Invert", "tone"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::invert(image);
    }
};

struct Posterize final {
    inline static constexpr OperationInfo info{"tone.posterize", "Posterize", "tone"};
    unsigned levelCount = 8;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::posterize(image, levelCount);
    }
};

struct Threshold final {
    inline static constexpr OperationInfo info{"tone.threshold", "Threshold", "tone"};
    double level = .5;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::threshold(image, level);
    }
};

struct Equalize final {
    inline static constexpr OperationInfo info{"tone.equalize", "Equalize", "tone"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::equalize(image);
    }
};

struct AutoLevels final {
    inline static constexpr OperationInfo info{"tone.autoLevels", "AutoLevels", "tone"};
    AutoToneOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::autoLevels(image, options);
    }
};

struct AutoContrast final {
    inline static constexpr OperationInfo info{"tone.autoContrast", "AutoContrast", "tone"};
    AutoToneOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::autoContrast(image, options);
    }
};

struct AutoColor final {
    inline static constexpr OperationInfo info{"tone.autoColor", "AutoColor", "tone"};
    AutoToneOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::autoColor(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
