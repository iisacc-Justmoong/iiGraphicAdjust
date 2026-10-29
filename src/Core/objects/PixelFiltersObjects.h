#pragma once
#include <iiGraphicAdjust/PixelFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct ColorHalftone final {
    inline static constexpr OperationInfo info{"pixel.colorHalftone", "ColorHalftone", "pixel"};
    ColorHalftoneOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::colorHalftone(image, options);
    }
};

struct Crystallize final {
    inline static constexpr OperationInfo info{"pixel.crystallize", "Crystallize", "pixel"};
    CrystallizeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::crystallize(image, options);
    }
};

struct Facet final {
    inline static constexpr OperationInfo info{"pixel.facet", "Facet", "pixel"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::facet(image);
    }
};

struct Fragment final {
    inline static constexpr OperationInfo info{"pixel.fragment", "Fragment", "pixel"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::fragment(image);
    }
};

struct Mezzotint final {
    inline static constexpr OperationInfo info{"pixel.mezzotint", "Mezzotint", "pixel"};
    MezzotintOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::mezzotint(image, options);
    }
};

struct Pointillize final {
    inline static constexpr OperationInfo info{"pixel.pointillize", "Pointillize", "pixel"};
    PointillizeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::pointillize(image, options);
    }
};

struct Mosaic final {
    inline static constexpr OperationInfo info{"pixel.mosaic", "Mosaic", "pixel"};
    double cellSize = 10;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::mosaic(image, cellSize);
    }
};

struct Emboss final {
    inline static constexpr OperationInfo info{"pixel.emboss", "Emboss", "pixel"};
    EmbossOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::emboss(image, options);
    }
};

struct FindEdges final {
    inline static constexpr OperationInfo info{"pixel.findEdges", "FindEdges", "pixel"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::findEdges(image);
    }
};

struct Solarize final {
    inline static constexpr OperationInfo info{"pixel.solarize", "Solarize", "pixel"};
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::solarize(image);
    }
};

struct Diffuse final {
    inline static constexpr OperationInfo info{"pixel.diffuse", "Diffuse", "pixel"};
    DiffuseOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::diffuse(image, options);
    }
};

struct Extrude final {
    inline static constexpr OperationInfo info{"pixel.extrude", "Extrude", "pixel"};
    ExtrudeOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::extrude(image, options);
    }
};

struct OilPaint final {
    inline static constexpr OperationInfo info{"pixel.oilPaint", "OilPaint", "pixel"};
    OilPaintOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::oilPaint(image, options);
    }
};

struct Tiles final {
    inline static constexpr OperationInfo info{"pixel.tiles", "Tiles", "pixel"};
    TilesOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::tiles(image, options);
    }
};

struct TraceContour final {
    inline static constexpr OperationInfo info{"pixel.traceContour", "TraceContour", "pixel"};
    TraceContourOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::traceContour(image, options);
    }
};

struct Wind final {
    inline static constexpr OperationInfo info{"pixel.wind", "Wind", "pixel"};
    WindOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::wind(image, options);
    }
};

} // namespace iiGraphicAdjust::objects
