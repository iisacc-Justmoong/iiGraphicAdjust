# Sensor Bayer RAW processing

`RawProcessing.h` processes an already decoded, unpacked unsigned 16-bit Bayer
mosaic. It does not open a RAW file or infer calibration from a camera model.
The implementation uses standard C++23 without Qt or external image libraries.

## Input and calibration contract

`BayerImageView` supplies the image width and height, a stride measured in
`uint16_t` samples, a read-only sample span, and one of `RGGB`, `BGGR`, `GRBG`, or
`GBRG`. Each dimension must be at least two, and the stride must be at least the
width. The span must contain `(height - 1) * strideSamples + width` samples;
padding after the final row is unnecessary. Row padding and extra trailing
samples are ignored. Dimension and buffer-address arithmetic are checked before
any sample is read.

The pattern describes the 2x2 tile at the sensor reference origin. `phaseX` and
`phaseY` must each be zero or one. For a crop starting at sensor position `(x,y)`,
use `phaseX = x % 2` and `phaseY = y % 2`. Both the CFA color and black-level
lookup follow this phase. Callers must retain the original tile description when
using phase offsets; changing the pattern as well would apply the shift twice.

`RawCalibration` supplies:

- `blackLevels`: four row-major values at the sensor reference tile, including
  separate values for its two green sites. Each value must be finite,
  nonnegative, and strictly below the white level.
- `whiteLevel`: a finite value in `(0,65535]`, expressed in the same units as
  the unpacked samples.
- `whiteBalanceGains`: positive finite gains for camera red, green, and blue.
- `cameraToLinearSrgb`: a finite row-major 3x3 matrix mapping white-balanced
  camera RGB to linear sRGB with the D65 reference white. The caller must supply
  a verified transform for the sensor and illuminant.

The identity matrix and unit gains are explicit defaults for synthetic data or
the assumption that camera RGB already matches linear sRGB. They do not establish
physical camera color accuracy. The matrix is not a DNG `ColorMatrix` tag or an
ICC profile; the required matrix direction and white point differ.

## Numerical processing

`developRaw(view, options)` performs these operations in order:

1. At each measured site, compute
   `max(0, sample - tileBlack) / (whiteLevel - tileBlack)` in double precision.
   Values above the white reference remain above one.
2. Reconstruct the missing channels with Bayer bilinear interpolation. Measured
   channels are retained. Missing green at red or blue averages axial green
   neighbors; missing opposite red/blue averages diagonal neighbors. At a green
   site, red and blue use their matching horizontal or vertical neighbors.
3. At boundaries, average the available neighbors from the same stencil. No
   reflected, replicated, or fabricated sensor samples are added. A 2x2 image is
   supported; a single row or column is rejected because it does not contain all
   required colors.
4. Multiply camera RGB by its white-balance gains, then apply the supplied color
   matrix. Negative matrix results clamp to zero as an explicit policy.
5. Multiply by `2^exposureStops`, where exposure is finite in `[-16,16]` stops.
   Positive highlights remain unclipped.

The result is a detached `LinearRgbImage` containing opaque `HdrRgba` pixels.
RGB is finite, nonnegative, linear-light sRGB stored as floats; values above one
are valid. Its `view()` can feed the existing `hdrToning` API. Nonfinite
intermediate calculations or output exceeding the finite float range return an
`InvalidPixel` error. Allocation failures return `AllocationFailure` rather than
escaping as allocation exceptions. Input samples and calibration remain unchanged.

`renderRaw(view, options, toning)` composes `developRaw` and `hdrToning`. The
toning stage applies its own exposure and gamma, clipping or highlight
compression, and sRGB transfer encoding. It produces the existing normalized
straight-alpha `Image`. Development exposure and toning exposure are separate,
additive stops. Keeping the linear result allows a later toning adjustment to
recover values above one without an earlier clipping loss.

```cpp
#include <iiGraphicAdjust/RawProcessing.h>
#include <array>

// Synthetic camera RGB assumed to have sRGB primaries.
const std::array<std::uint16_t,4> mosaic{110,410,410,810};
iiGraphicAdjust::BayerImageView source{
    2,2,2,mosaic,iiGraphicAdjust::BayerPattern::RGGB};
iiGraphicAdjust::RawDevelopmentOptions options;
options.calibration.blackLevels={10,10,10,10};
options.calibration.whiteLevel=1010;
auto linear=iiGraphicAdjust::developRaw(source,options);
// Each linear output pixel is approximately R=.1, G=.4, B=.8, A=1.
auto display=iiGraphicAdjust::renderRaw(source,options);
```

## Scope and reference boundaries

Bilinear demosaicing is a basic reconstruction algorithm. High-contrast edges can
show false color and zipper artifacts. This implementation does not claim
Malvar-He-Cutler, adaptive demosaicing, vendor RAW-converter parity, or measured
camera image quality. The bilinear stencils are described in the primary
[IPOL demosaicing article](https://www.ipol.im/pub/art/2011/g_mhcd/revisions/2011-08-14/g_mhcd.htm).
The implementation is independently written; no article source code is vendored.

The [Adobe DNG specification 1.7.1.0](https://helpx.adobe.com/content/dam/help/en/camera-raw/digital-negative/jcr_content/root/content/flex/items/position/position-par/download_section_733958301/download-1/DNG_Spec_1_7_1_0.pdf)
defines repeating black levels, white levels, and camera color transformations.
This API is a smaller numeric contract, not a DNG decoder or a complete DNG
rendering pipeline. In particular, it uses per-tile normalization, retains values
above one, clamps negative sensor values early, and accepts a direct camera-to-
sRGB matrix. DNG's normalization, clipping, linearization, illuminant
interpolation, and camera-to-XYZ processing require additional work by a DNG
adapter. DNG `ColorMatrix1` maps XYZ into camera space and must not be copied
directly into `cameraToLinearSrgb`.

Packed RAW unpacking, file metadata parsing, nonlinear sensor linearization,
defective-pixel correction, lens shading, denoising, highlight reconstruction,
ICC processing, wide-gamut output, and automatic calibration are not included.
Applications can perform suitable calibrated sensor corrections before this API
and use existing image adjustments after SDR rendering.

## Verification

`iiGraphicAdjust.raw.*` contains six independent CTest cases:

| Case | Verified behavior |
| --- | --- |
| `patterns` | Distinct constant RGB through all four patterns, including edges and corners; detached output and immutable input |
| `interpolation` | Independent affine RGB ramps in the interior, measured-site preservation, and explicit axial/diagonal/boundary arithmetic |
| `calibration` | Separate green black levels, black/white endpoints, nonunit gains and nonidentity matrix oracle, and negative matrix policy |
| `rendering` | Headroom preservation, negative toning exposure recovery, clipping/compression, and exact composition with `hdrToning` |
| `layout` | Padded stride, odd dimensions, all Bayer/phase combinations, minimum 2x2, phased black levels, and ignored trailing samples |
| `validation` | Invalid enum, phases, dimensions, truncation, addressing overflow, nonfinite calibration, gain/exposure ranges, and unrepresentable output |

These tests establish numeric behavior on synthetic sensor samples. Physical
camera calibration and hardware-captured RAW image quality remain outside this
verification.
