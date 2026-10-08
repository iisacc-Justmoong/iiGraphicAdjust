# Architecture

`iiGraphicAdjust` is a standalone leaf SDK. It depends only on C++23 language and standard-library facilities. Product state, GUI types, document persistence, font shaping and file decoding remain responsibilities of its callers. The package introduces no reverse dependency on Dreamscapes, iiSharedCanvas or iiPaintEngine.

## Change boundaries

| File | Responsibility |
| --- | --- |
| `Image.h/.cpp` | Borrowed/owned pixel contract and byte/word conversion |
| `Adjustments.h/.cpp` | Color and opacity adjustments |
| `Effects.h/.cpp` | Blur, unsharp mask and deterministic noise |
| `Blend.h/.cpp` | Blend modes, working space and alpha composition |
| `Tone.h/.cpp` | Tonal, discrete and automatic corrections |
| `Color.h/.cpp` | Color-range operations and reference statistics |
| `Mapping.h/.cpp` | Gradient mapping, LUT tables and strict Cube parsing |
| `Hdr.h/.cpp` | Separate linear HDR input and normalized tone-mapped output |
| `Internal.h` | Private validation, allocation handling and transfer functions |

Operations are synchronous free functions over value settings and borrowed const spans. There are no virtual backends, factories, dependency injection frameworks or new container wrappers. Public value structs are final. A host can select processing order by passing each returned image to the next function, without a fixed pipeline object. New behavior belongs in the file that owns its change reason.

## Storage and concurrency

Outputs are always detached. Allocation errors become expected errors; no output is partially written into a caller buffer. Reads and settings are immutable during a call, and all temporary storage/random state is local. Separate calls may run concurrently when their source storage stays alive and immutable. There is no global cache, worker thread or GUI event loop.

Color adjustment, noise, composition and format conversion cost O(N) for N pixels. Separable blur and sharpening cost O(N * ceil(3*sigma)) and use O(N) temporary memory. Blur keeps two double-precision premultiplied sample buffers plus the float output, approximately 80 bytes per pixel at peak, excluding caller storage. Sharpening also retains its detached output, adding approximately 16 bytes per pixel during blur. This reference CPU implementation prioritizes explicit image semantics; no benchmark or real-time throughput guarantee is made.

## Packaging

The package target is `iiGraphicAdjust::iiGraphicAdjust`; CMake propagates C++23 and the installed public include path. `GenerateExportHeader` supports shared-library visibility and static definitions. ABI series 0.2 uses `SameMinorVersion` package compatibility. Consumers must use a compatible compiler/standard-library ABI when crossing a shared-library boundary with standard containers.

The default install prefix is `~/.local/SDK/iiGraphicAdjust`. The parent `SDK/install_all.py` discovers this directory from `CMakeLists.txt`; no central module list or duplicated install script is needed. The exported package declares no external library dependencies. Qt independence must be verified from source, exported CMake targets and linked binaries, rather than relying on an unused CMake disable-find option.

No existing product or SDK adapter is rewritten in this release. Integrating a host means adapting its actual pixel format and ownership contract to `ImageView` or the documented converters, then calling the desired operations explicitly.

Automatic corrections sort visible samples and cost O(N log N); Equalize uses fixed-bin histograms. Match Color computes alpha-weighted statistics without storing borrowed views. LUT interpolation uses fixed per-pixel working arrays; 1D LUTs retain direct tables. Upstream reference attribution is installed with the package and adds no library dependency.
