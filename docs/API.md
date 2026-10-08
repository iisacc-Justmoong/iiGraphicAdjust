# API contract — 0.1.0

Include `<iiGraphicAdjust/iiGraphicAdjust.h>` for the complete API, or the independent `Image.h`, `Adjustments.h`, `Effects.h` and `Blend.h` headers. All names are in `iiGraphicAdjust`.

## Pixels and errors

`Rgba` contains four normalized `float` channels, encoded sRGB RGB and straight alpha. Alpha defaults to one. Images are densely packed in row-major order; width and height are positive `std::size_t` values. The view must contain exactly `width * height` pixels. No stride, color profile, platform object or borrowed output is implied.

Every operation validates dimensions, multiplication/container capacity, exact input length and finite `[0,1]` channels before processing. Every numeric setting must be finite and within its documented range. Invalid enumeration values are rejected, including on neutral calls. All operations return `Result<T>`, an alias for `std::expected<T, Error>`; error messages have static lifetime.

| Error code | Meaning |
| --- | --- |
| `InvalidDimensions` | Empty dimensions or overflow/container capacity violation |
| `InvalidBuffer` | Pixel/byte count differs from the dimensions |
| `InvalidPixel` | Nonfinite or non-normalized RGBA input |
| `InvalidParameter` | Nonfinite/out-of-range settings or unknown enum |
| `SizeMismatch` | Both blend inputs are valid but have different dimensions |
| `AllocationFailure` | Temporary/output allocation failed or exceeded container limits |

Invalid spans that do not refer to live readable storage cannot be detected by the SDK; maintaining the span's lifetime is the caller's responsibility. Exceptions from storage allocation are translated to `AllocationFailure`. The SDK never mutates inputs or keeps a view beyond the synchronous call.

`fromRgba8(width,height,bytes)` reads `R,G,B,A` byte groups; `toRgba8(view)` rounds normalized channels to nearest 8-bit values. `fromArgb32` and `toArgb32` interpret integer words as `0xAARRGGBB` regardless of memory byte order. Both formats use straight alpha and preserve RGB under zero alpha. Unpremultiply or normalize a GUI/renderer buffer in the host before passing it here.

## Color adjustment

`adjust(view, ColorAdjustment)` applies operations in this fixed order:

1. Decode sRGB, multiply by `2^exposure`, apply relative temperature/tint channel gains, clamp to `[0,1]`, and encode to sRGB. This conversion is skipped when all three settings are neutral.
2. Apply brightness and contrast in encoded RGB: `(channel + brightness - 0.5) * (1 + contrast) + 0.5`, then clamp.
3. Compute encoded-RGB luma `0.2126 R + 0.7152 G + 0.0722 B` and multiply the distance from that luma by `1 + saturation`, then clamp. This is an artistic saturation adjustment, not a colorimetric luminance conversion.
4. Apply encoded RGB power `1 / gamma`.
5. Multiply the original alpha by opacity.

| Field | Default | Valid range |
| --- | --- | --- |
| exposure (stops) | 0 | `[-16,16]` |
| brightness | 0 | `[-1,1]` |
| contrast | 0 | `[-1,1]` |
| saturation | 0 | `[-1,1]` |
| gamma | 1 | `[0.01,100]` |
| temperature | 0 | `[-1,1]` |
| tint | 0 | `[-1,1]` |
| opacity | 1 | `[0,1]` |

Positive temperature increases red gain to `1+0.25*temperature` and decreases blue gain to `1-0.25*temperature`. Positive tint increases green gain to `1+0.25*tint`. These values are relative effects rather than Kelvin or a color-profile white-balance model. Zero-alpha RGB is kept unchanged. Default settings produce an exact detached copy.

## Effects

`gaussianBlur(view,sigma)` accepts a standard deviation of `[0,32]` pixels. A normalized separable Gaussian kernel has radius `ceil(3*sigma)`. Samples outside the image repeat the nearest edge. RGB is decoded to linear sRGB and multiplied by alpha before convolution; all four premultiplied channels are filtered. Output is unpremultiplied and encoded to sRGB. A fully transparent filtered pixel becomes `(0,0,0,0)`; sigma zero returns an exact copy, including invisible RGB.

`sharpen(view,SharpenOptions)` uses the same blur as an unsharp mask. Sigma is `[0,32]`, amount `[0,8]`, and threshold `[0,1]`. Linear RGB becomes `original + amount*(original-blurred)` if the maximum absolute channel difference exceeds the threshold, then is clipped and encoded. Original alpha and invisible RGB are preserved. Zero sigma or amount returns an exact copy.

`addNoise(view,NoiseOptions)` adds samples to encoded RGB and clips to `[0,1]`. Amount `[0,1]` means half-range for Uniform samples and standard deviation for Gaussian samples. The requested Gaussian deviation describes noise before clipping. Monochromatic noise adds the same scalar to each RGB channel; color noise draws three separate scalars. Alpha and zero-alpha RGB remain unchanged.

A 64-bit seed and pixel index are hashed into a private per-pixel SplitMix64 state. Uniform samples use a fixed integer-to-open-interval conversion; Gaussian samples use Box–Muller. There is no global random generator. Identical settings and inputs reproduce output within the same build/runtime. Cross-platform transcendental math may differ in the last bits; pixel equality across different standard libraries is not guaranteed. Neutral noise returns an exact copy.

## Blending

`blend(backdrop,source,BlendOptions)` requires equal dimensions. Opacity `[0,1]` multiplies source alpha. Blend space is encoded `Srgb` by default or `LinearSrgb`. Supported modes are Normal, Multiply, Screen, Overlay, Darken, Lighten, SoftLight and Difference. The blend function affects the overlapping source/backdrop portion, followed by Porter–Duff source-over composition; the output remains straight-alpha encoded sRGB.

Source alpha zero, including opacity zero, preserves the backdrop exactly. Transparent backdrop RGB does not influence the result. SoftLight uses the cubic branch below backdrop 0.25 and square-root branch above it, matching the equations in [W3C Compositing and Blending Level 1](https://www.w3.org/TR/2024/CRD-compositing-1-20240321/#blendingsoftlight). sRGB transfer functions follow [CSS Color Level 4](https://www.w3.org/TR/css-color-4/#color-conversion-code). Selecting linear sRGB is an explicit working-space choice; it does not claim CSS renderer parity.
