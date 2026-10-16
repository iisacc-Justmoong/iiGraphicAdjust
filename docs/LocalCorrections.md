# Masked correction composition

Version 0.5.1 adds `applyMaskedCorrection()` in the public `LocalCorrections.h` header. It takes a base rendered image, an already-computed corrected image, and caller-owned `MaskView` coverage. It applies RGB from any existing correction kernel locally without introducing a dependency on the module that generated the mask. There is no callback, document, brush runtime, GPU backend or cross-SDK adapter inside this leaf function.

## Contract

Both images contain tightly packed normalized encoded sRGB and straight alpha, following `ImageView`. The mask contains exactly `width * height` finite floats in `[0,1]`. All three geometries must match. The call validates every image channel, every coverage value, dimensions, exact storage lengths, amount and interpolation enum before returning even a no-op result. Inputs must refer to live, immutable storage for the synchronous call; the SDK cannot validate dangling or otherwise invalid spans.

| Setting | Default | Contract |
| --- | --- | --- |
| `MaskedCorrectionOptions::amount` | `100` | Finite percentage `[0,100]`; this is a blend fraction, distinct from `LocalAdjustmentOptions::amount` `[0,200]` slider overdrive |
| `space` | `CorrectionSpace::LinearSrgb` | `Srgb` interpolates encoded channels; `LinearSrgb` decodes, interpolates linear channels, then encodes to sRGB |
| `invert` | `false` | Invert coverage before scaling by amount |

For a visible base pixel, the effective weight is `(invert ? 1-coverage : coverage) * amount/100`. Weight zero leaves the base pixel bit-for-bit unchanged. Weight one directly copies corrected RGB, bypassing transfer functions. Fractional weight uses RGB interpolation in the selected space. A component whose two endpoint values are equal keeps the base component, avoiding an unnecessary round trip at the sRGB transfer knee. The original base alpha is always preserved. A base pixel with alpha zero, including negative zero, retains all its hidden RGB even under full coverage.

Corrected alpha is validated but does not affect interpolation. For example, corrected alpha zero still supplies RGB to a visible base pixel. This function is RGB correction composition, not Porter–Duff source-over, alpha interpolation, selection segmentation or correction-parameter interpolation. Encoded black/white at half coverage yields `0.5`; linear sRGB yields approximately `0.735357`.

The output owns detached storage. Aliasing the base and corrected borrowed views is supported; neither image nor coverage is modified or retained. The operation costs O(N) work and one output image, without decoding the entire input into extra planes. Existing local slider accumulation and existing correction algorithms retain their previous semantics.

Errors use the existing `Result<Image>`/`ErrorCode` contract: invalid geometry is `InvalidDimensions`, mismatched storage is `InvalidBuffer`, invalid image channels are `InvalidPixel`, invalid coverage/options are `InvalidParameter`, and valid inputs with different shapes are `SizeMismatch`. Allocation failures use `AllocationFailure`.

## Applying existing kernels

```cpp
#include <iiGraphicAdjust/iiGraphicAdjust.h>

// source is a live ImageView; coverage is a live float span from any producer.
auto corrected = iiGraphicAdjust::whiteBalance(source, {4200, 8});
if (!corrected) return;
iiGraphicAdjust::MaskView mask{source.width, source.height, coverage};
auto local = iiGraphicAdjust::applyMaskedCorrection(
    source, corrected->view(), mask,
    {80, iiGraphicAdjust::CorrectionSpace::LinearSrgb, false});
if (!local) return;
// Consume local->pixels; source, corrected and coverage remain unchanged.
```

Replace `whiteBalance()` with `levels()`, `curves()`, `developCurve()`, `hueSaturation()`, `developDetail()`, `cameraRawFilter()` or another operation returning the same-sized normalized `Image`. The correction's own spatial/statistical calculations run according to its existing contract; this function restricts the final RGB write. Corrected HDR, another color profile, a changed image shape, and automatic conversion between RAW and rendered images are outside this API.

The standalone `iiGraphicAdjustLocalCorrectionsExample` performs a real Kelvin white-balance correction, applies a feathered radial coverage mask and writes a 256×128 PPM:

```sh
build/iiGraphicAdjustLocalCorrectionsExample build/local-correction.ppm
```

## Recovered Adobe evidence and implementation boundary

This composition is an independent C++23 implementation. It is not a recovered Adobe rendering equation and does not establish Photoshop or Lightroom pixel parity. Existing rendered correction/local-slider models are independently adapted from the pinned PhotoCraft/LightCraft sources, as documented elsewhere in this SDK. The separately added `SampleFilters.h` kernels have their own recovered-native/replay evidence and contracts in [SampleFilters](SampleFilters.md); that evidence does not validate these correction algorithms.

The actual Lightroom 9.6 recovered `DevelopLocalizedCommon.lua` output classifies `localPointCurve`, `EnableLocalColor`, `EnableLocalToneCurve`, and `CorrectionAmount` as localized controls at output line 5311. Its output lines 5270–5320 distinguish a masking tone-curve panel from the global panel. The SDK's existing `LocalAdjustmentOptions` exposes 17 accumulated sliders and a color tint, but no point-curve settings. Applying existing `curves()` or `developCurve()` results through `MaskView` closes that concrete functional gap without changing the public local-slider struct or inventing an Adobe equation.

The existing local temperature/tint implementation is a relative channel-gain model, whereas standalone `whiteBalance()` uses Kelvin/tint Bradford adaptation. Composing its computed result through a mask supplies a local Kelvin-WB path while preserving both existing models. The recovered `EditParameters2.lua` output declares `CorrectionAmount` and attaches it to the current correction at output lines 10525–10562, but this declaration does not establish Adobe's interpolation units or equation; `[0,100]` here is an explicit SDK design contract.

The recovered outputs are analysis pseudocode: `DevelopLocalizedCommon.lua` passes Lua 5.4 syntax checking but fails the original Lua 5.1 syntax check; `EditParameters2.lua` fails both. Their original bytecode and complete round-trip checks are preserved in the Lightroom engineering project. Neither script was executed. Native CameraRaw pixel processing and actual Adobe corrected pixels have not been recovered for this composition. Source identities, exact output spans and quality classifications are recorded in [LocalCorrectionsEvidence.json](LocalCorrectionsEvidence.json).

## Validation

The focused suites test bitwise zero/no-op/full-weight endpoints, detached/aliased inputs, signed-zero alpha and hidden RGB, fractional amount, inversion, transfer-space numeric oracles, complementary-mask symmetry and endpoint bounds. Invalid dimensions/overflow, short and excess storage, transposed shapes, every invalid RGBA component, nonfinite/out-of-range coverage and amount, and unknown enums are rejected even on inactive calls. Integration tests localize eight nontrivial existing correction kernels: Levels, Curves, Kelvin WB, HSL, Detail, rendered Camera Raw, Develop Curve and Develop Light. Release and ASan/UBSan results are recorded in [LocalCorrectionsVerification](LocalCorrectionsVerification.md).
