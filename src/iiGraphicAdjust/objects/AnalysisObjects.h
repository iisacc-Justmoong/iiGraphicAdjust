#pragma once
#include <iiGraphicAdjust/DevelopTone.h>
#include <iiGraphicAdjust/DevelopColor.h>
#include <iiGraphicAdjust/PhotoOperations.h>
#include <iiGraphicAdjust/RetouchFilters.h>
#include <iiGraphicAdjust/LensFilter.h>
#include <iiGraphicAdjust/Optics.h>
#include <iiGraphicAdjust/MeshFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {
struct AutoWhiteBalance final {
    inline static constexpr OperationInfo info{"develop.autoWhiteBalance","AutoWhiteBalance","develop"};
    WhiteBalanceOptions options;
    [[nodiscard]] Result<WhiteBalanceOptions> analyze(ImageView image) const { return autoWhiteBalance(image,options); }
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        auto estimated=analyze(image);
        if(!estimated) return std::unexpected(estimated.error());
        return whiteBalance(image,*estimated);
    }
};
struct SampledWhiteBalance final {
    inline static constexpr OperationInfo info{"develop.sampledWhiteBalance","SampledWhiteBalance","develop"};
    Rgba sample{.5F,.5F,.5F,1};
    WhiteBalanceOptions options;
    [[nodiscard]] Result<WhiteBalanceOptions> estimate() const { return sampledWhiteBalance(sample,options); }
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        auto estimated=estimate();
        if(!estimated) return std::unexpected(estimated.error());
        return whiteBalance(image,*estimated);
    }
};
struct PresetWhiteBalance final {
    inline static constexpr OperationInfo info{"develop.whiteBalancePreset","PresetWhiteBalance","develop"};
    WhiteBalancePreset preset=WhiteBalancePreset::Daylight;
    double asShotTemperature=6500, asShotTint=0;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        auto options=whiteBalancePreset(preset);
        if(!options) return std::unexpected(options.error());
        options->asShotTemperature=asShotTemperature; options->asShotTint=asShotTint;
        return whiteBalance(image,*options);
    }
};
struct AutoDevelopLight final {
    inline static constexpr OperationInfo info{"develop.autoDevelopLight","AutoDevelopLight","develop"};
    [[nodiscard]] Result<DevelopLightOptions> analyze(ImageView image) const { return autoDevelopLight(image); }
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        auto options=analyze(image);
        if(!options) return std::unexpected(options.error());
        return developLight(image,*options);
    }
};
struct AutoBlackAndWhiteMix final {
    inline static constexpr OperationInfo info{"develop.autoBlackAndWhiteMix","AutoBlackAndWhiteMix","develop"};
    [[nodiscard]] Result<std::array<double,8>> analyze(ImageView image) const { return autoBlackAndWhiteMix(image); }
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        auto mix=analyze(image);
        if(!mix) return std::unexpected(mix.error());
        DevelopColorOptions options; options.blackAndWhite=true; options.blackAndWhiteMix=*mix;
        return developColor(image,options);
    }
};
struct Histogram final {
    inline static constexpr OperationInfo info{"analysis.imageHistogram","Histogram","analysis"};
    HistogramOptions options;
    [[nodiscard]] Result<ImageHistogram> analyze(ImageView image) const { return imageHistogram(image,options); }
};
struct ChromaticAberrationEstimate final {
    inline static constexpr OperationInfo info{"analysis.estimateChromaticAberration","ChromaticAberrationEstimate","analysis"};
    [[nodiscard]] Result<std::array<double,2>> analyze(ImageView image) const { return estimateChromaticAberration(image); }
};
struct RankSpotSources final {
    inline static constexpr OperationInfo info{"analysis.rankSpotSources","RankSpotSources","analysis"};
    SpotSourceOptions options;
    [[nodiscard]] Result<std::vector<SpotSource>> analyze(ImageView image) const { return rankSpotSources(image,options); }
};
struct PupilDetector final {
    inline static constexpr OperationInfo info{"analysis.detectPupil","PupilDetector","analysis"};
    EyeOptions options;
    EyeKind kind=EyeKind::Red;
    [[nodiscard]] Result<DetectedPupil> analyze(ImageView image) const { return detectPupil(image,options,kind); }
};
struct DustDetector final {
    inline static constexpr OperationInfo info{"analysis.detectDustSpots","DustDetector","analysis"};
    DustDetectionOptions options;
    [[nodiscard]] Result<std::vector<DustSpot>> analyze(ImageView image) const { return detectDustSpots(image,options); }
};
struct AutoLensScale final {
    inline static constexpr OperationInfo info{"analysis.autoLensScale","AutoLensScale","analysis"};
    LensCorrectionOptions options;
    [[nodiscard]] Result<double> analyze(ImageView image) const { return autoLensScale(image,options); }
};
struct AdaptiveWideAngleGeometry final {
    inline static constexpr OperationInfo info{"analysis.solveAdaptiveWideAngle","AdaptiveWideAngleGeometry","analysis"};
    AdaptiveWideAngleOptions options;
    [[nodiscard]] Result<MeshGeometry> analyze(ImageView image) const { return solveAdaptiveWideAngle(image,options); }
};
struct PuppetWarpGeometry final {
    inline static constexpr OperationInfo info{"analysis.solvePuppetWarp","PuppetWarpGeometry","analysis"};
    PuppetWarpOptions options;
    [[nodiscard]] Result<MeshGeometry> analyze(ImageView image) const { return solvePuppetWarp(image,options); }
};
struct PerspectiveWarpPoint final {
    inline static constexpr OperationInfo info{"analysis.mapPerspectiveWarpPoint","PerspectiveWarpPoint","analysis"};
    PerspectiveWarpOptions options;
    [[nodiscard]] Result<MeshPoint> map(MeshPoint point) const { return mapPerspectiveWarpPoint(point,options); }
};
} // namespace iiGraphicAdjust::objects
