# iiGraphicAdjust

Independent C++23 CPU SDK for raster color adjustment, image effects and alpha compositing. Version **0.1.0**. The public API and implementation use only the C++ standard library. There is no Qt, LVRS, iiPaintEngine or iiSharedCanvas dependency, including transitive dependencies.

## Features

- Color adjustment: linear-light exposure, relative temperature/tint, brightness, contrast, saturation, gamma and opacity.
- Gaussian blur with linear-light premultiplied-alpha filtering, and unsharp-mask sharpening.
- Seeded uniform or Gaussian noise, monochromatic or independent RGB.
- Source-over compositing in sRGB or linear sRGB with Normal, Multiply, Screen, Overlay, Darken, Lighten, SoftLight and Difference blend modes.
- Conversion between normalized straight-alpha RGBA floats, RGBA8 bytes and numeric `0xAARRGGBB` words.

All processing returns new storage and leaves the input unchanged. Public results use `std::expected<T, Error>`. There are no placeholder processing functions or implicit product/document writes.

## Build and test

CMake 3.24 or later and a C++23 compiler/standard library with `std::expected` are required. No third-party packages are discovered or linked.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release
```

The standalone default installation prefix is `~/.local/SDK/iiGraphicAdjust`. Set `-DCMAKE_INSTALL_PREFIX=/your/prefix` to override it. Shared libraries are enabled by default; `-DBUILD_SHARED_LIBS=OFF` builds a static library. Set `-DIIGRAPHICADJUST_BUILD_EXAMPLES=OFF` and/or `-DBUILD_TESTING=OFF` for library-only builds.

This module is automatically discovered by the existing parent SDK installer:

```sh
python3 -B ../install_all.py --only iiGraphicAdjust --dry-run
python3 -B ../install_all.py --only iiGraphicAdjust
```

Builds, staged packages, consumer checks and generated examples stay under `build/`.

## Consume the installed package

```cmake
find_package(iiGraphicAdjust 0.1 CONFIG REQUIRED)
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
build/iiGraphicAdjustExample build/adjusted.ppm
```

The example writes a real 256×128 adjusted RGB gradient as binary PPM using standard C++ file I/O. File encoding is confined to the example; the library processes caller-provided pixels.

## Scope

This release processes normalized sRGB images on the CPU. Premultiplied input, HDR/wide-gamut/ICC conversion, image decoding, GPU execution, vector geometry and text shaping are outside its contract. Temperature/tint are relative channel gains, not calibrated photographic white balance. Noise modes are Uniform and Gaussian; no blue-noise claim is made.

The module is ready for a caller to use through its installed CMake package. Dreamscapes and existing SDK processing paths are not migrated by creating this package. Cross-platform source portability does not imply a verified Windows, Linux, iOS or Android build.

See [API contract](docs/API.md), [architecture](docs/Architecture.md) and [verification](docs/Verification.md).
