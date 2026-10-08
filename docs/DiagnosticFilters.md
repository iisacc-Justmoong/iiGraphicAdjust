# Diagnostic filters

`<iiGraphicAdjust/DiagnosticFilters.h>` exposes eight leaf CPU APIs for the actual Lightcraft image diagnostic kernels found at commit `629e39380e296f588c64cd9c0053a8edc3528f36`. The six range/mask previews come from `crates/pipeline/src/visualize.rs::point_color_range` and `mask_view`; destination and display gamut classification comes from the per-pixel branch of `finish.rs`, its `gamut_map`/`out_of_gamut` functions and `output.rs::Proof`. Both CPU and GPU renderers call the visualizer; the GPU finish stage has matching proof parameters.

| API | Explicit inputs | Output |
| --- | --- | --- |
| `pointColorRangePreview` | Display image, existing `PointColorOptions` | Selection keeps the photo's color; unselected pixels become grey. |
| `maskColorOverlay` | Display image, existing `MaskView`, `MaskOverlayOptions` | Encoded-color tint weighted by evaluated mask and opacity. |
| `maskColorOnBwOverlay` | Same | Grey photo with the same tint. |
| `maskImageOnBlack` | Display image, existing `MaskView` | Photo composited onto black according to evaluated mask. |
| `maskImageOnWhite` | Same | Photo composited onto white. |
| `maskWhiteOnBlack` | Same | Evaluated mask rendered as grey. |
| `destinationGamutWarning` | Display image, `GamutWarningOptions` | Pixels outside the proof destination gamut become red. |
| `displayGamutWarning` | Same | Proofed pixels outside the sRGB display gamut become blue. |

The diagnostic image retains the input width, height and every input alpha value. Input and output use finite normalized encoded sRGB. Fully transparent pixels retain their original hidden RGB and alpha; there are no neighbor operations that could propagate hidden RGB. Unlike the reference's opaque 8-bit mask overlays, these APIs retain float precision and source coverage. They do not implicitly construct masks, access a selected document, modify adjustment settings or draw editor handles.

## Point Color range

The existing 0.3 `PointColorOptions` is reused directly. `lightness` accepts [0,1], `chroma` [0,.5], `hue` [-180,360] degrees, and `range`, `hueRange`, `saturationRange`, `luminanceRange` [0,100]. The defaults remain the existing type's defaults. Adjustment sliders `hueShift`, `saturationShift`, `luminanceShift`, and `variance` accept [-100,100] and are validated; this selection preview does not execute those adjustments.

The host must supply the finished photo with this selected sample's own adjustments omitted. The reference achieves that by changing copied `DevelopSettings` before rendering. A leaf image API cannot undo an adjustment that has already been applied to its input, so this responsibility is explicit.

The image's linear sRGB is converted to OKLab with the SDK's existing `develop_detail::lab` math. This keeps preview sampling consistent with `samplePointColor` and `developColor`. The reference takes the equivalent path through linear Rec.2020, XYZ and its OKLab matrices; direct matrix coefficients and double precision can differ slightly from the reference's f32 chain. No 8-bit quantization or rounding is introduced.

For `k=max(.05,range/50)`, the selection half-widths are:

- Hue radians: `(.1+.6*hueRange/100)*k`.
- Chroma: `(.02+.12*saturationRange/100)*k`.
- Lightness: `(.05+.4*luminanceRange/100)*k`.

Each box axis has full weight up to half its half-width, then a smoothstep falloff to zero at the full half-width. Hue differences wrap at ±π. A sample chroma below .02 ignores hue; a chromatic sample adds a .005–.025 smooth gate to exclude near-neutral pixels. The three axis weights multiply. Encoded luminance is `g=.2126*R+.7152*G+.0722*B`; each output channel is `g+(channel−g)*weight`.

## Mask previews

All five APIs consume an already evaluated `MaskView` from `<iiGraphicAdjust/Masks.h>`. The mask must have the same dimensions as the image, one finite normalized coverage sample per pixel, and valid nonempty dimensions. Existing `linearMask`, `radialMask`, `brushMask`, range masks or a host-supplied segmentation plane can provide it. Visibility flags and component composition remain the host's responsibility; a hidden editor mask may still be supplied for preview, as in the reference.

`MaskOverlayOptions` uses normalized encoded RGB `color` (default red `[1,0,0]`) and `opacity` [0,100] (default 50). The source receives byte RGB and byte integer percent from UI state; this SDK keeps the channels and opacity in floating-point. For mask `a`, opacity `o=opacity/100`, image channel `p` and encoded luminance `g`:

| Function | Channel formula |
| --- | --- |
| `maskColorOverlay` | `p+(color−p)*a*o` |
| `maskColorOnBwOverlay` | `g+(color−g)*a*o` |
| `maskImageOnBlack` | `p*a` |
| `maskImageOnWhite` | `p*a+1−a` |
| `maskWhiteOnBlack` | `a` |

Zero tint opacity is an exact identity for Color; ColorOnBw still renders its grey base. The three non-color modes do not accept irrelevant tint/opacity options. No mask ID lookup, UI color, GPU texture or `Plan` is required.

## Destination and display gamut warnings

`GamutWarningOptions::proofSpace` is one of `DiagnosticColorSpace::{Srgb,DisplayP3,AdobeRgb,ProPhoto,Rec2020}`; the default is Srgb. The published primary/white-point definitions and Bradford white adaptation follow `lightcraft_color` and `output.rs`. ProPhoto uses D50, while the other spaces use D65. AdobeRgb names the published 1998 compatible space; no Adobe ICC profile or proprietary engine is included.

`linearRec2020` is an optional borrowed span of pre-gamut scene-linear Rec.2020/D65 triples. When present, its count must equal the image pixel count and every channel must be finite in [-64,64]. This is a bounded diagnostic working buffer; it does not change the display image's sRGB/SDR contract. The host should pass the corresponding colors before destination/display gamut mapping. When absent, the SDK decodes the supplied sRGB image into linear Rec.2020. An already clipped sRGB display image contains no evidence of discarded RAW or wide-gamut colors; the empty-span fallback does not recover that information.

The reference gamut mapper converts RGB towards its clamped luminance until all channels fit [0,1], and records the resulting chroma scale. A color triggers a warning only when this scale is below .995, its channel spread exceeds both .001 and 2% of the larger channel's magnitude, and a channel is meaningfully outside the gamut (`min<−.001` or `max>1` with `min<1`). Neutral highlight/shadow clipping is deliberately excluded, matching the source predicate.

Destination Warning evaluates the original color after conversion into the proof space and paints encoded red `[1,0,0]` when it fails. Display Warning first gamut-maps into the proof space, converts that mapped color into linear sRGB, evaluates the display mapper and paints encoded blue `[0,.25,1]` when it fails. These are separate warning-only previews: pixels without a warning retain the caller's displayed color, rather than implicitly replacing the whole photo with a soft-proof render. They match the respective standalone source warning branches; a host that combines both should give the red destination warning priority, as the reference does.

## Excluded state and GUI branches

The source audit found no independent spatial highlight/shadow clipping image kernel. `ui-egui/src/panels/detail.rs::clipping_overlay` at line 730 examines approximate histogram edge-bin fractions and draws red/blue strokes outside the displayed image when the fraction exceeds .003. `view.clipping` only toggles this UI state. Painting a pixel warning mask from those fractions would invent a new algorithm, so neither branch is exposed as an image filter.

`Overlay::None`, mask-ID lookup, unknown-mask handling, `MaskView::next`/labels/parsing, overlay packing, cache keys, and `adjust_settings` are state/routing operations with no separate pixel formula. Histogram generation and assisted-culling clipping scores return statistics and are not overlay kernels. Crop guides, eye/spot handles, file-information overlays and paper-white preview surrounds are GUI drawings. They are outside this CPU image diagnostic module. `visualizeSpots` already implements the remaining actual `visualize.rs` image branch in `<iiGraphicAdjust/RetouchFilters.h>` and is not duplicated here.

## Validation and verification

All API fields are validated, including inactive Point Color adjustments and a tint color at zero opacity. Invalid option/coverage values return `InvalidParameter`; malformed mask buffers return `InvalidBuffer`; mismatched mask or pre-gamut plane dimensions/counts return `SizeMismatch`. Images use normal SDK validation and are limited to 64,000,000 pixels. All operations are linear in pixel count, use bounded per-pixel work, and translate storage failures to `AllocationFailure`.

```cpp
#include <iiGraphicAdjust/DiagnosticFilters.h>
using namespace iiGraphicAdjust;
auto sample = samplePointColor({.8F,.3F,.1F,1});
if (sample) {
    auto selection = pointColorRangePreview(input.view(), *sample);
}
auto tint = maskColorOverlay(input.view(), evaluatedMask.view(), {{1,0,0},40});
GamutWarningOptions warning;
warning.proofSpace = DiagnosticColorSpace::ProPhoto;
warning.linearRec2020 = preGamutColors;
auto monitorWarning = displayGamutWarning(input.view(), warning);
```

`tests/diagnostic_filters.cpp` has `point`, `masks`, `gamut`, and `validation` modes. It independently checks a .5 smoothstep membership, sample retention and distant-color greying, neutral/hue-wrap selection, all five mask formulas, zero opacity, Rec.2020-primary red/blue warning classification, neutral clipping exclusions, the .995 gamut boundary, all five profile white points, invalid nested values, buffer/count mismatches, input normalization, hidden RGB and 1×1/1×N/N×1 images. Normal and ASan/UBSan standalone builds were run. The reference Rust executable and Adobe renderer were not used as pixel oracles.
