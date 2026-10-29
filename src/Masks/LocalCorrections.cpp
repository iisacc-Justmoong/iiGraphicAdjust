#include "MaskInternal.h"
#include <iiGraphicAdjust/LocalCorrections.h>

namespace iiGraphicAdjust {
namespace {
float interpolate(float base, float corrected, double weight, CorrectionSpace space) {
    // Preserve an unchanged channel, including transfer-knee values and signed zero.
    if (base == corrected)
        return base;
    if (space == CorrectionSpace::Srgb)
        return static_cast<float>(std::lerp(double(base), double(corrected), weight));
    return detail::encoded(std::lerp(detail::linear(base), detail::linear(corrected), weight));
}
} // namespace

Result<Image> applyMaskedCorrection(ImageView base, ImageView corrected, MaskView mask,
                                   const MaskedCorrectionOptions &options) {
    if (auto checked = detail::validate(base); !checked)
        return std::unexpected(checked.error());
    if (auto checked = detail::validate(corrected); !checked)
        return std::unexpected(checked.error());
    if (auto checked = detail::validateMask(mask); !checked)
        return std::unexpected(checked.error());
    if (base.width != corrected.width || base.height != corrected.height ||
        base.width != mask.width || base.height != mask.height)
        return std::unexpected(Error{ErrorCode::SizeMismatch,
                                     "Base, corrected image and mask must have identical dimensions"});
    if (!detail::inRange(options.amount, 0, 100) ||
        (options.space != CorrectionSpace::Srgb && options.space != CorrectionSpace::LinearSrgb))
        return std::unexpected(Error{ErrorCode::InvalidParameter,
                                     "Correction amount must be finite [0,100] and space must be supported"});
    return detail::allocated([&] {
        auto output = detail::copy(base);
        if (options.amount == 0)
            return output;
        for (std::size_t i = 0; i < base.pixels.size(); ++i) {
            if (base.pixels[i].a == 0)
                continue;
            const double coverage = options.invert ? 1. - mask.coverage[i] : mask.coverage[i];
            const double weight = coverage * options.amount / 100;
            if (weight == 0)
                continue;
            const auto source = corrected.pixels[i];
            auto &destination = output.pixels[i];
            if (weight == 1) {
                destination.r = source.r;
                destination.g = source.g;
                destination.b = source.b;
            } else {
                destination.r = interpolate(destination.r, source.r, weight, options.space);
                destination.g = interpolate(destination.g, source.g, weight, options.space);
                destination.b = interpolate(destination.b, source.b, weight, options.space);
            }
        }
        return output;
    });
}
} // namespace iiGraphicAdjust
