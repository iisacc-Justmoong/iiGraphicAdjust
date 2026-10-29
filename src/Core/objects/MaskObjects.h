#pragma once
#include <iiGraphicAdjust/Masks.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {
struct LinearMask final {
    inline static constexpr OperationInfo info{"mask.linearMask","LinearMask","mask"};
    LinearMaskOptions options;
    [[nodiscard]] Result<Mask> generate(ImageView image) const { return linearMask(image.width,image.height,options); }
};
struct RadialMask final {
    inline static constexpr OperationInfo info{"mask.radialMask","RadialMask","mask"};
    RadialMaskOptions options;
    [[nodiscard]] Result<Mask> generate(ImageView image) const { return radialMask(image.width,image.height,options); }
};
struct BrushMask final {
    inline static constexpr OperationInfo info{"mask.brushMask","BrushMask","mask"};
    std::vector<BrushStroke> strokes;
    [[nodiscard]] Result<Mask> generate(ImageView image) const { return brushMask(image,strokes); }
};
struct LuminanceRangeMask final {
    inline static constexpr OperationInfo info{"mask.luminanceRangeMask","LuminanceRangeMask","mask"};
    LuminanceRangeOptions options;
    [[nodiscard]] Result<Mask> generate(ImageView image) const { return luminanceRangeMask(image,options); }
};
struct ColorRangeMask final {
    inline static constexpr OperationInfo info{"mask.colorRangeMask","ColorRangeMask","mask"};
    ColorRangeOptions options;
    [[nodiscard]] Result<Mask> generate(ImageView image) const { return colorRangeMask(image,options); }
};
struct CombineMasks final {
    inline static constexpr OperationInfo info{"mask.combineMasks","CombineMasks","mask"};
    Mask operand;
    MaskCombine operation=MaskCombine::Add;
    [[nodiscard]] Result<Mask> apply(MaskView mask) const { return combineMasks(mask,operand.view(),operation); }
};
struct InvertMask final {
    inline static constexpr OperationInfo info{"mask.invertMask","InvertMask","mask"};
    [[nodiscard]] Result<Mask> apply(MaskView mask) const { return invertMask(mask); }
};
struct RefineMask final {
    inline static constexpr OperationInfo info{"mask.refineMask","RefineMask","mask"};
    Mask mask;
    double amount=50;
    [[nodiscard]] Result<Mask> generate(ImageView image) const { return refineMask(image,mask.view(),amount); }
};
} // namespace iiGraphicAdjust::objects
