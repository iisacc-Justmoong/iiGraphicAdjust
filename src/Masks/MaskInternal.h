#pragma once

#include "Internal.h"
#include <iiGraphicAdjust/Masks.h>

namespace iiGraphicAdjust::detail {
inline Result<void> validateMask(MaskView mask) {
    const auto count = pixelCount(mask.width, mask.height);
    if (!count)
        return std::unexpected(count.error());
    if (*count != mask.coverage.size())
        return std::unexpected(Error{ErrorCode::InvalidBuffer, "Mask count must match dimensions"});
    for (auto value : mask.coverage)
        if (!inRange(value, 0, 1))
            return std::unexpected(Error{ErrorCode::InvalidParameter,
                                         "Mask coverage must be finite and normalized"});
    return {};
}
} // namespace iiGraphicAdjust::detail
