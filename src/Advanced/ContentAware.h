#pragma once

#include <iiGraphicAdjust/Masks.h>

namespace iiGraphicAdjust {

// Borrowed signed seam weights: positive protects, negative prefers removal.
// The caller retains immutable storage throughout the execution call.
struct SeamConstraintView final {
    std::size_t width = 0, height = 0;
    std::span<const float> weights;
};

// Owns signed weights in row-major image coordinates; copying is independent.
struct SeamConstraints final {
    std::size_t width = 0, height = 0;
    std::vector<float> weights;
    [[nodiscard]] SeamConstraintView view() const noexcept { return {width,height,weights}; }
};

namespace objects {

// Deterministic exemplar fill. Inputs are borrowed for one call; the returned
// image is detached. Calls on an unchanged object can run concurrently when
// inputs are immutable. The entire selection cannot be masked.
struct ContentAwareInpaintSolver final {
    unsigned patchRadius = 1;       // [1,4] pixels of known context.
    std::size_t maxCandidates = 4096; // [1,65536] evenly sampled original pixels.
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> execute(ImageView, MaskView) const;
};

// Combines normalized caller-owned masks as protect minus remove. Overlap
// cancels; neither mask is mutated or retained. Invalid values fail explicitly.
struct SeamConstraintMask final {
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<SeamConstraints>
    execute(MaskView protect, MaskView remove) const;
};

// Resizes by deterministic minimum-energy seam removal/insertion. Constraints
// bias selection rather than guaranteeing preservation. Empty constraints mean
// no preference. Limits bound work and can be configured by the caller.
struct ContentAwareRescaleSolver final {
    std::size_t maxPixels = 64U*1024U*1024U;
    std::size_t maxSeams = 4096;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
    execute(ImageView, std::size_t width, std::size_t height,
            SeamConstraintView constraints = {}) const;
};

} // namespace objects
} // namespace iiGraphicAdjust
