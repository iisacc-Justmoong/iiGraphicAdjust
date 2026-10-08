# Verification

Run all checks from the module directory. Keep every build, consumer and generated file below `build/`.

## Behavior coverage

The original eight CTest suites cover invalid image/settings rejection, integer conversion round trips, color-adjustment reference vectors, all eight blend modes and partial alpha, Gaussian impulse response and transparent-edge color preservation, unsharp-mask threshold/alpha behavior, noise repeatability/distribution/independent sequences, and normalized-output/input-immutability invariants.

Version 0.2.0 has 33 suites: the original 8, 10 tonal, 11 color, 3 mapping and 1 HDR. Added checks cover master/channel ordering, curve endpoints/duplicate points, exact neutral copies, gamma/offset references, automatic-correction differences and quantile ties, six hue controls and hue selection, alpha-weighted matching statistics, selective-range locality, all six tetrahedra versus trilinear interpolation, Cube syntax/count/domain failures, HDR input above one, and tiny/subnormal RGB hue rotation. Invisible RGB and source alpha are tested separately from visible processing.

Tests explicitly check successful outputs before inspecting pixels. Seed-repeatability and image invariants do not replace numerical reference checks. The initial unimplemented entry points failed all eight suites; `build/tdd-red.log` records that local TDD step. The final passing logs are distinct evidence.

## Build and installed consumer

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/stage"
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --output-log build/shared-tests.log
cmake --install build
cmake -S tests/consumer -B build/consumer-shared/build \
  -DiiGraphicAdjust_DIR="$PWD/build/stage/lib/cmake/iiGraphicAdjust"
cmake --build build/consumer-shared/build --parallel 2
ctest --test-dir build/consumer-shared/build --output-on-failure
```

The consumer uses only installed public headers and the imported package, exercising conversion, adjustment, noise, blur, sharpen and SoftLight composition plus every one of the 25 new adjustment categories, Cube parsing and real linear HDR input. Repeat with `-DBUILD_SHARED_LIBS=OFF` in `build/static/build` and an independent staged prefix to check static export definitions. Version 0.2 requires `find_package(iiGraphicAdjust 0.2 CONFIG REQUIRED)`.

## Sanitizers

With a supported Clang/GCC toolchain:

```sh
cmake -S . -B build/sanitized/build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build/sanitized/build --parallel 2
ctest --test-dir build/sanitized/build --output-on-failure
```

## Host dependency verification

On macOS, inspect `otool -L` on the SDK dylib and installed consumer, and run the consumer with `DYLD_PRINT_LIBRARIES=1` to prove which SDK dylib actually loaded. Assert the loaded path matches the intended prefix and that no Qt framework/library appears. Source scans and imported CMake target inspection supplement this binary evidence. On Linux/Windows use the host's equivalent dependency inspector.

Physical verification recorded below is limited to the tested host and does not prove product integration or other platform builds.

## Historical 0.1.0 host result — 2026-10-08

macOS arm64 with AppleClang 21.0.0 and CMake 4.4.3:

| Configuration/check | Result |
| --- | --- |
| Release shared library behavior | 8/8 passed |
| Release static library behavior | 8/8 passed |
| Debug AddressSanitizer + UndefinedBehaviorSanitizer | 8/8 passed, no sanitizer findings |
| Shared staged-package consumer | 1/1 passed |
| Static staged-package consumer | 1/1 passed |
| Actual host-installed package consumer | 1/1 passed |
| `otool -L` SDK and consumer | C++/system libraries, no Qt dependency |
| `DYLD_PRINT_LIBRARIES=1` host consumer | Actual `~/.local/SDK/iiGraphicAdjust/lib/libiiGraphicAdjust.0.1.0.dylib` loaded; no Qt |
| Build vs host-installed SDK SHA-256 | Identical |
| Parent SDK installer discovery | `iiGraphicAdjust: dependencies=-` |
| Executed example output | Binary PPM, 256×128, 98,319 bytes |

The 27 successful test executions above include repeated behavior coverage across three build configurations, not 27 unique algorithms. The original machine-readable report is retained as `build/verification-0.1.0.json`; current verification is recorded in `build/verification.json`. Windows, Linux, iOS, Android and integration into Dreamscapes/iiSharedCanvas remain unverified. No existing running product was restarted.

## Recorded 0.2.0 adjustment result — 2026-10-08

The final source was validated on the same macOS arm64/AppleClang 21.0.0 host:

| Configuration/check | Result |
| --- | --- |
| Release shared library | 33/33 passed |
| Release static library | 33/33 passed |
| Debug AddressSanitizer + UndefinedBehaviorSanitizer | 33/33 passed, no findings |
| Shared and static staged-package consumers | 1/1 each passed |
| Actual host-installed package consumer | 1/1 passed |
| SDK and consumer linkage / actual loaded libraries | No Qt |
| Loaded SDK dylib | `~/.local/SDK/iiGraphicAdjust/lib/libiiGraphicAdjust.0.2.0.dylib`, no 0.1 SDK loaded |
| SDK build/installed SHA-256 and new public headers | Identical |
| Upstream MIT attribution | Installed with package |
| Parent SDK discovery | No local dependency edges |

This is 33 suites exercised in three configurations plus three package consumers, totalling 102 successful final test executions. The installed consumer resolves every new operation, Cube parsing and the separate HDR input overload against the installed package. Numeric behavior is assessed by the unit suites, not by a Photoshop render.

Independent static review identified an HSL cancellation at tiny lightness and malformed `+-1` Cube numbers. Regression tests reproduced the former before its fix (`build/hsl-regression-red.log`), and both cases now pass. Extremely narrow highlight masks and gray-preserving color replacement also have regression coverage. Source/build/test and actual installation proof are complete for this host; exact Photoshop rendering, upstream Rust tests, other platforms and product integration were not performed.
