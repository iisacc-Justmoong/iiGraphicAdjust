# Gradient Map, Color Lookup and HDR Toning — 0.2.0

These are independent CPU operations with explicit numeric contracts. They preserve source alpha and return detached normalized sRGB images; they do not create adjustment layers or modify a document. All input images/settings are validated before neutral shortcuts.

## Gradient Map

`gradientMap(ImageView, GradientMapOptions)` uses encoded RGB tone `0.2126 R + 0.7152 G + 0.0722 B`. With reverse enabled the tone becomes `1-tone`. A gradient has 2..256 strictly increasing normalized positions with normalized RGB colors. Endpoints need not be 0 and 1: tones outside the first/last position use the closest stop. Colors interpolate linearly in encoded RGB. Amount `[0,1]` interpolates the mapped color with the original, preserving alpha and hidden RGB.

Optional dither is a zero-mean 4×4 Bayer offset of less than half an 8-bit step, applied to the mapped RGB before clipping and amount interpolation. It is ordered dither, not blue noise. Pixel coordinates are relative to the input image origin.

## Color Lookup

`ColorLut` owns either `size` RGB rows for a 1D LUT or `size^3` rows for a 3D LUT. Sizes are 2..65536 for 1D and 2..129 for 3D. Three-dimensional rows use red-fastest indexing `((b*size)+g)*size+r`. A 1D LUT independently interpolates each channel; it is not expanded into a large 3D table.

Each finite increasing domain interval normalizes the corresponding input RGB coordinate, then clips the coordinate to the table. Values must be finite but may be outside `[0,1]`; only the final mapped color is clipped. Three-dimensional lookup offers trilinear or tetrahedral interpolation. Tetrahedral interpolation sorts the three fractions and interpolates four vertices in one of six tetrahedra. The tests distinguish both methods using analytic cross-product corner data and all six fraction orderings.

`colorLookup(ImageView, ColorLut, ColorLookupOptions)` accepts amount `[0,1]` and the same optional ordered dither as Gradient Map. Unknown interpolation enum, malformed domain, nonfinite rows and wrong row count fail explicitly, including amount zero. Alpha and hidden RGB are unchanged.

`identityLut(size)` builds an actual 3D identity lattice. `parseCubeLut(string_view)` parses a standalone `.cube` text buffer without filesystem I/O, using locale-independent `from_chars`. Supported directives are a quoted optional TITLE, exactly one LUT_1D_SIZE or LUT_3D_SIZE, optional DOMAIN_MIN/MAX, comments and exact finite RGB triples. Duplicate/misplaced directives, trailing garbage, missing/extra rows, invalid number syntax and combined 1D-shaper-plus-3D files are rejected. Input text is limited to 128 MiB. TITLE is validated but not retained; formats `.3dl`, `.look`, ICC profiles and LUT_*_INPUT_RANGE directives are outside this parser's contract.

## HDR Toning

`HdrImageView` is a separate borrowed view of `HdrRgba` samples in linear-light sRGB primaries. RGB may exceed one but must be finite and nonnegative; alpha remains normalized. The ordinary `ImageView` overload decodes encoded normalized sRGB before processing. Both return a normal `Image`.

`HdrToningOptions` supports ExposureGamma and HighlightCompression. Exposure `[-16,16]` multiplies linear RGB by `2^exposure`. HighlightCompression additionally divides RGB by `1 + linear luminance`, a global Reinhard-style compression. Gamma `[0.01,100]` applies linear-light power `1/gamma` before sRGB output encoding. Clipping before power avoids overflow and produces the same normalized output as clipping after a positive power.

HDR zero-alpha samples become transparent black because their unbounded hidden RGB cannot be preserved in the normalized output contract. The SDR overload keeps its original hidden RGB and returns an exact copy for neutral ExposureGamma. Alpha is unchanged in both overloads. Negative radiance, NaN, infinity, buffer mismatch, unknown methods and invalid settings fail.

This SDK does not implement Photoshop's Local Adaptation or HDR histogram-equalization method, merge bracketed photographs, read HDR files, or reproduce its document flattening behavior. HDR Toning here names the supported exposure/gamma and global highlight-compression operations; it is not a full Photoshop dialog implementation.
