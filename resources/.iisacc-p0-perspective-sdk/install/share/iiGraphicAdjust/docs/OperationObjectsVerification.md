# Operation object and correction verification

Verified on 2026-10-19 on macOS arm64. Source package/ABI is 0.6.0/0.6.
The source work and generated evidence remain inside the Workspace module.
No host SDK installation or active product was replaced.

## Confirmed results

| Check | Result |
| --- | --- |
| Pre-change Release baseline | 163/163 passed |
| Final Release CTest | **202/202 passed** |
| Final ASan/UBSan/float-cast-overflow CTest | **202/202 passed** |
| Isolated Release installed consumer | Existing API consumer and complete 309-object consumer passed |
| Isolated instrumented installed consumer | Both consumers passed; object runner reports `Executed 309 objects` |
| Standalone installed object headers | **34/34** compile separately with C++23 |
| Exported processing coverage | Every exported adjustment/filter/mask/analysis/RAW/resource/sample function maps to a concrete object |
| C++ standard | 68/68 Release and 64/64 instrumented translation units use C++23 |
| Instrumentation | All 64 instrumented translation units have address, undefined and float-cast-overflow sanitizers |
| Qt check | No direct Qt dependency in the engine or its existing native file provider |
| Retained filtering audit | Existing pinned filter/recipe/resampling/retouch/diagnostic inventory passed |
| Patch formatting | `git diff --check` passed |

There are 309 concrete objects, not 309 newly implemented algorithms. Existing
processing kernels remain covered by the original suites. New object execution
suites cover every concrete type, including actual typed mask, analysis, RAW,
resource and sample-plane calls. Metadata cardinality and execution cardinality
are checked for equality. Pixel converters keep their low-level utility APIs.

The package still links the existing iiFileProviderNative (current local 0.8.0)
for native file integration. It does not add a new third-party runtime or Qt.
The provider uses native macOS frameworks and SQLite; CPU kernels and the new
object implementation use C++23 standard-library facilities. These host results
do not verify other operating systems or third-party consumers in production.

## Behavioral evidence

- Object snapshots survive source edits/destruction and own curve/LUT/reference/
  mask/depth data. Source input remains immutable; outputs are detached.
- Stack order has an independently computed exposure/inversion oracle. Empty,
  disabled, invalid, moved-from and over-budget cases have explicit behavior.
- Mask/amount interpolation checks encoded and linear-light midpoints, exact
  zero coverage and premultiplied-alpha isolation from invisible source RGB.
- Geometry-changing full stages work. Partial geometry is rejected at the exact
  stage; kernel errors retain the operation identity and original cause.
- Color-to-alpha tests recomposition over randomized keys against the independent
  compositing equation, existing alpha, transparent pixels and threshold steps.
- Flat-field tests recover a known uniform linear scene from nonuniform response,
  verify gain interpolation and reject coverage/dimension errors.
- Gradient fills have an independent transfer-function midpoint check. All five
  geometries execute; pattern wrapping/pixel centers and bounded large offsets
  are exercised.
- Sharpen Radius/Detail and Luminance NR Contrast each measurably change pixels;
  default legacy oracles still pass and inactive invalid controls are rejected.
- Recovered sample-plane objects check integer/float average/fill values and
  Solarize integer thresholds without converting through normalized RGBA.

`build/objects-initial-red.log` records the initial missing-object-API failure.
Final evidence is separate from that TDD checkpoint.

## Evidence files

- `build/objects-baseline-build.log`, `build/objects-baseline-tests.log`
- `build/objects-configure.log`, `build/objects-build.log`, `build/objects-tests.log`
- `build/objects-sanitizer-configure.log`, `build/objects-sanitizer-build.log`,
  `build/objects-sanitizer-tests.log`
- `build/objects-package-{install,configure,build,test}.log`
- `build/objects-sanitized/build/objects-package-{install,configure,build,test}.log`
- `build/objects-package/build/Testing/Temporary/LastTest.log`
- `build/objects-sanitized/build/objects-package/build/Testing/Temporary/LastTest.log`
- `build/objects-header-check.log`, `build/objects-linked-libraries.log`
- `build/objects-retained-filter-audit.log`, `build/objects-validation.json`

`objects-validation.json` records final source/document/log/library hashes and
scope boundaries. Build outputs are local evidence and are not installed as API.
Installed documentation and public headers are checked against their final source
contents after the final documentation update.

## Reproduction

Run from the module directory with iiFileProviderNative >= 0.6 discoverable.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure --parallel 4
```

The installed-package CTest installs under `build/objects-package/install`, builds
an independent consumer under `build/objects-package/build`, and runs both its
legacy API and complete object tests. It never uses the default host install
prefix for this check.

```sh
cmake -S . -B build/objects-sanitized/build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DIIGRAPHICADJUST_BUILD_EXAMPLES=OFF \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined,float-cast-overflow" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined,float-cast-overflow"
cmake --build build/objects-sanitized/build --parallel 4
ctest --test-dir build/objects-sanitized/build --output-on-failure --parallel 4
```

`iiGraphicAdjustObjectsExample` renders a procedural four-panel comparison to
`build/operation-objects.ppm`: original, global exposure/grading/detail, local
masked monochrome, and watercolor. The corresponding PNG is a lossless format
conversion of those actual output pixels. It is an SDK demonstration, not an
Adobe oracle or performance benchmark.

## Unverified and incomplete

This verifies independently implemented CPU contracts, public object ownership
and packaging. It does **not** verify complete Photoshop/Lightroom functionality,
all Adobe option/depth semantics, original-application pixel equivalence,
learned/semantic/generative operations, complete RAW file/profile handling,
scene-linear wide-gamut/HDR output, preset/smart-filter persistence, panorama/HDR
merge, hardware performance or non-macOS behavior. The exact category/remaining
mapping is in [adobe-feature-coverage.json](adobe-feature-coverage.json); no
unsupported neural placeholder is counted among the 309 executable objects.
