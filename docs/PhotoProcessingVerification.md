# Photo processing verification — 0.5.0

On 2026-10-09 (Asia/Seoul), the C++23 source was built and executed on macOS
arm64 with AppleClang 21.0.0. No Qt or external processing dependency was added.

| Configuration | Library suites | Installed public-package consumer |
| --- | ---: | ---: |
| Release shared | 150/150 | 1/1 |
| Release static | 150/150 | 1/1 |
| Debug shared, AddressSanitizer + UndefinedBehaviorSanitizer | 150/150 | 1/1 |

There are 140 retained rendered-image suites and ten added cases: six sensor RAW
cases and four photo-operation cases. This is a suite count, not a count of all
possible photographic functions. The new consumer calls `developRaw`, `renderRaw`,
`applyOrientation`, `crop` and `imageHistogram` through installed headers/targets,
while retaining the existing adjustment/filter consumer calls.

RAW numeric tests compare independently calculated stencils, measured sites,
four CFA patterns and phase offsets, per-site black levels, nonidentity color
matrices, white balance and exposure. Values above one remain available in linear
output and can be recovered with later negative toning exposure. Malformed sample
buffers, stride/address overflow, NaN/Inf and unrepresentable output are rejected.
The photo suite uses exact 3x2 pixel permutations for all eight EXIF orientations,
cropped pixel/alpha comparisons and independently weighted histogram statistics.

The optional upper-layer iiCameraIntegration 0.2.0 `ImageProcessing` component
was verified in shared/static/sanitized configurations, with 21/21 camera suites
and one installed consumer per variant. A generated first frame is decoded,
interpreted under explicit color rules, oriented/cropped/developed/resized and
analyzed. Camera types are not referenced by this processing SDK.

Both SDKs are sanitizer-instrumented. External FFmpeg/system libraries are not.
macOS LeakSanitizer is unavailable, so tests use `ASAN_OPTIONS=detect_leaks=0`.
No sanitizer finding was reported. This does not prove absence of every memory
or numerical defect or constitute a performance guarantee.

Run `../iiCameraIntegration/tools/verify.sh --photo --sanitize` from this module
with the sibling source and installed FFmpeg development libraries. Build, stage,
generated images and consumer output stay under the respective `build/` folders.
The routine explicitly selects each variant's package prefix. Host loader
inspection confirms the shared consumer loads the staged 0.5.0 library. Existing
`~/.local` 0.4.0 remains unchanged, and no running product was restarted.

Logs and JUnit results are `build/photo-{shared,static,sanitized}-ctest.xml`,
`build/photo-{static,sanitized}-tests.log`, `build/photo-tests.log` and the
`build/photo-*-consumer/ctest.xml` files. The complete repeatable workflow output
and machine-readable matrix are in the camera module's
`build/photo-workflow.log` and `build/photo-verification-report.json`.

The RAW input is caller-decoded uint16 Bayer with caller-supplied calibration.
Encoded RAW file decoding, a DNG pipeline, camera profiles, ICC/wide-gamut output,
adaptive demosaicing, physical camera image quality, GPU acceleration, product
integration and Windows/Linux/iOS/Android runtime are not established here.
Existing [historical verification](Verification.md) remains separately documented.
