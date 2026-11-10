#pragma once

#include <iiGraphicAdjust/Hdr.h>
#include <array>
#include <cstdint>

namespace iiGraphicAdjust {

// Tile order describes the sensor reference origin, before phase offsets.
enum class BayerPattern { RGGB, BGGR, GRBG, GBRG };

// Caller-decoded, unpacked unsigned sensor samples. Stride is measured in samples,
// not bytes. The buffer may include row padding and additional trailing samples.
// phaseX/phaseY identify a cropped view's parity relative to the sensor origin.
struct BayerImageView final {
    std::size_t width=0,height=0,strideSamples=0;
    std::span<const std::uint16_t> samples;
    BayerPattern pattern=BayerPattern::RGGB;
    std::uint8_t phaseX=0,phaseY=0;
};

struct RawCalibration final {
    // Row-major sensor-reference 2x2 tile: top left, top right, bottom left,
    // bottom right. Two green sites can therefore have separate black levels.
    std::array<double,4> blackLevels{0,0,0,0};
    double whiteLevel=65535;
    std::array<double,3> whiteBalanceGains{1,1,1}; // Camera R,G,B, positive.
    // Row-major transform from white-balanced camera RGB to linear sRGB (D65).
    // Identity defaults assume the camera primaries already are linear sRGB;
    // physical color calibration is the caller's responsibility.
    std::array<double,9> cameraToLinearSrgb{1,0,0,0,1,0,0,0,1};
};

struct RawDevelopmentOptions final {
    RawCalibration calibration;
    double exposureStops=0; // [-16,16], applied after the color matrix.
};

// Opaque, finite nonnegative linear-light sRGB. Values above one are retained.
struct LinearRgbImage final {
    std::size_t width=0,height=0;
    std::vector<HdrRgba> pixels;
    [[nodiscard]] HdrImageView view() const noexcept { return {width,height,pixels}; }
};

// Subtract black, normalize, bilinearly reconstruct Bayer RGB, white balance,
// transform to linear sRGB, and apply exposure. At edges, average only available
// neighbors of the required color. At least 2x2 samples are required. Negative
// matrix results clamp to zero; highlights are not clipped. No file decoding,
// metadata inference, automatic calibration, denoising, or gamut mapping occurs.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<LinearRgbImage> developRaw(
    BayerImageView image,const RawDevelopmentOptions& options = {});

// Convenience composition of developRaw followed by the existing hdrToning.
// SDR transfer encoding and highlight clipping/compression occur in hdrToning.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> renderRaw(
    BayerImageView image,const RawDevelopmentOptions& options = {},
    const HdrToningOptions& toning = {});

} // namespace iiGraphicAdjust
