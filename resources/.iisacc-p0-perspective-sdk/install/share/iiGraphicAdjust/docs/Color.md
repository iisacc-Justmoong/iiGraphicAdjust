# Color adjustments

Include `<iiGraphicAdjust/Color.h>` or the umbrella `<iiGraphicAdjust/iiGraphicAdjust.h>`.
These operations are independent C++23 implementations of familiar adjustment concepts.
They do not reproduce Photoshop's private algorithms or claim Photoshop pixel parity.
The feature names follow the [official Adobe adjustment command overview](https://helpx.adobe.com/photoshop/desktop/adjust-color/color-corrections/color-adjustment-commands.html);
the formulas below are this SDK's explicit contracts.
All images use normalized encoded sRGB and straight alpha. No ICC profile conversion,
actual CMYK profile processing, HDR color management, or skin detection is performed.

Every function validates all input pixels and all settings, including inactive controls.
It returns a detached `Result<Image>`, preserves dimensions and alpha exactly, and
copies RGB at zero alpha without processing it. An invalid image returns its
`InvalidDimensions`, `InvalidBuffer`, or `InvalidPixel` error; invalid settings return
`InvalidParameter`; failed allocations return `AllocationFailure`. Neutral settings
for hue/saturation, vibrance, color balance, zero-density photo filter, default RGB
channel mixer, selective color, and replace color produce exact copies.

## Hue/Saturation and Colorize

```cpp
HueSaturationOptions options;
options.hue = 30;
options.saturation = .2;
options.range = HueRange::Reds;
auto result = hueSaturation(image.view(), options);
```

The implementation converts encoded RGB to HSL. For a value `v` and signed control
`a`, saturation and lightness use `v + a * (a >= 0 ? 1-v : v)`.
Hue rotation is `[-180,180]` degrees, saturation and lightness offsets are `[-1,1]`.
Achromatic pixels remain achromatic under saturation changes unless Colorize is on.
Lightness zero is unchanged; -1 produces black; +1 produces white.

`HueRange::Master` has full influence on every visible pixel. The six other sectors
are centered at 0, 60, 120, 180, 240 and 300 degrees for red, yellow, green, cyan,
blue and magenta. Circular hue distances through 30 degrees have full influence.
The next `falloff` degrees, in `[0,60]`, smoothly decay to zero with
`1 - smoothstep((distance-30)/falloff)`, where `smoothstep(t)=t*t*(3-2*t)`.
A zero falloff produces a hard sector boundary. Achromatic pixels have zero influence
in a selected hue sector. The mask is evaluated on the source, then the original and
adjusted encoded RGB are interpolated by that mask.

Colorize sets a wrapped absolute hue from the same degree control and sets absolute
saturation in `[0,1]`; its saturation cannot be negative. Source HSL lightness is
retained before the optional signed lightness control. To tint gray pixels, use Master.

## Vibrance

`vibrance(image, VibranceOptions{vibrance, saturation})` operates in HSL.
Both controls are `[-1,1]`. First `s' = clamp(s + vibrance*s*(1-s))` adds a
relative emphasis to muted colors; the saturation control then applies the signed
interpolation described above. An original zero-saturation pixel remains gray.
A fully saturated pixel is unchanged by the vibrance control alone, including negative
vibrance. This explicitly bounded contract is not a skin-protecting Photoshop model.

## Color Balance

`ColorBalanceOptions` contains independent `shadows`, `midtones` and `highlights`
`ToneBalance` values. Each has `cyanRed`, `magentaGreen` and `yellowBlue` controls
in `[-1,1]`; positive values respectively add red, green and blue.

For encoded luminance `y = .2126*r + .7152*g + .0722*b`, the tone weights are
`shadows=(1-y)^2`, `midtones=2*y*(1-y)`, and `highlights=y^2`. Their sum is one.
Each output channel receives half the weighted control value. For example, a .5-gray
pixel and a midtone cyan/red control of +1 produce RGB `(.75,.5,.5)` when
`preserveLuminosity` is false.

`preserveLuminosity` is true by default. It subtracts the new luminance from each
adjusted channel, restores the original luminance, and uniformly reduces chroma as
needed to fit `[0,1]`. This preserves encoded weighted luminance even at gamut limits.
It preserves neither HSL lightness nor physical linear-light energy.

## Black & White and tint

`BlackWhiteOptions::weights` stores six coefficients in red, yellow, green, cyan,
blue, magenta order. The defaults are `.4,.6,.4,.6,.2,.8` and each is accepted in
`[-2,3]`. A source hue linearly interpolates adjacent coefficients around the hue
circle. The grayscale result is
`clamp(minimum RGB + (maximum RGB - minimum RGB) * coefficient)`.
Consequently pure red becomes .4 gray, pure blue .2 gray, and gray pixels retain their
source gray value. Negative and large coefficients intentionally clip the result.

With `tint=true`, the grayscale output becomes the HSL lightness of a color with
`tintHue` in `[0,360]` and `tintSaturation` in `[0,1]`. Tinting is an HSL operation
and does not promise constant weighted luminance after adding chroma.

## Photo Filter

`PhotoFilterOptions::color` is an encoded sRGB triplet in `[0,1]` representing
filter transmittance. Density is `[0,1]`, default .25. Each channel is transformed as
`channel * (1-density + density*filterChannel)`.

`preserveLuminosity=true` uses the same luminance restoration and gamut compression
as Color Balance. With it disabled, a red filter `(1,0,0)` at density .5 changes
`.5-gray` to `(.5,.25,.25)`. The explicit RGB filter is not a physical spectral
camera-filter simulation or a Kelvin white-balance control.

## Channel Mixer

`ChannelMixerOptions` has separate `ChannelMix` settings named `red`, `green` and
`blue`. Each contains three RGB `coefficients` in `[-2,2]` and a `constant` in
`[-1,1]`. Each output channel is the source RGB dot product plus that constant,
clamped into `[0,1]`. The RGB defaults are an identity transform.

`monochrome=true` uses the `gray` mix, whose defaults are `(.4,.4,.2)` with no
constant, and copies its result to all output channels. Coefficients need not sum
to one: exposure changes and clipping are deliberate, documented behavior.

## Selective Color

`SelectiveColorOptions::ranges` has nine `SelectiveColorAdjustment` entries indexed
by `ColorRange`: Reds, Yellows, Greens, Cyans, Blues, Magentas, Whites, Neutrals and
Blacks. Every entry has cyan, magenta, yellow and black controls in `[-1,1]`.
The default method is Relative; Absolute is also supported.

The independent source RGB weights are:

| Range | Weight |
| --- | --- |
| Reds | `max(0,r-max(g,b))` |
| Yellows | `max(0,min(r,g)-b)` |
| Greens | `max(0,g-max(r,b))` |
| Cyans | `max(0,min(g,b)-r)` |
| Blues | `max(0,b-max(r,g))` |
| Magentas | `max(0,min(r,b)-g)` |
| Whites | `max(0,2*min(r,g,b)-1)` |
| Neutrals | `(1-chroma)*(1-abs(2*HSL_lightness-1))` |
| Blacks | `max(0,1-2*max(r,g,b))` |

Weights may overlap and are not normalized against each other. They are all computed
from the source, so the array order does not make adjustment order affect the result.
Let `ink` be the sum of weighted cyan controls for red, magenta controls for green,
and yellow controls for blue; let `key` be the sum of weighted black controls.
For each original channel `c`, Relative produces
`clamp(c - ink*(1-c) - key*c)`, while Absolute produces
`clamp(c - ink - key)`.

This is a defined RGB complement / black-intensity CMYK-style adjustment. It does not
convert to a CMYK space or approximate the behavior of a particular CMYK ICC profile.
Relative cyan therefore scales existing red-complement ink, while relative black
scales source red intensity. A red-range cyan control of .4 on RGB `(.75,.25,.25)`
changes red to .7 in Relative and .55 in Absolute.

## Desaturate and Replace Color

`desaturate(image)` uses HSL lightness `(minimum RGB + maximum RGB)/2` as its gray.
It is intentionally distinct from the legacy `adjust()` saturation control, which
uses encoded weighted luminance.

`ReplaceColorOptions::target` is an encoded RGB triplet in `[0,1]`. Selection distance
is `sqrt(((r-tr)^2+(g-tg)^2+(b-tb)^2)/3)`, so its normalized range is `[0,1]`.
`fuzziness` is the selection radius in `[0,1]`; `softness` is the fraction of that
radius occupied by a smoothstep falloff, also `[0,1]`. Pixels inside
`fuzziness*(1-softness)` have full influence. Influence falls to zero at the selection
radius. A zero softness selects a hard radius; an exact radius endpoint has zero
influence. A zero fuzziness matches only distances through `1e-12`.

The hue, saturation and lightness offsets have the same ranges and HSL semantics as
Hue/Saturation, and the original and transformed encoded RGB are interpolated by the
selection influence. Replace Color does not use perceptual Delta E or an image mask.

## Match Color

```cpp
MatchColorOptions options;
options.fade = .25;
auto result = matchColor(source.view(), reference.view(), options);
```

Source and reference may have different dimensions. Both are validated and must have
at least one pixel with positive alpha. If either has no visible sample, the function
returns `InvalidParameter`, including at full fade. It validates settings even when
`fade=1` would otherwise preserve the source unchanged.

Each image's mean and population standard deviation are computed per channel in
linear sRGB with each sample weighted by alpha. Hidden RGB never enters statistics.
The variance uses a two-pass centered sum. Each visible source channel is mapped to
`referenceMean + (linearSource - sourceMean) * referenceStd/sourceStd`.
A source standard deviation through `1e-12` uses ratio one, avoiding an undefined
zero-variance division. Constant source pixels consequently map to reference means;
no detail is fabricated from a constant channel.

`neutralize=true` replaces all three target means with the reference's weighted linear
luminance mean, removing the mean color cast of the matched result. It does not
perform an independent white-balance estimate. `luminance` in `[0,2]` multiplies the
matched linear weighted luminance. `colorIntensity` in `[0,2]` scales deviations from
that luminance. The resulting linear channels are clipped and sRGB encoded once.
`fade` in `[0,1]` then interpolates encoded RGB back to the original; full fade is an
exact copy. Default luminance and color intensity are one, and default fade is zero.

The statistical match is per channel in linear RGB, not a Photoshop or perceptual
Lab transfer. It can clip out-of-gamut channels and does not transfer local texture.

## Validation coverage

`iiGraphicAdjustColorTests` exposes `hue`, `vibrance`, `balance`, `black-white`,
`filter`, `mixer`, `selective`, `replace`, `match`, `validation` and `invariants`.
Tests include independent numerical vectors, smooth falloff values, linear mean and
variance transfer, alpha-weighted statistics, finite extreme settings, gray protection,
subnormal RGB, invalid enums/NaN/inactive controls, exact neutral copies, detached
storage, input immutability, alpha preservation and hidden-RGB preservation.
