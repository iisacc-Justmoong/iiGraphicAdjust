#pragma once
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {

enum class BlendMode { Normal, Multiply, Screen, Overlay, Darken, Lighten, SoftLight, Difference };
enum class BlendSpace { Srgb, LinearSrgb };
struct BlendOptions final {
    BlendMode mode = BlendMode::Normal;
    double opacity = 1;
    BlendSpace space = BlendSpace::Srgb;
};

// Source-over composition. Both images must have identical dimensions.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> blend(ImageView backdrop, ImageView source,
                                                       const BlendOptions& options = {});
} // namespace iiGraphicAdjust
