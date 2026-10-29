#pragma once
#include <iiGraphicAdjust/BlurFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct FilterGaussianBlur final {
    inline static constexpr OperationInfo info{"blur.filterGaussianBlur", "FilterGaussianBlur", "blur"};
    double sigma = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::filterGaussianBlur(image, sigma);
    }
};

struct BoxBlur final {
    inline static constexpr OperationInfo info{"blur.boxBlur", "BoxBlur", "blur"};
    double radius = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::boxBlur(image, radius);
    }
};

struct Blur final {
    inline static constexpr OperationInfo info{"blur.blur", "Blur", "blur"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::blur(image);
    }
};

struct BlurMore final {
    inline static constexpr OperationInfo info{"blur.blurMore", "BlurMore", "blur"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::blurMore(image);
    }
};

struct MotionBlur final {
    inline static constexpr OperationInfo info{"blur.motionBlur", "MotionBlur", "blur"};
    MotionBlurOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::motionBlur(image, options);
    }
};

struct RadialBlur final {
    inline static constexpr OperationInfo info{"blur.radialBlur", "RadialBlur", "blur"};
    RadialBlurOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::radialBlur(image, options);
    }
};

struct SurfaceBlur final {
    inline static constexpr OperationInfo info{"blur.surfaceBlur", "SurfaceBlur", "blur"};
    SurfaceBlurOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::surfaceBlur(image, options);
    }
};

struct UnsharpMask final {
    inline static constexpr OperationInfo info{"blur.unsharpMask", "UnsharpMask", "blur"};
    UnsharpMaskOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::unsharpMask(image, options);
    }
};

struct FilterSharpen final {
    inline static constexpr OperationInfo info{"blur.filterSharpen", "FilterSharpen", "blur"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::filterSharpen(image);
    }
};

struct SharpenMore final {
    inline static constexpr OperationInfo info{"blur.sharpenMore", "SharpenMore", "blur"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::sharpenMore(image);
    }
};

struct SharpenEdges final {
    inline static constexpr OperationInfo info{"blur.sharpenEdges", "SharpenEdges", "blur"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::sharpenEdges(image);
    }
};

struct SmartSharpen final {
    inline static constexpr OperationInfo info{"blur.smartSharpen", "SmartSharpen", "blur"};
    SmartSharpenOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::smartSharpen(image, options);
    }
};

struct HighPass final {
    inline static constexpr OperationInfo info{"blur.highPass", "HighPass", "blur"};
    double radius = 10;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::highPass(image, radius);
    }
};

struct Despeckle final {
    inline static constexpr OperationInfo info{"blur.despeckle", "Despeckle", "blur"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::despeckle(image);
    }
};

struct SmartBlur final {
    inline static constexpr OperationInfo info{"blur.smartBlur", "SmartBlur", "blur"};
    SmartBlurOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::smartBlur(image, options);
    }
};

struct ShapeBlur final {
    inline static constexpr OperationInfo info{"blur.shapeBlur", "ShapeBlur", "blur"};
    ShapeBlurOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::shapeBlur(image, options);
    }
};

struct TiltShift final {
    inline static constexpr OperationInfo info{"blur.tiltShift", "TiltShift", "blur"};
    TiltShiftOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::tiltShift(image, options);
    }
};

struct IrisBlur final {
    inline static constexpr OperationInfo info{"blur.irisBlur", "IrisBlur", "blur"};
    std::vector<IrisBlurPin> pins{IrisBlurPin{}};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::irisBlur(image, pins);
    }
};

struct FieldBlur final {
    inline static constexpr OperationInfo info{"blur.fieldBlur", "FieldBlur", "blur"};
    std::vector<FieldBlurPin> pins{FieldBlurPin{}};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::fieldBlur(image, pins);
    }
};

struct SpinBlur final {
    inline static constexpr OperationInfo info{"blur.spinBlur", "SpinBlur", "blur"};
    std::vector<SpinBlurPin> pins{SpinBlurPin{}};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::spinBlur(image, pins);
    }
};

struct PathBlur final {
    inline static constexpr OperationInfo info{"blur.pathBlur", "PathBlur", "blur"};
    std::vector<BlurPath> paths{BlurPath{}};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::pathBlur(image, paths);
    }
};

} // namespace iiGraphicAdjust::objects
