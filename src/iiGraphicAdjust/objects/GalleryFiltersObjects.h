#pragma once
#include <iiGraphicAdjust/GalleryFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct ColoredPencil final {
    inline static constexpr OperationInfo info{"gallery.coloredPencil", "ColoredPencil", "gallery"};
    GalleryColoredPencilOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::coloredPencil(image, options);
    }
};

struct Cutout final {
    inline static constexpr OperationInfo info{"gallery.cutout", "Cutout", "gallery"};
    GalleryCutoutOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::cutout(image, options);
    }
};

struct DryBrush final {
    inline static constexpr OperationInfo info{"gallery.dryBrush", "DryBrush", "gallery"};
    GalleryDryBrushOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::dryBrush(image, options);
    }
};

struct GalleryFilmGrain final {
    inline static constexpr OperationInfo info{"gallery.galleryFilmGrain", "GalleryFilmGrain", "gallery"};
    GalleryFilmGrainOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::galleryFilmGrain(image, options);
    }
};

struct Fresco final {
    inline static constexpr OperationInfo info{"gallery.fresco", "Fresco", "gallery"};
    GalleryFrescoOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::fresco(image, options);
    }
};

struct NeonGlow final {
    inline static constexpr OperationInfo info{"gallery.neonGlow", "NeonGlow", "gallery"};
    GalleryNeonGlowOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::neonGlow(image, options);
    }
};

struct PaintDaubs final {
    inline static constexpr OperationInfo info{"gallery.paintDaubs", "PaintDaubs", "gallery"};
    GalleryPaintDaubsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::paintDaubs(image, options);
    }
};

struct PaletteKnife final {
    inline static constexpr OperationInfo info{"gallery.paletteKnife", "PaletteKnife", "gallery"};
    GalleryPaletteKnifeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::paletteKnife(image, options);
    }
};

struct PlasticWrap final {
    inline static constexpr OperationInfo info{"gallery.plasticWrap", "PlasticWrap", "gallery"};
    GalleryPlasticWrapOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::plasticWrap(image, options);
    }
};

struct PosterEdges final {
    inline static constexpr OperationInfo info{"gallery.posterEdges", "PosterEdges", "gallery"};
    GalleryPosterEdgesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::posterEdges(image, options);
    }
};

struct RoughPastels final {
    inline static constexpr OperationInfo info{"gallery.roughPastels", "RoughPastels", "gallery"};
    GalleryRoughPastelsOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::roughPastels(image, options);
    }
};

struct SmudgeStick final {
    inline static constexpr OperationInfo info{"gallery.smudgeStick", "SmudgeStick", "gallery"};
    GallerySmudgeStickOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::smudgeStick(image, options);
    }
};

struct Sponge final {
    inline static constexpr OperationInfo info{"gallery.sponge", "Sponge", "gallery"};
    GallerySpongeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::sponge(image, options);
    }
};

struct Underpainting final {
    inline static constexpr OperationInfo info{"gallery.underpainting", "Underpainting", "gallery"};
    GalleryUnderpaintingOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::underpainting(image, options);
    }
};

struct Watercolor final {
    inline static constexpr OperationInfo info{"gallery.watercolor", "Watercolor", "gallery"};
    GalleryWatercolorOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::watercolor(image, options);
    }
};

struct AccentedEdges final {
    inline static constexpr OperationInfo info{"gallery.accentedEdges", "AccentedEdges", "gallery"};
    GalleryAccentedEdgesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::accentedEdges(image, options);
    }
};

struct AngledStrokes final {
    inline static constexpr OperationInfo info{"gallery.angledStrokes", "AngledStrokes", "gallery"};
    GalleryAngledStrokesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::angledStrokes(image, options);
    }
};

struct Crosshatch final {
    inline static constexpr OperationInfo info{"gallery.crosshatch", "Crosshatch", "gallery"};
    GalleryCrosshatchOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::crosshatch(image, options);
    }
};

struct DarkStrokes final {
    inline static constexpr OperationInfo info{"gallery.darkStrokes", "DarkStrokes", "gallery"};
    GalleryDarkStrokesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::darkStrokes(image, options);
    }
};

struct InkOutlines final {
    inline static constexpr OperationInfo info{"gallery.inkOutlines", "InkOutlines", "gallery"};
    GalleryInkOutlinesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::inkOutlines(image, options);
    }
};

struct Spatter final {
    inline static constexpr OperationInfo info{"gallery.spatter", "Spatter", "gallery"};
    GallerySpatterOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::spatter(image, options);
    }
};

struct SprayedStrokes final {
    inline static constexpr OperationInfo info{"gallery.sprayedStrokes", "SprayedStrokes", "gallery"};
    GallerySprayedStrokesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::sprayedStrokes(image, options);
    }
};

struct SumiE final {
    inline static constexpr OperationInfo info{"gallery.sumiE", "SumiE", "gallery"};
    GallerySumiEOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::sumiE(image, options);
    }
};

struct DiffuseGlow final {
    inline static constexpr OperationInfo info{"gallery.diffuseGlow", "DiffuseGlow", "gallery"};
    GalleryDiffuseGlowOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::diffuseGlow(image, options);
    }
};

struct Glass final {
    inline static constexpr OperationInfo info{"gallery.glass", "Glass", "gallery"};
    GalleryGlassOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::glass(image, options);
    }
};

struct OceanRipple final {
    inline static constexpr OperationInfo info{"gallery.oceanRipple", "OceanRipple", "gallery"};
    GalleryOceanRippleOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::oceanRipple(image, options);
    }
};

struct BasRelief final {
    inline static constexpr OperationInfo info{"gallery.basRelief", "BasRelief", "gallery"};
    GalleryBasReliefOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::basRelief(image, options);
    }
};

struct ChalkCharcoal final {
    inline static constexpr OperationInfo info{"gallery.chalkCharcoal", "ChalkCharcoal", "gallery"};
    GalleryChalkCharcoalOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::chalkCharcoal(image, options);
    }
};

struct Charcoal final {
    inline static constexpr OperationInfo info{"gallery.charcoal", "Charcoal", "gallery"};
    GalleryCharcoalOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::charcoal(image, options);
    }
};

struct Chrome final {
    inline static constexpr OperationInfo info{"gallery.chrome", "Chrome", "gallery"};
    GalleryChromeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::chrome(image, options);
    }
};

struct ConteCrayon final {
    inline static constexpr OperationInfo info{"gallery.conteCrayon", "ConteCrayon", "gallery"};
    GalleryConteCrayonOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::conteCrayon(image, options);
    }
};

struct GraphicPen final {
    inline static constexpr OperationInfo info{"gallery.graphicPen", "GraphicPen", "gallery"};
    GalleryGraphicPenOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::graphicPen(image, options);
    }
};

struct HalftonePattern final {
    inline static constexpr OperationInfo info{"gallery.halftonePattern", "HalftonePattern", "gallery"};
    GalleryHalftonePatternOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::halftonePattern(image, options);
    }
};

struct NotePaper final {
    inline static constexpr OperationInfo info{"gallery.notePaper", "NotePaper", "gallery"};
    GalleryNotePaperOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::notePaper(image, options);
    }
};

struct Photocopy final {
    inline static constexpr OperationInfo info{"gallery.photocopy", "Photocopy", "gallery"};
    GalleryPhotocopyOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::photocopy(image, options);
    }
};

struct Plaster final {
    inline static constexpr OperationInfo info{"gallery.plaster", "Plaster", "gallery"};
    GalleryPlasterOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::plaster(image, options);
    }
};

struct Reticulation final {
    inline static constexpr OperationInfo info{"gallery.reticulation", "Reticulation", "gallery"};
    GalleryReticulationOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::reticulation(image, options);
    }
};

struct Stamp final {
    inline static constexpr OperationInfo info{"gallery.stamp", "Stamp", "gallery"};
    GalleryStampOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::stamp(image, options);
    }
};

struct TornEdges final {
    inline static constexpr OperationInfo info{"gallery.tornEdges", "TornEdges", "gallery"};
    GalleryTornEdgesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::tornEdges(image, options);
    }
};

struct WaterPaper final {
    inline static constexpr OperationInfo info{"gallery.waterPaper", "WaterPaper", "gallery"};
    GalleryWaterPaperOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::waterPaper(image, options);
    }
};

struct GlowingEdges final {
    inline static constexpr OperationInfo info{"gallery.glowingEdges", "GlowingEdges", "gallery"};
    GalleryGlowingEdgesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::glowingEdges(image, options);
    }
};

struct Craquelure final {
    inline static constexpr OperationInfo info{"gallery.craquelure", "Craquelure", "gallery"};
    GalleryCraquelureOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::craquelure(image, options);
    }
};

struct Grain final {
    inline static constexpr OperationInfo info{"gallery.grain", "Grain", "gallery"};
    GalleryGrainOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::grain(image, options);
    }
};

struct MosaicTiles final {
    inline static constexpr OperationInfo info{"gallery.mosaicTiles", "MosaicTiles", "gallery"};
    GalleryMosaicTilesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::mosaicTiles(image, options);
    }
};

struct Patchwork final {
    inline static constexpr OperationInfo info{"gallery.patchwork", "Patchwork", "gallery"};
    GalleryPatchworkOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::patchwork(image, options);
    }
};

struct StainedGlass final {
    inline static constexpr OperationInfo info{"gallery.stainedGlass", "StainedGlass", "gallery"};
    GalleryStainedGlassOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::stainedGlass(image, options);
    }
};

struct Texturizer final {
    inline static constexpr OperationInfo info{"gallery.texturizer", "Texturizer", "gallery"};
    GalleryTexturizerOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::texturizer(image, options);
    }
};

struct FilterGallery final {
    inline static constexpr OperationInfo info{"gallery.filterGallery", "FilterGallery", "gallery"};
    std::vector<GalleryEffect> effects;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::filterGallery(image, effects);
    }
};

} // namespace iiGraphicAdjust::objects
