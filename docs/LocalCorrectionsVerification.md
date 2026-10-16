# iiGraphicAdjust 0.5.1 verification

This record covers the complete module build after masked correction composition and the separate sample-plane filters were integrated. Host verification is macOS arm64 with the C++23 compiler/standard library recorded in `build/local-corrections-verification.json`. No Qt or external SDK is linked.

| Check | Result | Scope |
| --- | --- | --- |
| Unmodified 0.5.0 Release baseline | 150/150 CTest cases passed | Existing module behavior before this change |
| New focused Release suites | 13/13 passed | Six masked-correction cases and seven sample-filter cases |
| Complete 0.5.1 Release suite | 163/163 passed, 12.71 seconds | All existing and new processing tests |
| Complete 0.5.1 ASan/UBSan suite | 163/163 passed, 136.62 seconds | Debug build with AddressSanitizer and UndefinedBehaviorSanitizer; no sanitizer failures |
| Current SDK vs original Photoshop kernel replay | 78 Average + 26 Solarize cases; 1,730,775 active samples and 1,291,286 assertions passed | Current SDK sample APIs linked against original arm64 instructions in owned memory with synthetic host callbacks; no Photoshop GUI execution |
| Standalone masked-correction example | Successful PPM P6 RGB8, 256×128, 98,319 bytes | Actual Kelvin-WB result applied through feathered radial coverage |
| Language/float flags | 30 library C++ units; all 59 compilation units are C++ | `cxx_std_23`, extensions disabled, sample source has `-fno-fast-math -ffp-contract=off` |
| Release dynamic linkage | libc++ and libSystem only | Actual `libiiGraphicAdjust.0.5.1.dylib` inspection |

The original-instruction replay is separately recorded in [SampleFiltersVerification.json](SampleFiltersVerification.json) and [SampleFilters.md](SampleFilters.md). It validates those sample-plane Average/Solarize kernels, including native host assumptions and numeric limits. It does not establish masked-composition parity, other SDK correction-kernel parity, or a complete Photoshop/Lightroom workflow.

The library remains ABI series `0.5`; current shared-library version is `0.5.1`. Existing public structs, signatures, local-slider amount units, normalized `average(ImageView)` and `solarize(ImageView)` behavior were not changed. The Release library SHA-256 at this verification is `7a8c59ef5671dcfa6dbafbdb0ffdd3c70ef87ce564388104c966f758400a08f6`.

## Masked correction coverage

The six focused cases are `identity`, `coverage`, `spaces`, `alpha`, `validation`, and `composition`. They include bitwise preserved no-ops, aliasing/detachment, signed zero, sRGB transfer-knee values, fractional/inverted coverage and amount, unchanged component preservation, normalized endpoint bounds and complementary-mask symmetry. The independent linear black/white midpoint oracle is approximately `0.735356983`.

Validation tests reject empty/overflow dimensions, short/excess spans, transposed geometries, each nonfinite/out-of-range RGBA component, coverage and amount, and unsupported enums even on inactive/hidden data. Composition tests apply eight real existing corrections to a partially selected image: Levels, Curves, Kelvin white balance, HSL, Detail, Camera Raw, Develop Curve and Develop Light. Each produces a visible local change while preserving unselected/hidden pixels and base alpha.

Tests were added before implementation. The initial focused build linked against the declared but unimplemented `applyMaskedCorrection` symbol and failed as expected; after implementation, the focused and complete suites passed. The retained RED log is `build/masked-correction-red-build.log`. Mask validation was shared privately between the existing mask operations and the new correction consumer; existing mask suites remain included in both complete runs.

## Reproduction

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DIIGRAPHICADJUST_BUILD_EXAMPLES=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel 2
ctest --test-dir build --parallel 2 --output-on-failure
build/iiGraphicAdjustLocalCorrectionsExample build/local-correction.ppm

cmake -S . -B build/local-corrections-sanitized -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DIIGRAPHICADJUST_BUILD_EXAMPLES=ON \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build/local-corrections-sanitized --parallel 2
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
  UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build/local-corrections-sanitized --parallel 2 --output-on-failure
```

LeakSanitizer was explicitly disabled. Non-macOS builds and the original Adobe CameraRaw pixel engine remain unverified. The recovered Lightroom control declarations motivate the functional gap and are classified as decompiler pseudocode in [LocalCorrectionsEvidence.json](LocalCorrectionsEvidence.json).

Actual `~/.local` installation and the staged two-SDK brush/correction integration consumer belong to the parent task. The module includes the new public headers in its CMake install list and extends `tests/consumer/main.cpp` to resolve masked-correction and sample-plane APIs from an installed package. This module-level report does not substitute source tests for installed-artifact evidence.
