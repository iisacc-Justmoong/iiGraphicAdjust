# Verification

Run all checks from the module directory. Keep every build, consumer and generated file below `build/`.

## Behavior coverage

The eight CTest suites cover invalid image/settings rejection, integer conversion round trips, color-adjustment reference vectors, all eight blend modes and partial alpha, Gaussian impulse response and transparent-edge color preservation, unsharp-mask threshold/alpha behavior, noise repeatability/distribution/independent sequences, and normalized-output/input-immutability invariants.

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

The consumer uses only installed public headers and the imported package, exercising conversion, adjustment, noise, blur, sharpen and SoftLight composition. Repeat with `-DBUILD_SHARED_LIBS=OFF` in `build/static/build` and an independent staged prefix to check static export definitions.

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

## Recorded host result — 2026-10-08

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

The 27 successful test executions above include repeated behavior coverage across three build configurations, not 27 unique algorithms. Detailed logs and machine-readable evidence are in `build/verification.json` and its referenced test logs. Windows, Linux, iOS, Android and integration into Dreamscapes/iiSharedCanvas remain unverified. No existing running product was restarted.
