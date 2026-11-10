#pragma once
#include <iiGraphicAdjust/BlurFilters.h>
#include <iiGraphicAdjust/DiagnosticFilters.h>
#include <iiGraphicAdjust/MeshFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

// The free-function options borrow views. These long-lived object parameters
// instead own every referenced buffer and reconstruct views for each call.
struct LensBlurParameters final {
    double radius=15;
    unsigned blades=6;
    double curvature=0, rotation=0;
    LensDepthSource depth=LensDepthSource::None;
    double focalDistance=0;
    bool invertDepth=false;
    double brightness=0, threshold=255, noise=0;
    NoiseDistribution distribution=NoiseDistribution::Uniform;
    bool monochromatic=true;
    std::uint32_t seed=0;
    std::optional<Image> depthMap;
};
struct LensBlur final {
    inline static constexpr OperationInfo info{"blur.lensBlur","LensBlur","blur"};
    LensBlurParameters options;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> apply(ImageView) const;
};
struct GamutWarningParameters final {
    DiagnosticColorSpace proofSpace=DiagnosticColorSpace::Srgb;
    std::vector<std::array<double,3>> linearRec2020;
};
struct DestinationGamutWarning final {
    inline static constexpr OperationInfo info{"diagnostic.destinationGamutWarning","DestinationGamutWarning","diagnostic"};
    GamutWarningParameters options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return destinationGamutWarning(image,{options.proofSpace,options.linearRec2020});
    }
};
struct DisplayGamutWarning final {
    inline static constexpr OperationInfo info{"diagnostic.displayGamutWarning","DisplayGamutWarning","diagnostic"};
    GamutWarningParameters options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return displayGamutWarning(image,{options.proofSpace,options.linearRec2020});
    }
};
struct OwnedVanishingPaste final {
    Image image;
    std::size_t plane=0;
    MeshPoint at{0,0};
    double width=1, opacity=1;
};
struct VanishingPointParameters final {
    std::vector<VanishingPlane> planes;
    std::vector<VanishingTearOff> tearOffs;
    double focalLength=0;
    std::vector<OwnedVanishingPaste> pastes;
    std::vector<VanishingCloneStroke> clones;
};
struct VanishingPoint final {
    inline static constexpr OperationInfo info{"geometry.vanishingPoint","VanishingPoint","geometry"};
    VanishingPointParameters options;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> apply(ImageView) const;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<std::vector<VanishingPlane>> planes(ImageView) const;
};
struct OwnedLocalAdjustment final {
    Mask mask;
    LocalAdjustmentOptions options;
};
struct ApplyLocalAdjustments final {
    inline static constexpr OperationInfo info{"local.applyLocalAdjustments","ApplyLocalAdjustments","local"};
    std::vector<OwnedLocalAdjustment> adjustments;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> apply(ImageView) const;
};
} // namespace iiGraphicAdjust::objects
