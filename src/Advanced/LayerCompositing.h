#pragma once

#include <iiGraphicAdjust/Blend.h>
#include <iiGraphicAdjust/Masks.h>

namespace iiGraphicAdjust {

// A borrowed layer and its source-over settings; both remain valid for the call.
struct LayerView final { ImageView image; BlendOptions blend; };
// Moving-image coordinates plus this integer displacement yield reference coordinates.
struct LayerTranslation final { int dx = 0, dy = 0; double error = 0; };

namespace objects {

// Searches integer translations using alpha-weighted encoded RGB error. Ties
// prefer smaller motion; insufficient visible overlap is an error. No image is
// retained, and immutable inputs allow concurrent executions.
struct LayerRegistrationSolver final {
    unsigned maxShiftX = 32, maxShiftY = 32; // Each <= 1024.
    double minOverlap = .5;                  // Fraction of image pixels, (0,1].
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<LayerTranslation>
    execute(ImageView reference, ImageView moving) const;
};

// Composites caller-provided, same-sized layers in bottom-to-top order. The
// first layer is composited over transparent black; no input storage is retained.
struct LayerStackBlendSolver final {
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> execute(std::span<const LayerView>) const;
};

// Interpolates foreground and replacement in premultiplied sRGB using the
// supplied sky coverage. A zero/one mask exactly preserves the selected pixel.
struct SkyReplacementCompositor final {
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
    execute(ImageView foreground, ImageView sky, MaskView skyCoverage) const;
};

// Refines the supplied subject mask using the existing edge-guided filter;
// amount follows refineMask's [0,100] contract and zero is an exact copy.
struct SubjectEdgeMatteRefiner final {
    double amount = 50;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Mask> execute(ImageView, MaskView) const;
};

// Unmixes caller-supplied background from partial subject pixels in linear
// sRGB. Full/zero coverage and source alpha are preserved. Tiny coverage is
// bounded by minimumCoverage to avoid unstable division; RGB is clipped.
struct ColorDecontaminationPass final {
    double amount = 1;              // [0,1].
    double minimumCoverage = .01;   // (0,1].
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
    execute(ImageView observed, ImageView background, MaskView subjectCoverage) const;
};

} // namespace objects
} // namespace iiGraphicAdjust
