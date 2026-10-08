# Lightroom-style development

Reference: [storytold/lightcraft](https://github.com/storytold/lightcraft), inspected commit `629e39380e296f588c64cd9c0053a8edc3528f36`. This is an independent C++23 implementation, not an Adobe SDK or a claim of Adobe pixel parity. LightCraft's MIT attribution is distributed in `third_party/lightcraft/`.

## Implementation order and source audit

| Order | Target | Actual reference processing |
| --- | --- | --- |
| 1 | Kelvin/tint WB, presets, auto/eyedropper; six Light sliders and Auto; parametric/cubic point curves and Refine Saturation | `color/src/cct.rs`, `color/src/spline.rs`, `pipeline/src/local.rs`, `tone.rs`, `auto.rs`, `finish.rs` |
| 2 | Eight-band Color Mixer/B&W; Point Color; skin-protected Vibrance/Saturation; four-wheel grading; Calibration | `pipeline/src/colorops.rs` |
| 3 | Texture, Clarity, Dehaze; post-crop vignette and grain; detail sharpening and luminance/color NR | `pipeline/src/local.rs`, `finish.rs` |
| 4 | Manual distortion, manual/automatic lateral CA, supplied lens coefficients, lens vignetting and purple/green defringe | `pipeline/src/optics.rs` |
| 5 | Composed development and deterministic local masks/coverage | `pipeline/src/lib.rs`, `masks.rs`, `finish.rs` |

The reference declares Sharpen Radius, Sharpen Detail, NR Contrast, Lens Blur and Enhance settings, but the inspected CPU/GPU pipeline does not read them. They are not added as ineffective controls. RAW decoding, camera/lens profile registries, AI segmentation, healing, red-eye, crop/geometry, catalog/preset persistence and GUI are separate host concerns. Existing Cube LUT APIs supply creative looks without importing the reference's preset database. Embedded DNG lens metadata is not decoded by this module.

## Pixel contract

Standalone operations take normalized straight-alpha encoded sRGB. Internal tone/spatial calculations decode to double-precision linear sRGB; color tools use OkLab/OkLCh derived from those primaries. This intentionally differs from LightCraft's Rec.2020 working space. Outputs preserve alpha and invisible normalized RGB, are detached, and gamut-map towards linear luminance before encoding. Spatial statistics are weighted by alpha: hidden pixels cannot influence visible neighbors. Inputs are finite and checked; invalid settings return `InvalidParameter` instead of silently clamping.

Light slider units match the reference: exposure in stops `[-5,5]`, other sliders `[-100,100]`. Highlights/shadows shift a Gaussian self-guided log-luminance base. Contrast, whites and blacks use the reference's display-referred curve with a highlight shoulder when active; all-neutral rendered development is an exact copy. No camera tone look is guessed from SDR pixels.

White balance resolves a Planckian locus in CIE 1960 uv, tint normal to that locus, and a luminance-normalized Bradford adaptation relative to the supplied as-shot temperature/tint. Temperatures are `[2000,50000]` K and tints `[-150,150]`. Auto uses alpha-weighted midtone/low-chroma statistics, while sampled WB uses one visible nonblack sample. Estimates return settings, leaving the application responsible for when to apply them.

Parametric curves use movable splits in percent, strictly ordered inside `(0,100)`, raised-cosine region windows and a monotone sampled base. Point curves have 2..65536 finite normalized points with increasing inputs separated by at least `1e-9`; cubic Hermite interpolation limits segment slopes to avoid overshoot. Order is parametric → master → individual channel. Refine Saturation restores the pre-curve encoded chroma/luma ratio using the reference's bounded power blend; exact gray output stays gray. Previous Photoshop-style piecewise-linear `curves()` semantics are unchanged.

Tint is a correction slider: positive adds magenta. The adaptation and estimator invert the reference's illuminant-tint sign consistently to honor this UI direction. Gaussian windows below sigma 3 use explicit convolution; wider windows use three variance-matched box passes to avoid radius-dependent photo-size processing costs. The latter is an approximation rather than exact LightCraft kernel parity.

## Color

`DevelopColorOptions` applies eight-band Mixer → up to eight Point Color samples → skin-protected Vibrance/Saturation → optional eight-band B&W → Shadows/Midtones/Highlights/Global grading. Mixer and B&W arrays use `MixerBand` order. Hue membership uses raised-cosine weights between OkLCh hues of pure sRGB band centers; weights sum to one around the hue circle. Mixer hue shift is ±0.5 radians, chroma scales by `1 + saturation/100`, and lightness shifts by ±0.18 with chroma weighting. Gray pixels receive no arbitrary mixer hue. Positive vibrance protects skin around the hue of sRGB HSV 25 degrees and saturated colors.

`samplePointColor()` returns a visible color's OkLCh sample. Each sample controls hue/chroma/lightness widths, soft selection, three shifts and spread variance. Grading uses hue degrees `[0,360]`, saturation `[0,100]`, luminance `[-100,100]`, blending `[0,100]` and balance `[-100,100]`. Calibration rotates/scales linear sRGB primaries and normalizes the matrix to preserve the neutral axis, then applies luminance-preserving shadow tint. Degenerate calibration matrices return an error. Auto B&W computes alpha-weighted lightness separation for each chromatic band and returns eight slider values.

Point Color is disabled during B&W treatment, matching the actual reference implementation.

## Effects and detail

Texture scales a fine Gaussian log-luminance band (sigma `max(0.6, 0.0018*longEdge)`); Clarity uses a self-guided band (sigma `max(1, 0.012*longEdge)`, epsilon 0.8), with edge/detail taming and midtone weighting. Dehaze follows the CPU code rather than the README claim: blur `min(R,G,B)`, estimate airlight at the 99.5th percentile of visible values, and use transmission bounded below by 0.12. Negative Dehaze adds the estimated veil.

Post-crop Vignette uses normalized current-image coordinates, midpoint, roundness and feather. HighlightPriority protects bright pixels with the Highlights slider; ColorPriority scales colors; PaintOverlay uses black/white paint behavior (negative ColorPriority and PaintOverlay are numerically identical in this reference). Film Grain is deterministic seeded two-octave value noise in encoded sRGB, with size relative to the long edge and midtone weighting. No global random state is used.

Detail exposes only controls used by the reference: Sharpen Amount/Masking, Luminance Noise/Detail and Color Noise/Detail/Smoothness. Luminance NR filters log luminance with edge-aware guidance; color NR filters chromaticity at preserved luminance. Sharpening follows NR and scales the fine log-luminance band with optional edge masking. This is conventional deterministic NR, not Adobe AI Denoise. Operations validate inactive controls too.

## Optics

Manual distortion uses radial coefficient `-0.2*slider/100` about the image center, vignetting uses corner gain up to ±1.5 stops, and R/B chromatic aberration scales each plane relative to green by up to ±0.003. Auto CA collects radial high-contrast edges spread over a 24×24 grid, matches normalized channel profiles at subpixel offsets, and robustly fits displacement/radius. Weak evidence returns zero. No global image cache is used. Center coordinates consistently use pixel centers.

Hosts may supply decoded DNG-style per-plane radial/tangential warp coefficients and five radial vignette coefficients through `embeddedWarp`/`embeddedVignette`. Profile strengths scale those supplied coefficients; the SDK does not load Adobe LCP profiles or decode file metadata. Coefficients are finite `[-16,16]`, normalized centers `[0,1]`, radii `[1e-6,2]` long-edge units. Defringe reduces OkLab chroma of purple/green colors only near strong lightness edges; amount is `[0,20]` and ordered hue range sliders are `[0,100]`.

Geometric sampling is bilinear in linear light with alpha normalization per channel. Hidden RGB never leaks into resampled colors. Dimensions remain unchanged; green-plane coverage supplies output alpha, outside green samples become transparent black, R/B use edge clamps, and warp-created invisible RGB is canonicalized. Vignette gain is evaluated once at the green-plane source position and shared across RGB. This is the explicit exception to non-geometric alpha/hidden-color preservation. Defringe lightness extrema use separable sliding windows with linear pixel cost.

## Masks and local adjustment

`Mask` owns row-major float coverage `[0,1]`; `MaskView` borrows it. Linear gradients require endpoints separated by at least `1e-9` in normalized coordinates. Radial masks use normalized centers, long-edge radii and rotated elliptical feathering. Brush strokes interpolate dabs at radius/4 (minimum half a pixel), accumulate flow up to density, combine paint with max and subtract eraser paint. Auto Mask weights dabs by similarity to the color under the dab and uses guided refinement. Limits are 4096 total stroke points and one million dabs per call. No UI gesture state is retained.

Luminance Range maps log luminance from -8 to +4 EV around linear gray 0.18 into normalized range bounds. Color Range compares OkLab after a per-channel `c/(1+c)` selection mapping; supplied normalized colors are mapped the same way, so the exact sampled color selects itself at neutral exposure. Range masks have an explicit exposure argument. Add is `max(a,b)`, Subtract is `a*(1-b)`, Intersect is `a*b`. Invert and image-guided edge refinement return detached coverage. The caller can pass independently generated segmentation coverage through `MaskView` without importing an AI runtime.

`applyLocalAdjustments()` accepts up to 32 same-sized borrowed masks. It accumulates coverage × amount × each slider before processing: Exposure, Temperature, Tint, Contrast, Highlights, Shadows, Whites, Blacks, Texture, Clarity, Dehaze, Hue, Saturation, Sharpness, Noise, Moiré and Defringe. Amount is `[0,200]`; most sliders are `[-100,100]`, Exposure `[-5,5]`, Defringe `[0,100]`. The last contributing color tint wins, following the reference. Tint hue is `[0,360]`, strength `[0,100]`. Spatial statistics are computed from the complete image and weighted by source alpha; coverage controls which pixels receive changes. Zero/cancelled contributions preserve original encoded pixels exactly, including the sRGB transfer-function knee; source alpha and hidden RGB are preserved. All mask storage must remain alive and immutable for the call.

## Composition and usage

`develop()` invokes Optics → WB → Detail → Effects → Calibration → Light → Color → Vignette → Curve → Grain. This convenience deliberately composes public rendered-image operations; every stage returns normalized encoded sRGB and gamut-maps before the next stage. It is **not** LightCraft's scene-linear Rec.2020 RAW pipeline: exposure headroom and colors outside sRGB can be lost between stages. The existing `HdrImageView`/`hdrToning()` APIs remain separate. Call individual operations when the host needs another order, and apply local masks explicitly in the desired stage.

```cpp
#include <iiGraphicAdjust/iiGraphicAdjust.h>

iiGraphicAdjust::DevelopOptions options;
options.light.exposure = 0.5;
options.light.shadows = 25;
options.color.vibrance = 20;
options.effects.texture = 15;
auto result = iiGraphicAdjust::develop(source.view(), options);
if (!result) return;

auto mask = iiGraphicAdjust::radialMask(result->width, result->height);
if (!mask) return;
iiGraphicAdjust::LocalAdjustment local{mask->view(), {}};
local.options.exposure = 0.3;
std::array<iiGraphicAdjust::LocalAdjustment, 1> adjustments{local};
auto localized = iiGraphicAdjust::applyLocalAdjustments(result->view(), adjustments);
```

The runnable `iiGraphicAdjustDevelopExample` generates a 256×128 PPM from a procedural source:

```sh
./build/iiGraphicAdjustDevelopExample build/lightroom.ppm
```

Source and package contain no Rust runtime, Qt, GUI framework or model dependency. Reference attribution is preserved under the MIT option of LightCraft's dual license. Lightroom/Adobe output parity, camera RAW development, color-managed wide-gamut export, UI integration and non-macOS platforms require separate validation.
