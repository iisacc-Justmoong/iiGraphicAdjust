#pragma once
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {

struct ColorAdjustment final {
    double exposure = 0;    // Linear-light stops, [-16,16].
    double brightness = 0; // Encoded RGB offset, [-1,1].
    double contrast = 0;   // Encoded RGB slope 1+contrast around 0.5, [-1,1].
    double saturation = 0; // Encoded RGB chroma multiplier 1+saturation, [-1,1].
    double gamma = 1;      // Encoded RGB power 1/gamma, [0.01,100].
    double temperature = 0; // Relative red/blue gain, [-1,1], not Kelvin.
    double tint = 0;        // Relative green gain, [-1,1].
    double opacity = 1;     // Multiplies alpha, [0,1].
};

// Produces detached storage and never writes through the view.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> adjust(ImageView image,
                                                        const ColorAdjustment& adjustment = {});
} // namespace iiGraphicAdjust
