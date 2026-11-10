#pragma once
#include <iiGraphicAdjust/UtilityFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct Average final {
    inline static constexpr OperationInfo info{"utility.average", "Average", "utility"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::average(image);
    }
};

struct FilterAddNoise final {
    inline static constexpr OperationInfo info{"utility.filterAddNoise", "FilterAddNoise", "utility"};
    FilterNoiseOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::filterAddNoise(image, options);
    }
};

struct Median final {
    inline static constexpr OperationInfo info{"utility.median", "Median", "utility"};
    double radius = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::median(image, radius);
    }
};

struct DustAndScratches final {
    inline static constexpr OperationInfo info{"utility.dustAndScratches", "DustAndScratches", "utility"};
    DustAndScratchesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::dustAndScratches(image, options);
    }
};

struct Minimum final {
    inline static constexpr OperationInfo info{"utility.minimum", "Minimum", "utility"};
    MorphologyOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::minimum(image, options);
    }
};

struct Maximum final {
    inline static constexpr OperationInfo info{"utility.maximum", "Maximum", "utility"};
    MorphologyOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::maximum(image, options);
    }
};

struct ReduceNoise final {
    inline static constexpr OperationInfo info{"utility.reduceNoise", "ReduceNoise", "utility"};
    ReduceNoiseOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::reduceNoise(image, options);
    }
};

struct CustomFilter final {
    inline static constexpr OperationInfo info{"utility.customFilter", "CustomFilter", "utility"};
    CustomFilterOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::customFilter(image, options);
    }
};

struct HsbHsl final {
    inline static constexpr OperationInfo info{"utility.hsbHsl", "HsbHsl", "utility"};
    FilterColorModel input = FilterColorModel::Rgb, output = FilterColorModel::Hsb;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::hsbHsl(image, input, output);
    }
};

struct DeInterlace final {
    inline static constexpr OperationInfo info{"utility.deInterlace", "DeInterlace", "utility"};
    bool eliminateEven = false, interpolate = true;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::deInterlace(image, eliminateEven, interpolate);
    }
};

struct NtscColors final {
    inline static constexpr OperationInfo info{"utility.ntscColors", "NtscColors", "utility"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::ntscColors(image);
    }
};

struct GuidedBlur final {
    inline static constexpr OperationInfo info{"utility.guidedBlur", "GuidedBlur", "utility"};
    GuidedBlurOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::guidedBlur(image, options);
    }
};

struct CrossGuidedBlur final {
    inline static constexpr OperationInfo info{"utility.crossGuidedBlur", "CrossGuidedBlur", "utility"};
    Image guide;
    GuidedBlurOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::crossGuidedBlur(image, guide.view(), options);
    }
};

} // namespace iiGraphicAdjust::objects
