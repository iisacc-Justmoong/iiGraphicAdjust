#pragma once
#include <iiGraphicAdjust/DiagnosticFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct PointColorRangePreview final {
    inline static constexpr OperationInfo info{"diagnostic.pointColorRangePreview", "PointColorRangePreview", "diagnostic"};
    PointColorOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::pointColorRangePreview(image, options);
    }
};

struct MaskColorOverlay final {
    inline static constexpr OperationInfo info{"diagnostic.maskColorOverlay", "MaskColorOverlay", "diagnostic"};
    Mask mask;
    MaskOverlayOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::maskColorOverlay(image, mask.view(), options);
    }
};

struct MaskColorOnBwOverlay final {
    inline static constexpr OperationInfo info{"diagnostic.maskColorOnBwOverlay", "MaskColorOnBwOverlay", "diagnostic"};
    Mask mask;
    MaskOverlayOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::maskColorOnBwOverlay(image, mask.view(), options);
    }
};

struct MaskImageOnBlack final {
    inline static constexpr OperationInfo info{"diagnostic.maskImageOnBlack", "MaskImageOnBlack", "diagnostic"};
    Mask mask;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::maskImageOnBlack(image, mask.view());
    }
};

struct MaskImageOnWhite final {
    inline static constexpr OperationInfo info{"diagnostic.maskImageOnWhite", "MaskImageOnWhite", "diagnostic"};
    Mask mask;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::maskImageOnWhite(image, mask.view());
    }
};

struct MaskWhiteOnBlack final {
    inline static constexpr OperationInfo info{"diagnostic.maskWhiteOnBlack", "MaskWhiteOnBlack", "diagnostic"};
    Mask mask;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::maskWhiteOnBlack(image, mask.view());
    }
};

} // namespace iiGraphicAdjust::objects
