# iiGraphicAdjust

Sources and adjacent headers are grouped by responsibility under `src/`; see [source layout](docs/SourceLayout.md). Public include names remain stable.

[Windows build and execution](docs/WindowsBuildAndRun.md)

Package release: **0.6.1** (2026-10-19). This maintenance release follows the
concurrently completed 0.6.0 object API implementation. Build, full tests and
installation are verified together against this final source snapshot.


Independent C++23 CPU SDK for raster color adjustment, image effects and alpha compositing. Version **0.6.1**. Processing uses the C++ standard library and caller-provided image data; the library neither creates files nor depends on other SDK modules. Applications provide file loading and publishing separately, for example through iiFileProvider. There is no Qt dependency.

## Features

Version **0.6.0** adds **309 typed value objects** for the full existing processing
surface, owned resources, automatic analysis, masks, RAW and LUT/profile recipes.
`Operation` captures an immutable object snapshot and `OperationStack` applies
up to 256 ordered stages with owned masks, transfer-space-aware premultiplied
interpolation and precise failure locations. Five new kernels add flat-field
correction, color-to-alpha unmixing and solid/gradient/pattern fills. Sharpen
Radius/Detail and Luminance NR Contrast now perform real processing.

See [object API and contracts](docs/OperationObjects.md), the
[complete object inventory](docs/object-catalog.json) and
[Adobe coverage and remaining features](docs/adobe-feature-coverage.json).
All existing CPU operations are objectified; Adobe neural/generative functions,
full RAW/HDR color workflows and Adobe rendering equivalence remain incomplete.
This is not a recovery of Adobe's original source. Public detail settings changed
layout, so **0.6 consumers must rebuild**. Validation installs only to `build/`.

Version 0.5.1 adds two explicitly separate processing contracts without changing existing correction models or the ABI series:

- `applyMaskedCorrection()` applies the RGB of any same-sized correction result through caller-supplied float coverage, with amount `[0,100]`, optional inversion and encoded/linear sRGB interpolation. It preserves base alpha and hidden RGB, validates inactive inputs too, and returns detached storage. Existing Levels, Curves, Kelvin WB, HSL, Detail and Camera Raw results can be localized without importing a brush SDK. This composition is an independent implementation; recovered Lightroom local-curve/control declarations establish the functional gap, not an Adobe pixel oracle.
- `SampleFilters.h` separately exposes the evidence-bound Photoshop Average kernels for 8-bit, 16-bit and float32 sample planes and Solarize for 8/16-bit rows. Average preserves the recovered tile/mask/rounding and float accumulation behavior; Solarize retains the observed integer scales and 16-bit wrapping. These mutable sample-plane contracts differ from normalized `ImageView`, and the existing independent `average(ImageView)`/`solarize(ImageView)` APIs retain their previous behavior. Strict floating-point source flags preserve the measured accumulation order.

See [masked correction composition](docs/LocalCorrections.md), [recovered sample-plane kernels](docs/SampleFilters.md) and [local correction verification](docs/LocalCorrectionsVerification.md). Native sample-kernel replay evidence is bounded to those kernels; the SDK does not claim to reconstruct Adobe's complete rendering engine.

Version 0.5.0 adds a sensor and photograph processing foundation without changing the existing rendered-image filters:

- `developRaw()` accepts caller-decoded 16-bit Bayer samples with row stride, four CFA patterns and crop phase. It subtracts per-tile black levels, normalizes by white level, bilinearly demosaics, applies supplied white balance and a camera-to-linear-sRGB matrix, and preserves highlight headroom above one.
- `renderRaw()` composes the new linear output with the existing HDR toning function. RAW container decoding and physical camera calibration remain the caller's responsibility.
- `applyOrientation()` handles all eight EXIF orientations; `crop()` performs exact bounded integer cropping.
- `imageHistogram()` reports encoded RGB and linear-luminance bins/statistics with explicit alpha weighting.

See [RAW processing](docs/RawProcessing.md) and [photographic operations](docs/PhotoOperations.md). These leaf APIs introduce no dependency on a camera SDK. An optional upper layer in iiCameraIntegration composes captured frames with these functions and the existing development/resampling stages.

Version 0.4.0 adds individual filtering APIs selected by re-inspecting both supplied repositories. [The complete filter inventory](docs/Filters.md) and [machine-readable catalog](docs/filter-catalog.json) map every executable command to a public function and pinned source.

- PhotoCraft: all 120 executable Filter-menu operations, including 47 individual Gallery effects, plus the two Edit transforms that can be recorded as smart filters. Blur/Blur Gallery, Sharpen, Noise, Distort, Pixelate, Stylize, Render, Video and Other families have separate typed APIs.
- Special filters: relative-temperature Camera Raw, encoded Lens Correction, dense-field Liquify, spherical/constraint-based Adaptive Wide Angle, metric-plane Vanishing Point, cotangent-ARAP Puppet Warp and linked-plane Perspective Warp. Geometry helpers expose mesh/pin/constraint results to hosts.
- Gallery: the 47 effects retain individual functions/options; a typed `GalleryEffect` stack applies them in order.
- LightCraft: 22 named looks, 41 named built-in presets, Box/Bilinear/Mitchell/Lanczos3 resampling, and actual Clone/Heal/Remove, Red Eye/Pet Eye, spot visualization and dust detection kernels. Point Color range preview, five mask composites and destination/display gamut warnings are separate diagnostic APIs.
- Reusable alpha-weighted box-guided and cross-guided filtering. Original linear-light `gaussianBlur`, `sharpen` and normalized `addNoise` behavior is preserved alongside the new encoded-space menu functions.


Version 0.3.0 adds 27 public Lightroom-style development and masking operations, selected from the actual LightCraft processing code. Details, source mapping and intentional differences are in [Lightroom development](docs/Lightroom.md).

- White balance: Kelvin/tint Bradford adaptation, six presets, Auto and sampled neutralization.
- Light: Exposure, Contrast, Highlights, Shadows, Whites, Blacks, Auto Light; parametric and cubic RGB/master curves with Refine Saturation.
- Color: skin-protected Vibrance/Saturation, eight-band Mixer/B&W/Auto B&W, Point Color, four-wheel grading and Calibration.
- Effects/detail: Texture, Clarity, Dehaze, three vignette styles, seeded film grain, masked detail sharpening and luminance/color noise reduction.
- Optics: distortion, lens vignette compensation, manual/automatic lateral CA, purple/green defringe and host-supplied embedded lens coefficients.
- Local adjustments: linear/radial/brush/luminance/color masks, combine/invert/edge refinement, 17 accumulated local sliders and color tint.
- `develop()` composes the standalone SDR stages. Each stage normalizes to sRGB; this is not LightCraft's full scene-linear RAW pipeline.

Version 0.2.0 adds APIs corresponding to the 22 classic Photoshop/PhotoCraft adjustment categories and three automatic corrections. This is independent processing with documented numeric contracts, not Photoshop binary or pixel compatibility.

- Tone: Brightness/Contrast, per-channel Levels and Curves, Exposure with offset/gamma, Shadows/Highlights, Invert, Posterize, Threshold, Equalize, Auto Levels/Auto Tone, Auto Contrast and Auto Color.
- Color: targeted Hue/Saturation and colorize, Vibrance, tone-range Color Balance, six-color Black & White, Photo Filter, Channel Mixer, nine-range Selective Color, Desaturate, Replace Color and reference-image Match Color.
- Mapping: Gradient Map and Color Lookup with 1D/3D `.cube` parsing, trilinear/tetrahedral interpolation and optional ordered dither.
- HDR Toning: linear HDR input above one, ExposureGamma and HighlightCompression, normalized sRGB output.

The original operations remain available with their 0.1 semantics:

- Color adjustment: linear-light exposure, relative temperature/tint, brightness, contrast, saturation, gamma and opacity.
- Gaussian blur with linear-light premultiplied-alpha filtering, and unsharp-mask sharpening.
- Seeded uniform or Gaussian noise, monochromatic or independent RGB.
- Source-over compositing in sRGB or linear sRGB with Normal, Multiply, Screen, Overlay, Darken, Lighten, SoftLight and Difference blend modes.
- Conversion between normalized straight-alpha RGBA floats, RGBA8 bytes and numeric `0xAARRGGBB` words.

Normalized image and mask processing returns new storage and leaves inputs unchanged. Public results use `std::expected`; image/mask operations retain `Result<T>` and the existing `Error` type. `SampleFilters` fill/Solarize operations explicitly write caller-owned mutable sample planes after validation and use their separate `SampleFilterError`. There are no placeholder processing functions or implicit product/document writes.

## Build and test

CMake 3.24 or later and a C++23 compiler/standard library with `std::expected` are required. No third-party packages are discovered or linked.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release
```

The standalone default installation prefix is `~/.local/SDK/iiGraphicAdjust`. Set `-DCMAKE_INSTALL_PREFIX=/your/prefix` to override it. Shared libraries are enabled by default; `-DBUILD_SHARED_LIBS=OFF` builds a static library. Set `-DBUILD_TESTING=OFF` for a library-only build. The examples are separately configured applications rather than targets in the SDK build.

This module is automatically discovered by the existing parent SDK installer:

```sh
python3 -B ../install_all.py --only iiGraphicAdjust --dry-run
python3 -B ../install_all.py --only iiGraphicAdjust
```

Builds, staged packages, consumer checks and generated examples stay under `build/`.

## Consume the installed package

```cmake
find_package(iiGraphicAdjust 0.6 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE iiGraphicAdjust::iiGraphicAdjust)
```

Configure the consumer with `-DiiGraphicAdjust_DIR="$HOME/.local/SDK/iiGraphicAdjust/lib/cmake/iiGraphicAdjust"` or add that SDK prefix to `CMAKE_PREFIX_PATH`. The target exports the C++23 requirement, include directories and static/shared export definitions.

```cpp
#include <iiGraphicAdjust/iiGraphicAdjust.h>
#include <array>

std::array<std::uint32_t, 2> source{0xff808080, 0x804080c0};
auto image = iiGraphicAdjust::fromArgb32(2, 1, source);
if (!image) {
    // Handle image.error().code and image.error().message.
    return;
}

iiGraphicAdjust::ColorAdjustment adjustment;
adjustment.exposure = 1.0;
adjustment.saturation = 0.15;
auto adjusted = iiGraphicAdjust::adjust(image->view(), adjustment);
if (!adjusted) return;

auto blurred = iiGraphicAdjust::gaussianBlur(adjusted->view(), 1.5);
if (!blurred) return;
auto words = iiGraphicAdjust::toArgb32(blurred->view());
if (!words) return;
// Consume *words; the original source has not been modified.
```

`ImageView` borrows the caller's packed row-major pixels. Keep that storage alive for the complete call and avoid mutating it concurrently. `Image` owns the returned pixels. Public structs are value types without inheritance or GUI objects.

## Example

```sh
cmake -S examples -B build/examples \
  -DCMAKE_PREFIX_PATH="$HOME/.local/SDK/iiGraphicAdjust;$HOME/.local/SDK/iiFileProvider"
cmake --build build/examples
build/examples/iiGraphicAdjustExample resources/adjusted.ppm
```

The original example and `iiGraphicAdjustDevelopExample` write real 256×128 adjusted RGB gradients as binary PPM through iiFileProvider. The application links the two independent SDKs explicitly. The development example also applies a radial local adjustment. File encoding is confined to examples; the library processes caller-provided pixels.

`iiGraphicAdjustLocalCorrectionsExample` computes Kelvin white balance first, then applies that result only through a feathered radial mask with 85% amount:

```sh
build/examples/iiGraphicAdjustLocalCorrectionsExample resources/local-correction.ppm
```

## Scope

Rendered-image functions process normalized sRGB on the CPU. `developRaw()` separately processes unpacked Bayer sensor samples and produces linear sRGB with HDR headroom; a separate `HdrImageView` accepts linear HDR radiance for tone mapping. Premultiplied input, wide-gamut/ICC conversion, encoded RAW/image-file decoding, GPU execution, general vector drawing and text shaping remain outside the contract. Legacy `ColorAdjustment` temperature/tint retain their relative channel-gain semantics; the new `whiteBalance()` uses Kelvin/tint chromatic adaptation. Legacy noise modes are Uniform and Gaussian; `filmGrain()` adds a distinct correlated procedural model. No blue-noise claim is made. `develop()` normalizes between SDR stages and does not retain the reference's RAW scene headroom.

The module is ready for a caller to use through its installed CMake package. Dreamscapes and existing SDK processing paths are not migrated by creating this package. Cross-platform source portability does not imply a verified Windows, Linux, iOS or Android build.

See the [filter catalog and contracts](docs/Filters.md), original [API contract](docs/API.md), [tone](docs/Tone.md), [color](docs/Color.md), [mapping/HDR](docs/Mapping.md), [Lightroom development](docs/Lightroom.md), [masked corrections](docs/LocalCorrections.md), [sample-plane kernels](docs/SampleFilters.md), [Photoshop reference coverage](docs/References.md), [architecture](docs/Architecture.md) and [verification](docs/Verification.md).

Version 0.6 uses shared-library ABI series 0.6 and `SameMinorVersion` CMake compatibility. Rebuild consumers and request package version 0.6 for the current public APIs; existing 0.1, 0.2 and 0.3 function behavior is preserved. The current sensor/photo verification is recorded in [PhotoProcessingVerification](docs/PhotoProcessingVerification.md).

## License

SPDX-License-Identifier: MIT

The original code and documentation in this repository are licensed under the
[MIT License](LICENSE). Third-party code, libraries, fonts, tools, and model
weights retain their respective licenses.

<!-- workspace-docs:readme:start -->
## Documentation entry points

- Human overview: this English `README.md`.
- Agent instructions: [AGENT.md](AGENT.md).
- Human documentation: [documentation index](docs/README.md).
- Structured agent documentation: [agent index](docs/agent-index.json).
<!-- workspace-docs:readme:end -->
