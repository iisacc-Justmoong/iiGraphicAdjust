# Camera Raw rendered-image filter

`<iiGraphicAdjust/CameraRawFilter.h>` exports `cameraRawFilter(ImageView image, const CameraRawOptions& options = {})`. It returns an owned `Result<Image>` and borrows the source image without changing it. The implementation uses C++23 and has no Qt dependency.

The algorithm is adapted from Photocraft [commit 5896f0b63e6f342e0311920b4d2a0ea5a94330fc, `crates/algo/src/camera_raw.rs`](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/camera_raw.rs). Its MIT notice is retained in `third_party/photocraft/LICENSE-MIT`. This is the actual `Filter › Camera Raw Filter` over display-encoded RGB. It is not a sensor RAW decoder, a DNG camera profile implementation or a promise of Adobe pixel parity.

## Processing order and parameter meaning

All RGB input and output channels use straight-alpha encoded sRGB in `[0,1]`. The filter keeps extended intermediate RGB values through its stages and applies the SDR clamp at the end. For example, an exposed red channel above 1 still contributes its full value to a later desaturation operation. Alpha remains exactly the source value. Transparent pixels keep their original hidden RGB; spatial means, guided statistics, chroma blurs and atmospheric-light estimates use alpha-weighted samples to keep hidden RGB out of visible output. This transparent-pixel handling strengthens the SDK contract relative to the reference, which iterates directly over every source RGB value.

The ordered stages are:

1. Relative white balance and exposure. Temperature and tint are centered slider units, not Kelvin. Linear-light channel gains are `(1+.35*temperature/100, 1-.25*tint/100, 1-.35*temperature/100)`, normalized by BT.709 luma and multiplied by `2^exposure`. The result is encoded back to sRGB without intermediate clipping. No conversion to the Develop API's Kelvin white-balance setting occurs.
2. Dehaze. A proxy with a longest axis at most 768 samples uses the dark-channel prior. The brightest 0.1 percent of its dark channel estimate atmospheric color; normalized dark-channel transmission is refined by a box-guided filter and upsampled. Positive values remove haze; negative values add haze toward the estimated atmosphere. Alpha is used for proxy averaging and atmospheric selection.
3. Tone on encoded BT.709 luminance. Highlights and Shadows use fast box-guided base layers and exposure weights; Clarity adds a midtone-weighted larger band; Texture adds a small Gaussian band. Whites and Blacks shift their ends of the curve. Contrast adds an S-curve based on `-sin(2*pi*luma)`. Color ratios are retained, and over-range luminance from prior stages is carried forward.
4. Presence. Saturation changes encoded chroma around BT.709 luma. Vibrance weights less saturated colors and partially protects orange skin hues.
5. Curves. Four smooth parametric regions with movable splits are sampled into a 1024-entry monotonic LUT. A Fritsch-Carlson point-curve LUT then applies the master curve, followed by red, green and blue curves. Point coordinates use the reference's `[0,255]` units. LUT evaluation extends linearly outside `[0,1]` for intermediate values.
6. HSL mixer. Eight neighboring hue centers use circular hat weights. Hue moves by up to 30 degrees; saturation scales HSV saturation; luminance is restored after hue/saturation changes, then adjusted by a saturation-weighted exposure shift. Achromatic pixels skip the hue mixer.
7. Color grading. Shadows, midtones, highlights and global wheels add luma-neutral HSV tints with tone masks from Blending and Balance, then apply each wheel's luminance exposure.
8. Detail. Luminance NR uses a box-guided filter with an amount-dependent radius and epsilon. Color NR blurs `B-Y` and `R-Y` chroma. Sharpening uses an unsharp luminance band with its actual Radius, Detail halo limit and Masking gradient gate. Detail settings affect their active parent stage; changing a radius with Sharpen Amount zero remains identity.
9. Post-crop vignette and grain. The three vignette styles use a rounded ellipse/rectangle mask, highlight protection and exposure or overlay operations. Grain uses Photocraft's 64-bit position hash, interpolated value noise, a second roughness layer and luminance weighting. Seed is deterministic; Pixel Scale maps preview positions to full-resolution grain coordinates.

Scalar Gaussian stages preserve the reference's three-box approximation and its small-sigma correction in [`fxutil.rs`](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/fxutil.rs#L165): if all rounded box radii become zero, one radius-1 pass is used. Thus the documented Sharpen Radius `.5` and low Color NR amounts still have a real effect. The SDK's other Gaussian filters have separate contracts.

## All option fields

`CameraRawWheel` contains `hue`, `saturation` and `luminance`. `CameraRawVignetteStyle` contains `HighlightPriority`, `ColorPriority` and `PaintOverlay`. `CameraRawOptions` has the following complete set of controls; none is silently mapped to an unrelated Develop control.

| Fields | Defaults | Accepted range or meaning |
| --- | --- | --- |
| `temperature`, `tint` | 0, 0 | Relative sliders, `[-100,100]` |
| `exposure` | 0 | `[-5,5]` stops |
| `contrast`, `highlights`, `shadows`, `whites`, `blacks` | 0 | `[-100,100]` |
| `texture`, `clarity`, `dehaze`, `vibrance`, `saturation` | 0 | `[-100,100]` |
| `curveHighlights`, `curveLights`, `curveDarks`, `curveShadows` | 0 | `[-100,100]` |
| `curveSplits` | `{25,50,75}` | Each `[5,95]`, strictly increasing |
| `pointCurve`, `pointCurveRed`, `pointCurveGreen`, `pointCurveBlue` | Empty | Empty or 2–16 `[input,output]` pairs, each `[0,255]`, input gap at least 1 |
| `hslHue`, `hslSaturation`, `hslLuminance` | Eight zeroes each | `[-100,100]`, in red/orange/yellow/green/aqua/blue/purple/magenta order |
| `gradeShadows`, `gradeMidtones`, `gradeHighlights`, `gradeGlobal` | Neutral wheels | Hue `[0,360]`, saturation `[0,100]`, luminance `[-100,100]` |
| `gradeBlending`, `gradeBalance` | 50, 0 | `[0,100]`, `[-100,100]` |
| `sharpenAmount` | 0 | `[0,150]` |
| `sharpenRadius` | 1 | `[.5,3]` pixels before Pixel Scale |
| `sharpenDetail`, `sharpenMasking` | 25, 0 | `[0,100]` |
| `noiseLuminance`, `noiseLuminanceDetail` | 0, 50 | `[0,100]` |
| `noiseColor`, `noiseColorDetail` | 0, 50 | `[0,100]` |
| `grainAmount`, `grainSize`, `grainRoughness` | 0, 25, 50 | `[0,100]` |
| `vignetteAmount`, `vignetteRoundness` | 0, 0 | `[-100,100]` |
| `vignetteMidpoint`, `vignetteFeather`, `vignetteHighlights` | 50, 50, 0 | `[0,100]` |
| `vignetteStyle` | `HighlightPriority` | One of the three enum values |
| `seed` | 0 | Unsigned 32-bit grain seed |
| `pixelScale` | 1 | `[.01,1]`, scales detail radii and preview grain coordinates |

Every floating parameter must be finite, including inactive dependent controls. Curves require ascending input coordinates; this public API does not accept malformed legacy saved curves. Y coordinates may descend, as the reference's validator allows. Empty curves and points that lie entirely on the identity diagonal are treated as linear, matching the reference's stage-neutral rule. Unknown style enums and values outside the ranges are rejected with `InvalidParameter` rather than clamped silently.

The reference CameraRaw settings contain no Luminance NR Contrast or Color Smoothness field. Those controls are not exposed here. They also do not contain a RAW decoder, camera-profile/LUT setting, B&W treatment, optics stage, masks or retouch strokes. Other SDK modules provide those separate operations where implemented.

Input dimensions must be nonzero, each axis at most 65536 and total pixels at most `64*1024*1024`. The image buffer must have exactly `width*height` normalized finite RGBA pixels. Dimensions and multiplication overflow are checked before allocation. Allocation failures return `AllocationFailure`. Neutral settings return an exact detached copy after validation.

```cpp
#include <iiGraphicAdjust/CameraRawFilter.h>
using namespace iiGraphicAdjust;
CameraRawOptions settings;
settings.temperature = 20; // Relative warmth, not 20 K.
settings.exposure = .5;
settings.sharpenAmount = 35;
settings.sharpenRadius = .5;
settings.sharpenDetail = 60;
settings.noiseLuminance = 20;
settings.pointCurve = {{0,10},{64,55},{192,205},{255,255}};
auto result = cameraRawFilter(source.view(), settings);
```

## Verification

`tests/camera_raw_filter.cpp` supports `oracle`, `basic`, `color`, `detail`, `effects`, `validation`, `invariants` and `all` groups. It contains 35 numeric cases produced by the pinned Rust `camera_raw::develop` on an opaque 17×11 fixture, with five sampled pixels per case. The standalone Rust oracle only substitutes sequential workers and removes serialization attributes; its processing formulas are the upstream implementation. The source and execution log are retained under `build/camera-raw-upstream-oracle.rs` and `build/camera-raw-upstream-oracle.log` for reproduction. The test embeds fixed values and does not depend on a Rust compiler at test time.

Additional checks cover neutral identity, one-stop exposure, relative WB, intermediate over-range preservation, all four point curves and all eight HSL bands, curve splits, grading blending/balance, real Radius `.5` sharpening, active Detail/Masking and both NR Detail controls, every vignette style/control, grain parameters and seed, every scalar's NaN validation, all curve channels' malformed inputs, invalid dimensions/buffers, preserved alpha, hidden-RGB independence for all reference cases and deterministic combined-parameter finite-output trials.
