#pragma once
#include <iiGraphicAdjust/Color.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct HueSaturation final {
    inline static constexpr OperationInfo info{"color.hueSaturation", "HueSaturation", "color"};
    HueSaturationOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::hueSaturation(image, options);
    }
};

struct Vibrance final {
    inline static constexpr OperationInfo info{"color.vibrance", "Vibrance", "color"};
    VibranceOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::vibrance(image, options);
    }
};

struct ColorBalance final {
    inline static constexpr OperationInfo info{"color.colorBalance", "ColorBalance", "color"};
    ColorBalanceOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::colorBalance(image, options);
    }
};

struct BlackAndWhite final {
    inline static constexpr OperationInfo info{"color.blackAndWhite", "BlackAndWhite", "color"};
    BlackWhiteOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::blackAndWhite(image, options);
    }
};

struct PhotoFilter final {
    inline static constexpr OperationInfo info{"color.photoFilter", "PhotoFilter", "color"};
    PhotoFilterOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::photoFilter(image, options);
    }
};

struct ChannelMixer final {
    inline static constexpr OperationInfo info{"color.channelMixer", "ChannelMixer", "color"};
    ChannelMixerOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::channelMixer(image, options);
    }
};

struct SelectiveColor final {
    inline static constexpr OperationInfo info{"color.selectiveColor", "SelectiveColor", "color"};
    SelectiveColorOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::selectiveColor(image, options);
    }
};

struct Desaturate final {
    inline static constexpr OperationInfo info{"color.desaturate", "Desaturate", "color"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::desaturate(image);
    }
};

struct ReplaceColor final {
    inline static constexpr OperationInfo info{"color.replaceColor", "ReplaceColor", "color"};
    ReplaceColorOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::replaceColor(image, options);
    }
};

struct MatchColor final {
    inline static constexpr OperationInfo info{"color.matchColor", "MatchColor", "color"};
    Image reference;
    MatchColorOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::matchColor(image, reference.view(), options);
    }
};

} // namespace iiGraphicAdjust::objects
