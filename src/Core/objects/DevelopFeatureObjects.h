#pragma once
#include <iiGraphicAdjust/DevelopColor.h>
#include <iiGraphicAdjust/DevelopEffects.h>
#include <iiGraphicAdjust/Optics.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {
struct Texture final {
    inline static constexpr OperationInfo info{"develop.texture","Texture","develop"};
    double amount=0;
    [[nodiscard]] Result<Image> apply(ImageView image) const { return developEffects(image,{amount,0,0}); }
};
struct Clarity final {
    inline static constexpr OperationInfo info{"develop.clarity","Clarity","develop"};
    double amount=0;
    [[nodiscard]] Result<Image> apply(ImageView image) const { return developEffects(image,{0,amount,0}); }
};
struct Dehaze final {
    inline static constexpr OperationInfo info{"develop.dehaze","Dehaze","develop"};
    double amount=0;
    [[nodiscard]] Result<Image> apply(ImageView image) const { return developEffects(image,{0,0,amount}); }
};
struct ColorMixer final {
    inline static constexpr OperationInfo info{"develop.colorMixer","ColorMixer","develop"};
    std::array<MixerAdjustment,8> bands{};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        DevelopColorOptions options; options.mixer=bands; return developColor(image,options);
    }
};
struct PointColor final {
    inline static constexpr OperationInfo info{"develop.pointColor","PointColor","develop"};
    std::vector<PointColorOptions> samples;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> apply(ImageView) const;
};
struct ColorGrading final {
    inline static constexpr OperationInfo info{"develop.colorGrading","ColorGrading","develop"};
    ColorGradingOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        DevelopColorOptions color; color.grading=options; return developColor(image,color);
    }
};
struct DevelopBlackAndWhite final {
    inline static constexpr OperationInfo info{"develop.blackAndWhiteMix","DevelopBlackAndWhite","develop"};
    std::array<double,8> bands{};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        DevelopColorOptions options; options.blackAndWhite=true; options.blackAndWhiteMix=bands;
        return developColor(image,options);
    }
};
struct DevelopVibrance final {
    inline static constexpr OperationInfo info{"develop.vibrance","DevelopVibrance","develop"};
    double vibrance=0, saturation=0;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        DevelopColorOptions options; options.vibrance=vibrance; options.saturation=saturation;
        return developColor(image,options);
    }
};
struct Sharpening final {
    inline static constexpr OperationInfo info{"develop.sharpening","Sharpening","develop"};
    double amount=0, radius=1, detail=25, masking=0;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        DevelopDetailOptions options;
        options.sharpenAmount=amount; options.sharpenRadius=radius;
        options.sharpenDetail=detail; options.sharpenMasking=masking;
        return developDetail(image,options);
    }
};
struct LuminanceNoiseReduction final {
    inline static constexpr OperationInfo info{"develop.luminanceNoiseReduction","LuminanceNoiseReduction","develop"};
    double amount=0, detail=50, contrast=0;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        DevelopDetailOptions options;
        options.luminanceNoise=amount; options.luminanceDetail=detail; options.luminanceContrast=contrast;
        return developDetail(image,options);
    }
};
struct ColorNoiseReduction final {
    inline static constexpr OperationInfo info{"develop.colorNoiseReduction","ColorNoiseReduction","develop"};
    double amount=0, detail=50, smoothness=50;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        DevelopDetailOptions options;
        options.colorNoise=amount; options.colorDetail=detail; options.colorSmoothness=smoothness;
        return developDetail(image,options);
    }
};
struct Defringe final {
    inline static constexpr OperationInfo info{"optics.defringe","Defringe","optics"};
    double purple=0, green=0, purpleLow=30, purpleHigh=70, greenLow=40, greenHigh=60;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        OpticsOptions options; options.purpleDefringe=purple; options.greenDefringe=green;
        options.purpleHueLow=purpleLow; options.purpleHueHigh=purpleHigh;
        options.greenHueLow=greenLow; options.greenHueHigh=greenHigh;
        return correctOptics(image,options);
    }
};
} // namespace iiGraphicAdjust::objects
