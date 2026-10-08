#pragma once
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {

// Linear-light sRGB primaries, finite nonnegative RGB (values above one allowed).
struct HdrRgba final { float r=0,g=0,b=0,a=1; };
struct HdrImageView final {
    std::size_t width=0,height=0;
    std::span<const HdrRgba> pixels;
};
enum class HdrToningMethod { ExposureGamma, HighlightCompression };
struct HdrToningOptions final {
    HdrToningMethod method=HdrToningMethod::ExposureGamma;
    double exposure=0; // Stops, [-16,16].
    double gamma=1;    // Linear-light power 1/gamma, [0.01,100].
};

// Returns normalized straight-alpha encoded sRGB. Output clipping is explicit.
// Invisible HdrImageView pixels become transparent black; the SDR overload
// preserves the original hidden normalized RGB.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> hdrToning(
    HdrImageView image,const HdrToningOptions& options = {});
// Convenience for an existing normalized sRGB image; decodes it before toning.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> hdrToning(
    ImageView image,const HdrToningOptions& options = {});
} // namespace iiGraphicAdjust
