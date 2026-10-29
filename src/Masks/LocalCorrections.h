#pragma once

#include <iiGraphicAdjust/Masks.h>

namespace iiGraphicAdjust {

enum class CorrectionSpace { Srgb, LinearSrgb };
struct MaskedCorrectionOptions final {
    double amount = 100; // Finite percentage [0,100], distinct from local slider overdrive.
    CorrectionSpace space = CorrectionSpace::LinearSrgb;
    bool invert = false;
};

// Apply an already-computed, same-sized correction through borrowed [0,1] coverage.
// Effective weight = (invert ? 1-coverage : coverage) * amount/100.
// Preserve base alpha and its hidden RGB. Corrected alpha is validated but ignored.
// At visible base pixels, weight zero preserves base exactly and weight one
// copies corrected RGB exactly. Hidden base pixels are always unchanged.
// All inputs/settings are validated even for neutral calls. Output owns new storage;
// no input is mutated or retained. This is RGB correction interpolation, not source-over.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
applyMaskedCorrection(ImageView base, ImageView corrected, MaskView mask,
                      const MaskedCorrectionOptions &options = {});

} // namespace iiGraphicAdjust
