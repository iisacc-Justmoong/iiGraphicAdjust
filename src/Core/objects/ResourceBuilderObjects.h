#pragma once
#include <iiGraphicAdjust/DevelopColor.h>
#include <iiGraphicAdjust/LensFilter.h>
#include <iiGraphicAdjust/LightcraftFilters.h>
#include <iiGraphicAdjust/Mapping.h>
#include <iiGraphicAdjust/objects/OwnedResourceObjects.h>
#include <string>

namespace iiGraphicAdjust::objects {
struct IdentityLut final {
    inline static constexpr OperationInfo info{"resource.identityLut","IdentityLut","resource"};
    std::size_t size=2;
    [[nodiscard]] Result<ColorLut> build() const { return identityLut(size); }
};
struct CubeLut final {
    inline static constexpr OperationInfo info{"resource.parseCubeLut","CubeLut","resource"};
    std::string text;
    [[nodiscard]] Result<ColorLut> build() const { return parseCubeLut(text); }
};
struct PointColorSample final {
    inline static constexpr OperationInfo info{"resource.samplePointColor","PointColorSample","resource"};
    Rgba color{.5F,.5F,.5F,1};
    [[nodiscard]] Result<PointColorOptions> sample() const { return samplePointColor(color); }
};
struct LookRecipe final {
    inline static constexpr OperationInfo info{"resource.lightcraftLookOptions","LookRecipe","resource"};
    LightcraftLook look=LightcraftLook::Color;
    double amount=1;
    [[nodiscard]] Result<DevelopOptions> resolve() const { return lightcraftLookOptions(look,amount); }
};
struct PresetRecipe final {
    inline static constexpr OperationInfo info{"resource.lightcraftPresetOptions","PresetRecipe","resource"};
    LightcraftPreset preset=LightcraftPreset::WarmGlow;
    double amount=1;
    [[nodiscard]] Result<DevelopOptions> resolve() const { return lightcraftPresetOptions(preset,amount); }
};
struct GenericLensProfile final {
    inline static constexpr OperationInfo info{"resource.genericLensProfile","GenericLensProfile","resource"};
    double focal35mm=50;
    [[nodiscard]] Result<LensFilterProfile> resolve() const { return genericLensProfile(focal35mm); }
};
struct VanishingPointPlanes final {
    inline static constexpr OperationInfo info{"analysis.vanishingPointPlanes","VanishingPointPlanes","analysis"};
    VanishingPoint operation;
    [[nodiscard]] Result<std::vector<iiGraphicAdjust::VanishingPlane>> analyze(ImageView image) const {
        return operation.planes(image);
    }
};
} // namespace iiGraphicAdjust::objects
