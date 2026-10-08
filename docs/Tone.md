# Tonal adjustments

`<iiGraphicAdjust/Tone.h>` contains standalone tonal operations inspired by the
Photoshop Image > Adjustments commands. They are independent C++23 implementations
of the contracts below. They do not reproduce Adobe's private processing pipeline
and do not promise pixel-for-pixel Photoshop parity. In particular, this SDK
processes normalized float sRGB and has no ICC, CMYK, HDR, document bit-depth,
selection, or adjustment-layer context.

Every function returns `Result<Image>`, validates the input, keeps its dimensions,
and creates detached storage. Input storage is immutable. Alpha remains exactly
unchanged, including partially transparent samples. Pixels whose alpha is zero
retain their original hidden RGB and do not contribute to statistics. Every
nonzero-alpha pixel contributes equally to histograms and automatic corrections;
alpha is not a statistical weight. Invalid settings return `InvalidParameter`;
dimensions, buffers, pixels, and allocation failures use the existing error codes.

## Brightness and Contrast

`brightnessContrast(image, BrightnessContrastOptions)` operates on encoded RGB.
Brightness and contrast are finite values in `[-1,1]`. For each visible channel:

```
output = clamp((input + brightness - 0.5) * (1 + contrast) + 0.5)
```

Zero brightness and contrast are exact identity. Contrast `-1` produces 0.5.
This is the SDK's explicit slope-and-offset contract, rather than an emulation of
Photoshop's modern or legacy Brightness/Contrast switches.

## Levels

`levels(image, LevelsOptions)` applies the master channel first, followed by the
corresponding red, green, or blue channel. Each `LevelChannel` has normalized
`inputBlack`, `inputWhite`, `outputBlack`, and `outputWhite` endpoints and a
finite `gamma` in `[0.01,100]`. Input black must be strictly below input white.
Output endpoints may be equal or reversed, allowing flattening and inversion.

```
t = clamp((input - inputBlack) / (inputWhite - inputBlack))
output = outputBlack + (outputWhite - outputBlack) * pow(t, 1 / gamma)
```

Identity channels bypass arithmetic. This float formula does not include
Photoshop's document-depth rounding or the fitted shadow-toe behavior observed
in the reference Rust implementation.

## Curves

`curves(image, CurvesOptions)` applies a master curve before the individual RGB
curves. A `CurveChannel` contains between 2 and 65,536 finite `CurvePoint` values.
Both coordinates are in `[0,1]`; input coordinates must already be strictly
increasing. Duplicate, descending, and nonfinite input coordinates are errors,
not silently sorted or merged.

The SDK uses piecewise linear interpolation. Values below or above the supplied
domain map to the nearest endpoint output. Outputs need not be monotonic, and
linear segments cannot overshoot. Diagonal curves covering the complete `[0,1]`
domain are exact identity. This deliberately differs from Photoshop-style
cubic spline interpolation.

## Exposure

`exposure(image, ExposureOptions)` uses the SDK's standard sRGB transfer function
to linearize and encode color. `stops` is in `[-16,16]`, `offset` is a linear-light
offset in `[-1,1]`, and `gamma` is in `[0.01,100]`.

```
linearOutput = pow(clamp(sRGBDecode(input) * exp2(stops) + offset), 1 / gamma)
output = sRGBEncode(linearOutput)
```

Exposure, offset, and gamma all operate in linear light, in that order. Neutral
settings return exact channel values. Clipping before gamma intentionally makes
this an SDR operation; values above one are not retained.

## Shadows and Highlights

`shadowsHighlights(image, ShadowsHighlightsOptions)` uses per-pixel encoded
luminance `Y = 0.2126 R + 0.7152 G + 0.0722 B`. Amounts are in `[0,1]`; tonal
widths are in `(0,1]`. Let `smooth(x) = t²(3-2t)` for `t = clamp(x)`:

```
lift = shadows * smooth((shadowWidth - Y) / shadowWidth)
darken = highlights * smooth((Y - (1 - highlightWidth)) / highlightWidth)
output = clamp((input + lift * (1 - input)) * (1 - darken))
```

Both masks use the original pixel luminance. Lift precedes darkening when their
ranges overlap. Neutral amounts are exact identity. This is a global tonal-mask
contract with no spatial radius or neighborhood analysis, unlike Photoshop's
local Shadows/Highlights correction.

## Invert, Posterize, and Threshold

`invert(image)` maps each visible RGB channel to `1-input`.

`posterize(image, levelCount)` accepts integer counts from 2 to 65,536. It rounds
each visible channel to the nearest of `levelCount` evenly spaced normalized
levels: `round(input*(levelCount-1))/(levelCount-1)`. Halfway ties round upward.
This is float quantization, not Photoshop's 8-bit bucket quantization.

`threshold(image, level)` accepts a finite normalized level, defaulting to 0.5.
It compares encoded Rec.709-weighted luminance to the threshold. Equality maps
to white; lower values map to black. All RGB channels receive the same result.

## Equalize

`equalize(image)` constructs one 256-bin histogram by pooling the RGB channels of
all visible pixels. The bin index is `round(channel*255)`. It applies the same
cumulative distribution function to all three channels:

```
table[bin] = clamp((cumulativeCount[bin] - firstOccupiedCount) /
                   (totalCount - firstOccupiedCount))
```

An empty or one-bin histogram returns an exact copy. This operation quantizes
float color through the histogram bins and can change color balance; it is not
a luminance-only color-preserving equalizer.

## Automatic corrections

All three automatic operations accept `AutoToneOptions`. `blackClip` and
`whiteClip` are finite fractions in `[0,1)` whose sum is less than one. The
defaults are 0.005 on each side. The defaults are SDK policy and should not be
interpreted as identical defaults for every Adobe command.

Samples are sorted without histogram quantization. For `n` samples, black is
the sample at `floor(blackClip*(n-1))`, and white is the sample at
`ceil((1-whiteClip)*(n-1))`. Tied quantiles are retained; if black equals white,
that channel is copied instead of dividing by zero. Other values map through
`clamp((input-black)/(white-black))`. A clipping fraction denotes the requested
rank and does not promise that exactly that fraction of tied samples will clip.

| Function | Statistical samples | Correction |
| --- | --- | --- |
| `autoLevels` | Separate R, G, and B arrays | Independent channel ranges; can change color casts |
| `autoContrast` | One pooled RGB array | A common range for all channels |
| `autoColor` | Separate R, G, and B arrays | Independent ranges, then channel median gamma |

For Auto Color, the original channel median is the middle sample for odd counts,
or the mean of the two middle samples for even counts. After endpoint stretching,
an interior median `m` determines `gamma = clamp(log(m)/log(0.5), 0.01, 100)`;
the corrected value is `pow(stretchedInput,1/gamma)`. This targets each channel's
median to 0.5 while retaining endpoints. Endpoint medians skip gamma, and constant
channels are unchanged. This deterministic strategy is useful for removing
channel casts but may alter intentional colors and contrast. It is the SDK's
algorithm, not Adobe's Find Dark & Light Colors implementation. Neutral constant
images remain exact identity in every automatic mode. A color-constant image
is unchanged by Auto Levels and Auto Color; Auto Contrast can expand its pooled
RGB range.

## References and implementation provenance

The command roles were checked against Adobe's primary documentation:

- [Levels](https://helpx.adobe.com/photoshop/desktop/adjust-color/color-corrections/adjust-levels-in-an-image.html)
- [Curves](https://helpx.adobe.com/photoshop/using/curves-adjustment.html)
- [Automatic tonal adjustments and Equalize](https://helpx.adobe.com/photoshop/using/making-quick-tonal-adjustments.html)
- [Exposure and the command list](https://helpx.adobe.com/photoshop/desktop/adjust-color/color-corrections/color-adjustment-commands.html)

The supplied [photocraft repository](https://github.com/storytold/photocraft),
commit `5896f0b63e6f342e0311920b4d2a0ea5a94330fc`, was inspected for command
coverage and differences in float/quantized Levels, cubic Curves, and Posterize.
`Tone.cpp` is independently written from the numerical contracts in this document;
no Rust code or fitted Photoshop coefficients were copied.

`tests/tone.cpp` covers reference vectors, interpolation, channel composition,
automatic correction differences, histogram ties, endpoint medians, constant
images, detached storage, input immutability, alpha and hidden RGB preservation,
finite normalized output, and invalid images and parameters.
