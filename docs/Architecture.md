# Architecture

`iiGraphicAdjust` is a processing SDK built on iiSDKCore Contracts. Kernels use C++23 and the standard library and receive caller-owned image data. Product state, GUI types, document persistence, font shaping and file decoding remain responsibilities of callers. Applications may combine image processing with iiFileProvider without linking those two SDKs into each other.

## Change boundaries

- File: `RawProcessing.h/.cpp`; Responsibility: Unpacked Bayer normalization, demosaicing and supplied sensor calibration
- File: `PhotoOperations.h/.cpp`; Responsibility: Integer orientation/crop and alpha-aware photo histograms
- File: `Image.h/.cpp`; Responsibility: Borrowed/owned pixel contract and byte/word conversion
- File: `Adjustments.h/.cpp`; Responsibility: Color and opacity adjustments
- File: `Effects.h/.cpp`; Responsibility: Blur, unsharp mask and deterministic noise
- File: `Blend.h/.cpp`; Responsibility: Blend modes, working space and alpha composition
- File: `Tone.h/.cpp`; Responsibility: Tonal, discrete and automatic corrections
- File: `Color.h/.cpp`; Responsibility: Color-range operations and reference statistics
- File: `Mapping.h/.cpp`; Responsibility: Gradient mapping, LUT tables and strict Cube parsing
- File: `Hdr.h/.cpp`; Responsibility: Separate linear HDR input and normalized tone-mapped output
- File: `BlurFilters.h/.cpp`; Responsibility: Menu blur/sharpen and typed Blur Gallery pins/paths
- File: `DistortionFilters.h/.cpp`; Responsibility: Inverse mapping, displacement maps and border handling
- File: `UtilityFilters.h/.cpp`; Responsibility: Menu noise, morphology, convolution, video and box-guided kernels
- File: `PixelFilters.h/.cpp`; Responsibility: Pixelate and Stylize algorithms
- File: `RenderFilters.h/.cpp`; Responsibility: Procedural noise/light/Flame/Tree/Frame raster processing
- File: `GalleryFilters.h/.cpp`; Responsibility: 47 named artistic effects and typed ordered Gallery stack
- File: `LensFilter.h/.cpp`; Responsibility: PhotoCraft encoded Lens Correction, profiles and auto scale
- File: `CameraRawFilter.h/.cpp`; Responsibility: Separate relative-WB rendered Camera Raw pipeline
- File: `MeshFilters.h/.cpp`; Responsibility: Liquify fields, constrained projections, ARAP and metric planes
- File: `LightcraftFilters.h/.cpp`; Responsibility: 22 look/41 preset recipes and four linear-premultiplied resamplers
- File: `RetouchFilters.h/.cpp`; Responsibility: Clone/Heal/Remove, eye repair, spot visualization and dust detection
- File: `DiagnosticFilters.h/.cpp`; Responsibility: Selection/mask previews and explicit proof-space gamut warning overlays
- File: `FilterInternal.h`, `PixelRenderInternal.h`; Responsibility: Private encoded-space sampling, windows and procedural helpers
- File: `Internal.h`; Responsibility: Private validation, allocation handling and transfer functions
- File: `DevelopTone.h/.cpp`; Responsibility: Rendered-image white balance, Light, auto estimation and cubic/parametric curves
- File: `DevelopColor.h/.cpp`; Responsibility: Perceptual eight-band color, grading, Point Color and calibration
- File: `DevelopEffects.h/.cpp`; Responsibility: Spatial detail, denoise, dehaze, vignette and grain
- File: `Optics.h/.cpp`; Responsibility: Alpha-aware lens resampling, CA estimation and defringe
- File: `Masks.h/.cpp`; Responsibility: Normalized coverage generation, combination, refinement and local adjustments
- File: `LocalCorrections.h/.cpp`; Responsibility: Caller-supplied corrected RGB composed through normalized coverage, without alpha composition or correction-kernel dependencies
- File: `MaskInternal.h`; Responsibility: Shared private mask storage/coverage validation used by actual mask and correction consumers
- File: `SampleFilters.h/.cpp`; Responsibility: Separately typed recovered sample-plane Average/Solarize kernels with explicit mutable outputs and strict accumulation order
- File: `Develop.h/.cpp`; Responsibility: Explicit composition of the existing public development stages
- File: `DevelopInternal.h`; Responsibility: Shared private linear/OkLab math and alpha-weighted spatial primitives

Synchronous free functions retain value settings and borrowed const spans. Version 0.6 additionally provides concrete final value objects under `objects/`, with owned arrays and auxiliary resources. `Operation` uses a small const type-erased capture for immutable snapshots; virtual dispatch occurs once per image operation, not per pixel. `OperationStack` composes those captures in caller-selected order and reports precise failures. There is no algorithm backend, plugin factory or application history dependency. New numerical behavior remains in the kernel family owning its change reason. See [OperationObjects.md](OperationObjects.md).

## Storage and concurrency

Normalized image and mask outputs are always detached. Allocation errors become expected errors; these outputs are never partially written into a caller buffer. Reads and settings are immutable during a call, and all temporary storage/random state is local. `SampleFilters` separately declares mutable fill/Solarize sample planes, validates geometry before writes, and documents aliasing and exceptional floating-point input behavior. Separate calls may run concurrently when borrowed source storage stays alive and immutable and mutable sample outputs do not overlap. There is no global cache, worker thread or GUI event loop.

Color adjustment, noise, composition and format conversion cost O(N) for N pixels. Separable blur and sharpening cost O(N * ceil(3*sigma)) and use O(N) temporary memory. Blur keeps two double-precision premultiplied sample buffers plus the float output, approximately 80 bytes per pixel at peak, excluding caller storage. Sharpening also retains its detached output, adding approximately 16 bytes per pixel during blur. This reference CPU implementation prioritizes explicit image semantics; no benchmark or real-time throughput guarantee is made.

## Packaging

The package target is `iiGraphicAdjust::iiGraphicAdjust`; CMake propagates C++23 and the installed public include path. `GenerateExportHeader` supports shared-library visibility and static definitions. ABI series 0.6 uses `SameMinorVersion` package compatibility. Consumers must use a compatible compiler/standard-library ABI when crossing a shared-library boundary with standard containers.

The default install prefix is `~/.local/SDK/iiGraphicAdjust`. The parent `SDK/install_all.py` discovers this directory from `CMakeLists.txt`; no central module list or duplicated install script is needed. The exported package discovers iiSDKCore 0.20 Contracts as its common foundation. The processing library does not link iiFileProvider; installed consumers verify the package and foundation together.

No existing product or SDK adapter is rewritten in this release. Integrating a host means adapting its actual pixel format and ownership contract to `ImageView` or the documented converters, then calling the desired operations explicitly.

Automatic corrections sort visible samples and cost O(N log N); Equalize uses fixed-bin histograms. Match Color computes alpha-weighted statistics without storing borrowed views. LUT interpolation uses fixed per-pixel working arrays; 1D LUTs retain direct tables. Upstream reference attribution is installed with the package and adds no library dependency.

Development spatial windows use double precision and alpha-weighted statistics. Small Gaussian radii use exact separable convolution; sigma >= 3 uses three variance-matched box passes with O(N) work. Guided filters reuse these window primitives. Defringe uses O(N) sliding extrema. The measured 4K Light operation retained about 1.18 GiB peak resident memory; this CPU reference implementation has no low-memory or real-time guarantee. Local masks borrow coverage synchronously, accumulate settings without retaining pointers, and have explicit per-call bounds. Geometric operations document their separate alpha-resampling contract. `develop()` is explicit SDR stage composition, not a hidden RAW scene pipeline.

Masked correction composition uses O(N) work and one detached `Rgba` output, with no intermediate full-image RGB planes. Its amount scales mask coverage before pointwise RGB interpolation; it does not accumulate local slider parameters. The corrected image is supplied by the caller, so new correction kernels or brush producers require no delegation function or external SDK dependency here. Historical patch 0.5.1 did not change public struct layouts; 0.6 adds active detail fields and requires consumer rebuilding.

Filter families are split by their source algorithms and reasons to change, with explicit value options instead of command-name/string dispatch. Header-only private math is shared only where actual repeated sampling/alpha requirements exist. Geometry helper outputs let an editor inspect a real solved mesh without making the leaf SDK depend on a product. There is no plugin or application history abstraction in this SDK.

Filter inventories are pinned to actual upstream commits and are distributed as `docs/filter-catalog.json`. Smart-object aliases share an implementation; 47 Gallery functions are not collapsed into a single style switch. Resampling uses linear premultiplied RGB with antialiasing widened kernels, whereas PhotoCraft artistic/menu filters use encoded sRGB. Those working-space distinctions are explicit API contracts.
