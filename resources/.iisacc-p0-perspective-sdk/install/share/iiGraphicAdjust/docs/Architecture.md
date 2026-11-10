# Architecture

`iiGraphicAdjust` is an independent processing SDK. Kernels use C++23 and the standard library and receive caller-owned image data. Product state, GUI types, document persistence, font shaping and file decoding remain responsibilities of callers. Applications may combine image processing with iiFileProvider without linking one SDK into the other.

## Change boundaries

| File | Responsibility |
| --- | --- |
| `RawProcessing.h/.cpp` | Unpacked Bayer normalization, demosaicing and supplied sensor calibration |
| `PhotoOperations.h/.cpp` | Integer orientation/crop and alpha-aware photo histograms |
| `Image.h/.cpp` | Borrowed/owned pixel contract and byte/word conversion |
| `Adjustments.h/.cpp` | Color and opacity adjustments |
| `Effects.h/.cpp` | Blur, unsharp mask and deterministic noise |
| `Blend.h/.cpp` | Blend modes, working space and alpha composition |
| `Tone.h/.cpp` | Tonal, discrete and automatic corrections |
| `Color.h/.cpp` | Color-range operations and reference statistics |
| `Mapping.h/.cpp` | Gradient mapping, LUT tables and strict Cube parsing |
| `Hdr.h/.cpp` | Separate linear HDR input and normalized tone-mapped output |
| `BlurFilters.h/.cpp` | Menu blur/sharpen and typed Blur Gallery pins/paths |
| `DistortionFilters.h/.cpp` | Inverse mapping, displacement maps and border handling |
| `UtilityFilters.h/.cpp` | Menu noise, morphology, convolution, video and box-guided kernels |
| `PixelFilters.h/.cpp` | Pixelate and Stylize algorithms |
| `RenderFilters.h/.cpp` | Procedural noise/light/Flame/Tree/Frame raster processing |
| `GalleryFilters.h/.cpp` | 47 named artistic effects and typed ordered Gallery stack |
| `LensFilter.h/.cpp` | PhotoCraft encoded Lens Correction, profiles and auto scale |
| `CameraRawFilter.h/.cpp` | Separate relative-WB rendered Camera Raw pipeline |
| `MeshFilters.h/.cpp` | Liquify fields, constrained projections, ARAP and metric planes |
| `LightcraftFilters.h/.cpp` | 22 look/41 preset recipes and four linear-premultiplied resamplers |
| `RetouchFilters.h/.cpp` | Clone/Heal/Remove, eye repair, spot visualization and dust detection |
| `DiagnosticFilters.h/.cpp` | Selection/mask previews and explicit proof-space gamut warning overlays |
| `FilterInternal.h`, `PixelRenderInternal.h` | Private encoded-space sampling, windows and procedural helpers |
| `Internal.h` | Private validation, allocation handling and transfer functions |
| `DevelopTone.h/.cpp` | Rendered-image white balance, Light, auto estimation and cubic/parametric curves |
| `DevelopColor.h/.cpp` | Perceptual eight-band color, grading, Point Color and calibration |
| `DevelopEffects.h/.cpp` | Spatial detail, denoise, dehaze, vignette and grain |
| `Optics.h/.cpp` | Alpha-aware lens resampling, CA estimation and defringe |
| `Masks.h/.cpp` | Normalized coverage generation, combination, refinement and local adjustments |
| `LocalCorrections.h/.cpp` | Caller-supplied corrected RGB composed through normalized coverage, without alpha composition or correction-kernel dependencies |
| `MaskInternal.h` | Shared private mask storage/coverage validation used by actual mask and correction consumers |
| `SampleFilters.h/.cpp` | Separately typed recovered sample-plane Average/Solarize kernels with explicit mutable outputs and strict accumulation order |
| `Develop.h/.cpp` | Explicit composition of the existing public development stages |
| `DevelopInternal.h` | Shared private linear/OkLab math and alpha-weighted spatial primitives |

Synchronous free functions retain value settings and borrowed const spans. Version 0.6 additionally provides concrete final value objects under `objects/`, with owned arrays and auxiliary resources. `Operation` uses a small const type-erased capture for immutable snapshots; virtual dispatch occurs once per image operation, not per pixel. `OperationStack` composes those captures in caller-selected order and reports precise failures. There is no algorithm backend, plugin factory or application history dependency. New numerical behavior remains in the kernel family owning its change reason. See [OperationObjects.md](OperationObjects.md).

## Storage and concurrency

Normalized image and mask outputs are always detached. Allocation errors become expected errors; these outputs are never partially written into a caller buffer. Reads and settings are immutable during a call, and all temporary storage/random state is local. `SampleFilters` separately declares mutable fill/Solarize sample planes, validates geometry before writes, and documents aliasing and exceptional floating-point input behavior. Separate calls may run concurrently when borrowed source storage stays alive and immutable and mutable sample outputs do not overlap. There is no global cache, worker thread or GUI event loop.

Color adjustment, noise, composition and format conversion cost O(N) for N pixels. Separable blur and sharpening cost O(N * ceil(3*sigma)) and use O(N) temporary memory. Blur keeps two double-precision premultiplied sample buffers plus the float output, approximately 80 bytes per pixel at peak, excluding caller storage. Sharpening also retains its detached output, adding approximately 16 bytes per pixel during blur. This reference CPU implementation prioritizes explicit image semantics; no benchmark or real-time throughput guarantee is made.

## Packaging

The package target is `iiGraphicAdjust::iiGraphicAdjust`; CMake propagates C++23 and the installed public include path. `GenerateExportHeader` supports shared-library visibility and static definitions. ABI series 0.6 uses `SameMinorVersion` package compatibility. Consumers must use a compatible compiler/standard-library ABI when crossing a shared-library boundary with standard containers.

The default install prefix is `~/.local/SDK/iiGraphicAdjust`. The parent `SDK/install_all.py` discovers this directory from `CMakeLists.txt`; no central module list or duplicated install script is needed. The exported package does not discover or link another SDK. Independence must be verified from source, exported CMake targets, linked binaries and a separately configured installed consumer.

No existing product or SDK adapter is rewritten in this release. Integrating a host means adapting its actual pixel format and ownership contract to `ImageView` or the documented converters, then calling the desired operations explicitly.

Automatic corrections sort visible samples and cost O(N log N); Equalize uses fixed-bin histograms. Match Color computes alpha-weighted statistics without storing borrowed views. LUT interpolation uses fixed per-pixel working arrays; 1D LUTs retain direct tables. Upstream reference attribution is installed with the package and adds no library dependency.

Development spatial windows use double precision and alpha-weighted statistics. Small Gaussian radii use exact separable convolution; sigma >= 3 uses three variance-matched box passes with O(N) work. Guided filters reuse these window primitives. Defringe uses O(N) sliding extrema. The measured 4K Light operation retained about 1.18 GiB peak resident memory; this CPU reference implementation has no low-memory or real-time guarantee. Local masks borrow coverage synchronously, accumulate settings without retaining pointers, and have explicit per-call bounds. Geometric operations document their separate alpha-resampling contract. `develop()` is explicit SDR stage composition, not a hidden RAW scene pipeline.

Masked correction composition uses O(N) work and one detached `Rgba` output, with no intermediate full-image RGB planes. Its amount scales mask coverage before pointwise RGB interpolation; it does not accumulate local slider parameters. The corrected image is supplied by the caller, so new correction kernels or brush producers require no delegation function or external SDK dependency here. Historical patch 0.5.1 did not change public struct layouts; 0.6 adds active detail fields and requires consumer rebuilding.

Filter families are split by their source algorithms and reasons to change, with explicit value options instead of command-name/string dispatch. Header-only private math is shared only where actual repeated sampling/alpha requirements exist. Geometry helper outputs let an editor inspect a real solved mesh without making the leaf SDK depend on a product. There is no plugin or application history abstraction in this SDK.

Filter inventories are pinned to actual upstream commits and are distributed as `docs/filter-catalog.json`. Smart-object aliases share an implementation; 47 Gallery functions are not collapsed into a single style switch. Resampling uses linear premultiplied RGB with antialiasing widened kernels, whereas PhotoCraft artistic/menu filters use encoded sRGB. Those working-space distinctions are explicit API contracts.
