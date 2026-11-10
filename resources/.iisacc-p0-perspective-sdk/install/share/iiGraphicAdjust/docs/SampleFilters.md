# Evidence-bound sample filters

`SampleFilters.h` provides separate C++23 APIs for the Average and Solarize
sample kernels recovered from the installed Photoshop 27.10.0 arm64 plugins.
These APIs operate on caller-owned strided sample planes. They do not change
the existing `average(ImageView)` or `solarize(ImageView)` rendered-image
contracts, which use independent normalized RGBA models.

The implementation is adapted from the workspace's independently authored,
verified C++23 reconstruction, without linking the investigation project,
Adobe binaries, a scripting runtime, Qt, or another SDK. Public declarations
use standard C++ types and the module's normal export macro.

## Average

`sampleAverage8`, `sampleAverage16`, and `sampleAverageFloat32` accept one planar
channel and an optional `SampleSelectionMask`. Width counts active samples;
`rowStrideSamples` counts elements, including any row padding. The mask may
have its own stride but must have the same active dimensions. Its high bit
selects samples: bytes 128..255 select equally and 0..127 are excluded. This is
binary selection, not fractional mask weighting.

The kernels traverse 256x256 tiles in top-to-bottom, left-to-right order.
Unsigned integer tile sums use 32-bit arithmetic; completed tile sums and
counts accumulate as float64. Integer output is half-up rounded. Float input
instead accumulates as float32 within each tile, then float64 between tiles;
the final mean is converted to float32. Negative and HDR samples are unclamped.
An empty selection yields integer zero or floating NaN. Floating NaN payload
identity is outside the contract.

`SampleAverageResult<T>` returns the value, selected sample count, and tile
count. `fillSampleAverage8/16/Float32` validate both views, compute the complete
mean, then fill every active output sample. Output and input dimensions must
match. Exact or partial input/output aliasing is safe because all input reads
finish before writes start. Row padding and storage beyond the active extent
remain untouched. Photoshop's host selection compositing, per-channel callbacks,
progress, and cancellation are separate from these kernels.

Float arithmetic in this translation unit is compiled with contraction and
fast-math disabled (`/fp:strict` on MSVC). Application changes to the thread's
floating-point rounding environment are outside the default contract.

## Solarize

`solarizeSamples8/16` validate a mutable plane and then update active samples
in place. Interleaved channels are supported by passing `pixelWidth * channels`
as the active sample width; alpha is also transformed if included, so hosts
must exclude or separately preserve alpha when appropriate.

| Input | Verified operation |
|---|---|
| byte | `x < 128 ? x : 255 - x` |
| uint16 | `x < 16384 ? x : uint16_t(32768u - x)` |

The word scale is the observed Photoshop 0..32768 scale. Values above 32768
retain the original 16-bit wrapping subtraction; they are not clamped or
silently rescaled from a 65535-based range. No float Solarize behavior is exposed.
The original worker truncates the active row sample count to signed 16; this
safe API rejects active widths above 32767 before writing. Callers may process
smaller row subviews when their images have wider rows.

## Validation

All functions return `std::expected<..., SampleFilterError>`. Dimensions must
be positive and at most `INT32_MAX`, stride must cover an active row, and
`(height-1)*stride+width` must fit both `size_t` and the supplied span. Invalid
geometry, stride, storage, mask/output dimensions, and unsupported Solarize
row widths are reported before any output mutation. No allocation or image
container decoding occurs inside these sample kernels.

## Provenance and verification

The canonical workspace report is
[Photoshop native filters](/Volumes/Storage/Workspace/Engineering/Projects/Adobe/Photoshop/docs/Filters.md).
The corresponding prior
[native replay record](/Volumes/Storage/Workspace/Engineering/Projects/Adobe/Photoshop/evidence/native/filters/native-replay.json)
compared the reconstruction to original arm64 instructions in owned memory
with synthetic host callbacks: 78 Average cases, 26 Solarize cases, 1,730,775
active samples, and 1,291,286 assertions. Photoshop GUI was not executed.

| Kernel | Original slice SHA-256 | REA evidence |
|---|---|---|
| Average `_DoStart` `0x34fc` | `1fc869369f41119f05e4125aaa65000a60aede8ea21a5ef527fb8b328883cac0` | pseudocode `ev_8da0d5c2ab74959c1bce22fd79151d5739b47e5600dbbccc03fd3c05822d1cb6`; assembly `ev_98e5c7c36ac748c0bea9bbfab1201e07d3f2e8c300decfd9221f657563506c58` |
| Solarize worker `0x2800` | `491ef435586a6c94995eecdf9378878fe168a887032d4eee916b5a5f027f0550` | pseudocode `ev_6442587260ac96feab3b61a82ab904708052ad2b915ebc8fdbf899496bdf6210`; assembly `ev_581d5a58256253b626e530fbafeb6e5e307c73d21cd9ada4411cfdf2d1e44e6a` |

The SDK adds seven test cases covering integer tile bounds and rounding,
float cancellation/HDR/NaN/signed zero, mask thresholds/empty selection,
fill padding and aliasing, all 256 byte and all 65,536 word Solarize inputs, and
failure before mutation. The integration audit separately compares the SDK
functions to the pinned workspace reconstruction in 170 cases.

The new SDK library was also compared directly to the same SHA-bound original
arm64 instructions using the existing controlled replay harness. A names-only
compatibility header forwards the harness's C++ calls to the exported SDK APIs;
the workspace reconstruction is not linked into that replay executable. All
78 Average and 26 Solarize cases passed, including 1,730,775 active samples and
1,291,286 assertions. The original slices are copied into owned memory;
synthetic host records and callbacks replace application integration. Non-NaN
float output is bit-compared; NaN classification is compared. Current library,
wrapper, harness, and slice hashes are in
[SampleFiltersVerification.json](SampleFiltersVerification.json). This is a
bounded original-kernel comparison, not Photoshop GUI execution or complete
filter-host compatibility.
