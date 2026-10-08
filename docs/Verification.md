# Verification

Run all checks from the module directory. Keep every build, consumer and generated file below `build/`.

## Behavior coverage

Version 0.3.0 adds 20 Lightroom development suites, for 53 total. Tests cover radiometric Exposure, WB inverse/presets/Auto, monotone curve knots and clipping, 3600 hues of mixer partition, B&W Point Color disablement, calibration neutral-axis preservation, effect/detail behavior, seed repeatability, lens metadata and CA edge handling, guided masks, normalized local accumulation and zero-coverage exact identity. Deterministic randomized settings, NaN/Inf/enum/size rejection, hidden-color isolation and cancellation regressions supplement fixed numeric oracles. The additional mask suite compares 32-mask Exposure accumulation with an independent linear-light oracle and verifies Auto Mask selection across both luminance and equal-luminance chromatic boundaries.

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

The consumer uses only installed public headers and the imported package, exercising conversion, adjustment, noise, blur, sharpen and SoftLight composition plus all 25 Photoshop-style categories and all 27 Lightroom-style public operations, Cube parsing and real linear HDR input. Repeat with `-DBUILD_SHARED_LIBS=OFF` in `build/static/build` and an independent staged prefix to check static export definitions. Version 0.3 requires `find_package(iiGraphicAdjust 0.3 CONFIG REQUIRED)`.

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

## Recorded 0.3.0 development result — 2026-10-08

The final Lightroom-style source was validated on macOS arm64 with AppleClang 21.0.0 and CMake 4.4.3:

| Configuration/check | Result |
| --- | --- |
| Release shared library | 53/53 passed |
| Release static library | 53/53 passed |
| Debug AddressSanitizer + UndefinedBehaviorSanitizer | 53/53 passed, no findings |
| Shared and static staged-package consumers | 1/1 each passed |
| Actual host-installed package consumer | 1/1 passed |
| Actual loaded SDK | `~/.local/SDK/iiGraphicAdjust/lib/libiiGraphicAdjust.0.3.0.dylib` |
| SDK and consumer linkage / loaded libraries | No Qt, no older iiGraphicAdjust loaded |
| SDK build/installed SHA-256 and public headers | Identical |
| Upstream attribution and updated documentation | Installed with package |
| Parent SDK installer discovery | No dependency edges |
| Development example | Binary PPM, 256×128, 98,319 bytes |

There are 53 distinct CTest suites (33 retained and 20 new), exercised in three configurations plus three package consumers, totalling 162 final successful test executions. The installed consumer resolves all 27 new public operations through the installed CMake package. Tests include random slider combinations, 3600-hue mixer partition, independent radiometric Exposure oracles, normalized finite output, hidden-RGB isolation, exact zero-coverage/cancellation behavior, Auto Mask color/luminance boundary selection and thin-image optics/spatial cases. No sanitizer issue was reported.

Independent review also checked separable lightness extrema against brute-force windows and verified CA border neutrality and shared vignetting gain: 725,866 checks, zero failures, under ASan/UBSan. A separate color/development audit exercised 2000 valid random settings with zero failures. These manual audits supplement, rather than increase, the CTest suite count.

One 3840×2160 Light measurement (Highlights -60, Shadows +60) took 2.87315 seconds in the function and reached 1,262,780,416 bytes peak process resident memory. This is a single host/fixture measurement, not a latency guarantee or the composed-pipeline memory bound. Photo-size processing remains full-frame and allocation-heavy. Wider Gaussian windows use a documented box-filter approximation.

Final logs are `build/lightroom-{shared,static,sanitized}-tests.log` and `build/lightroom-consumer-{shared,static,host}-tests.log`; the exact host loader evidence is `build/lightroom-host-loaded.log`. Machine-readable source fingerprints and package evidence are saved in `build/verification.json`; older reports are retained separately. LightCraft's inspected commit is `629e39380e296f588c64cd9c0053a8edc3528f36`; its Rust tests were not run.

The new APIs are rendered-image operations on normalized sRGB. The convenience pipeline gamut-maps between stages and does not preserve the scene-linear Rec.2020 RAW headroom of LightCraft. Adobe pixel parity, RAW decoding, profile databases, AI tools, GUI/product integration and Windows/Linux/iOS/Android execution remain unverified or outside this module's implemented scope. No running product was restarted.
