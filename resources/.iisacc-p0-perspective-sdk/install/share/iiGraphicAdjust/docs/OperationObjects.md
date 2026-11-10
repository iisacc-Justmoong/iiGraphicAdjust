# Typed adjustment, correction and filter objects

Version 0.6.0 adds 309 concrete C++23 value objects over the complete existing
image-processing surface, explicit mask/analysis/RAW/resource contracts, five
new correction/fill kernels and active detail controls. It does not recover
Adobe's original source or establish complete Photoshop/Lightroom parity.
The local `Engineering/Projects/Adobe/Photoshop` and `Lightroom` directories
contain PhotoCraft and LightCraft reconstructions. Their algorithms and public
Adobe feature descriptions are references, not an Adobe rendering specification.

`Objects.h` includes the full object surface. Specific users can include one
header under `iiGraphicAdjust/objects/` to keep their compilation boundary small.
Existing free functions and their documented units/working spaces remain
available. The object layer does not change an encoded-space menu filter into a
linear-light Lightroom operation.

## Object families

| Headers | State and responsibility |
| --- | --- |
| `ToneObjects.h`, `ColorObjects.h`, `AdjustmentsObjects.h` | Brightness/Contrast, Levels, Curves, Exposure, automatic tonal correction, hue/saturation, Vibrance, selective color, channel mixing, monochrome, color matching and replacement |
| `MappingObjects.h`, `HdrObjects.h` | Gradient maps, owned 1D/3D LUTs, SDR or linear HDR tone mapping |
| `DevelopToneObjects.h`, `DevelopColorObjects.h`, `DevelopEffectsObjects.h`, `DevelopObjects.h` | Kelvin WB, light, curves, perceptual color, calibration, detail/effects and explicit SDR development composition |
| `DevelopFeatureObjects.h` | Independently usable Texture, Clarity, Dehaze, eight-band Color Mixer, Point Color, Color Grading, monochrome mixer, Vibrance, Sharpening, luminance/color NR and Defringe |
| `BlurFiltersObjects.h`, `OwnedResourceObjects.h` | Blur/sharpen menu algorithms, blur pins/paths and owned depth-map Lens Blur |
| `DistortionFiltersObjects.h`, `MeshFiltersObjects.h` | Owned displacement maps, shear points, Liquify strokes, wide-angle constraints, ARAP puppet pins and perspective planes |
| `UtilityFiltersObjects.h`, `PixelFiltersObjects.h` | Noise, morphology, convolution, guided kernels, video helpers, pixelation and stylization |
| `GalleryFiltersObjects.h`, `RenderFiltersObjects.h` | 47 separate Gallery effects, ordered Gallery settings, procedural textures/lights/flame/tree/frame |
| `LensFilterObjects.h`, `OpticsObjects.h`, `CameraRawFilterObjects.h` | Supplied lens coefficients, CA/defringe, rendered Camera Raw controls |
| `LightcraftFiltersObjects.h` | 22 named look recipes, 41 named presets, selected look/preset objects and four resamplers |
| `RetouchFiltersObjects.h`, `DiagnosticFiltersObjects.h` | Clone/Heal/Remove spots, red/pet eye, spot inspection, owned mask previews and proof-space warnings |
| `MaskObjects.h`, `AnalysisObjects.h` | Owned mask production/combination/refinement, WB/Light/monochrome estimation, histograms, CA/source/pupil/dust analysis and solved geometry |
| `CorrectionExtensionsObjects.h`, `PhotoOperationsObjects.h`, `LocalCorrectionsObjects.h` | Flat-field correction, color unmixing, solid/gradient/pattern fill, orientation/crop and owned masked RGB correction |
| `SampleFilterObjects.h` | Typed recovered Average/fill/Solarize over explicit sample-plane inputs and mutable outputs |
| `RawObjects.h`, `ResourceBuilderObjects.h` | Unpacked Bayer development/rendering, owned Cube text, LUT construction, look/preset/lens recipes and plane analysis |

The complete type/function/header/ID mapping is installed as
[`object-catalog.json`](object-catalog.json). `operationCatalog()` returns stable
metadata for the same 309 objects without a global mutable registry or a
string-to-algorithm dispatcher. Several feature objects intentionally select
different controls of one existing processing kernel. 309 objects do not mean
309 newly recovered algorithms. Pixel converters retain their low-level free-function contracts. `SampleFilterObjects.h`
additionally objectifies all eight recovered Average/fill/Solarize sample-plane
operations with explicit typed call-scoped input/output views; these retain the
original mutable-buffer semantics and do not enter the normalized-image stack.

## Construction, editing and ownership

Concrete objects are editable values with an `options` member or directly named
fields. `apply(ImageView)` returns detached `Result<Image>` storage. Callers may
reuse and edit an object between calls. Mask producers return `Result<Mask>` via
`generate`; analysis objects return the actual typed estimate via `analyze`.
`RawDevelopment` takes `BayerImageView`, not encoded pixels. `HdrToning` retains
both rendered and linear-HDR overloads. Resource objects use `build`, `resolve`,
`sample` or `map` as appropriate; they are not artificial image filters.

Long-lived objects own their arrays, LUTs, masks and auxiliary images. In
particular:

- `MatchColor`, `Displace`, `CrossGuidedBlur`, `Blend`, `PatternFill` and
  `FlatFieldCorrection` own their reference images.
- `LensBlurParameters::depthMap` owns an optional `Image`; it does not retain
  `LensBlurOptions::depthMap`'s borrowed view.
- `VanishingPointParameters` owns every paste image, plane, tear-off and clone
  stroke. Borrowed free-function views are reconstructed only during a call.
- `GamutWarningParameters` owns the optional pre-gamut linear Rec.2020 triples.
- `ApplyLocalAdjustments` owns its masks and slider settings.
- `ApplyMaskedCorrection` owns both the rendered correction and its mask.

`Operation{object}` captures an immutable value snapshot. Editing or destroying
the original object cannot change that snapshot. Copying an `Operation` shares
the immutable capture through `shared_ptr`; no buffer is copied per render.
Creating/copying ordinary concrete values and capturing them follow normal C++
allocation/exception rules. Processing allocations are returned as
`AllocationFailure`. There is no thread, timer, GUI, Rust runtime or neural model.
Concurrent reads of a capture are safe while caller-owned input pixels remain
alive and immutable; concurrent edits to the same concrete value/stack are the
caller's responsibility. Custom objects may satisfy `ImageOperationObject` with
static-lifetime `info` strings and an expected-returning const `apply` method.

## Ordered stack and local application

```cpp
#include <iiGraphicAdjust/Objects.h>
using namespace iiGraphicAdjust;
namespace fx = iiGraphicAdjust::objects;

fx::Exposure exposure;
exposure.options.stops = 0.7;
fx::ColorGrading grading;
grading.options.shadows = {215, 12, 0};
fx::Sharpening detail;
detail.amount = 55;
detail.radius = 1.2;

OperationStack edits{{
    {Operation{exposure}},
    {Operation{grading}},
    {Operation{detail}}
}};
auto rendered = edits.evaluate(source.view());
if (!rendered) {
    // rendered.error(): stage, operation ID/name/family, and original SDK cause.
    return;
}
```

Up to 256 stages execute in caller order. Full stages may change geometry;
later masks must match the image as it exists at that stage. A partial/masked
stage must keep dimensions. Its weight is `amount * coverage`, where `amount`
is `[0,1]`. `space` selects encoded sRGB or linear sRGB interpolation.
This is interpolation between two images: alpha and RGB are premultiplied while
mixing. It is not source-over layering. A fully applied result is copied exactly;
zero coverage preserves original bits. When both interpolated alphas are zero,
the input hidden RGB remains. Use the existing `ApplyMaskedCorrection` object
instead when original alpha must remain fixed independently of corrected alpha.

`enabled=false` bypasses the kernel. Stage amount, space and mask are still
validated. A zero-amount enabled stage still executes/validates its kernel and
geometry, then keeps the current pixels. Source input and every enabled
operation output are validated. Failure returns no partially rendered result
and never mutates source storage. `evaluate` reports stage index/metadata;
`apply` returns the underlying `Error` for callers using the common result type.
An empty stack produces a detached copy. A moved-from handle is an explicit
invalid-parameter error.

## New correction kernels

`ColorToAlpha` independently implements minimum-opacity unmixing in encoded
sRGB. For key `C`, source `P`, it chooses the smallest `a` with an in-range
`Q = C + (P-C)/a`. Recomposition over the key recovers the source with neutral
thresholds, including existing source alpha. Thresholds are independently in
`[0,1]`; equal/crossed thresholds form a deterministic step and never divide by
zero. Already invisible pixels retain their RGB. The current PhotoCraft tree
has this extra filter; its command source explicitly identifies it as an
extension absent from Photoshop, not proof of an Adobe feature.

`FlatFieldCorrection` accepts a same-sized decoded flat exposure. It estimates
alpha-weighted per-channel reference means and divides scene-linear RGB by the
reference response with a configurable positive floor. Luminance-only gain is
available. Strength interpolates correction gains in linear light. Reference
holes over visible source pixels are errors. Source alpha/hidden RGB remain.
Output is normalized SDR; this is not Lightroom's RAW/DNG metadata workflow or
a mechanism for recovering clipped sensor values.

`SolidFill`, `GradientFill` and `PatternFill` render real pixels. Gradients have
Linear/Radial/Angle/Reflected/Diamond geometry, strictly ordered RGB stops,
center/angle/scale/reverse and explicit transfer-space interpolation. Pattern
fill owns a tile and wraps premultiplied bilinear samples after scale/rotation/
pixel-offset transforms. Fill `preserveAlpha=true` multiplies/retains source
coverage as documented by each option; false allows a fully independent fill.
These are pixel operations, not document-layer or Photoshop pattern-file readers.

## Active detail controls and ABI

`DevelopDetailOptions` now also reads `sharpenRadius` `[.5,3]`, `sharpenDetail`
`[0,100]` and `luminanceContrast` `[0,100]`. Radius multiplies the existing
image-relative Gaussian detail scale; it is not a claim of Adobe pixel units.
Fine detail adds `(detail-25)/75 * .5` times a half-radius log-luminance band.
Luminance NR contrast restores the difference between pre/post-NR broad Gaussian
bases before blending the log-luminance correction. Default `1/25/0` retains
the preceding algorithm and existing numerical oracles. All controls are
validated even when their correction amount is zero. `Sharpening`,
`LuminanceNoiseReduction` and `ColorNoiseReduction` expose these independently.

This changes public struct layouts, including embedded `DevelopOptions`.
The package and shared-library ABI series therefore move to **0.6**. Rebuild
consumers using `find_package(iiGraphicAdjust 0.6 CONFIG REQUIRED)`. No active
application or user's global installed SDK is replaced by this change. Package
checks install to isolated subdirectories under each `build/` tree.

## Adobe coverage boundaries

[`adobe-feature-coverage.json`](adobe-feature-coverage.json) records current
category mappings, caller-supplied-data requirements and unimplemented areas.
This is an explicit partial Adobe implementation, even though the pre-existing
CPU processing API is fully objectified. There are no placeholder neural
objects that silently substitute ordinary blur/noise for a learned model.

Remaining areas include neural portrait/semantic restoration/colorization/style
transfer, generative removal/fill, automatic subject/sky/people/depth estimation,
AI RAW Denoise/Enhance/Super Resolution, reflection removal, complete HDR/wide-
gamut managed development, Adobe camera/lens profile registries, proprietary
preset/document interchange, panorama/HDR merge and original-application pixel
equivalence. Supplied depth maps and masks work through the existing CPU
operations; their semantic inference is not included. Classic statistical NR,
resampling and patch-based spot repair are implemented under their own names.

Feature categories were checked against Adobe's
[adjustment reference](https://helpx.adobe.com/photoshop/desktop/create-manage-layers/color-adjustment-fill-layers/adjustment-layers-options.html),
[Neural Filters reference](https://helpx.adobe.com/photoshop/desktop/effects-filters/neural-filters/neural-filters-gallery.html),
[Lightroom tone/color reference](https://helpx.adobe.com/lightroom-classic/desktop/process-and-develop-photos/image-tone-color.html)
and [Lightroom development index](https://helpx.adobe.com/lightroom-classic/desktop.html)
on 2026-10-19. These establish categories rather than numerical compatibility.
See [OperationObjectsVerification.md](OperationObjectsVerification.md) for
actual build, execution, sanitizer and isolated-consumer evidence.
