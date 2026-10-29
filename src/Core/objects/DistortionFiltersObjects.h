#pragma once
#include <iiGraphicAdjust/DistortionFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct Offset final {
    inline static constexpr OperationInfo info{"distortion.offset", "Offset", "distortion"};
    std::int32_t horizontal = 0, vertical = 0;
    FilterEdge edge = FilterEdge::Wrap;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::offset(image, horizontal, vertical, edge);
    }
};

struct Twirl final {
    inline static constexpr OperationInfo info{"distortion.twirl", "Twirl", "distortion"};
    double angle = 50;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::twirl(image, angle);
    }
};

struct Pinch final {
    inline static constexpr OperationInfo info{"distortion.pinch", "Pinch", "distortion"};
    double amount = 50;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::pinch(image, amount);
    }
};

struct Spherize final {
    inline static constexpr OperationInfo info{"distortion.spherize", "Spherize", "distortion"};
    SpherizeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::spherize(image, options);
    }
};

struct Wave final {
    inline static constexpr OperationInfo info{"distortion.wave", "Wave", "distortion"};
    WaveOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::wave(image, options);
    }
};

struct Ripple final {
    inline static constexpr OperationInfo info{"distortion.ripple", "Ripple", "distortion"};
    RippleOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::ripple(image, options);
    }
};

struct PolarCoordinates final {
    inline static constexpr OperationInfo info{"distortion.polarCoordinates", "PolarCoordinates", "distortion"};
    PolarMode mode = PolarMode::RectangularToPolar;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::polarCoordinates(image, mode);
    }
};

struct Displace final {
    inline static constexpr OperationInfo info{"distortion.displace", "Displace", "distortion"};
    Image map;
    DisplaceOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::displace(image, map.view(), options);
    }
};

struct Shear final {
    inline static constexpr OperationInfo info{"distortion.shear", "Shear", "distortion"};
    std::vector<ShearPoint> points{{0,0},{1,0}};
    FilterEdge edge = FilterEdge::Wrap;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::shear(image, points, edge);
    }
};

struct ZigZag final {
    inline static constexpr OperationInfo info{"distortion.zigZag", "ZigZag", "distortion"};
    ZigZagOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::zigZag(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
