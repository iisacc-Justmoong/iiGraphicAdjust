# Blur, distortion and utility filter contracts

These individual functions are in `BlurFilters.h`, `DistortionFilters.h` and `UtilityFilters.h`. They implement the executable PhotoCraft menu/extended-filter families, with additional reusable box-guided kernels. [Filters.md](Filters.md) maps every command to its inspected source. All amounts below are menu units; input/output is normalized encoded sRGB, detached storage and `Result<T>` errors. All numeric values and enum fields, including inactive branches, are validated. Zero settings do not bypass image validation.

## Blur and sharpening

| Function/options | Accepted values | Processing |
| --- | --- | --- |
| `filterGaussianBlur` | Sigma [0,1000] px | Premultiplied encoded RGBA, exact separable Gaussian for sigma ≤4; three variance-matched box passes above four. Zero copies every pixel exactly. |
| `boxBlur` | Rounded radius [0,2000] px | Separable sliding-window means with repeated edges, O(image pixels). |
| `blur`, `blurMore` | None | Gaussian sigma .6 and 1.4. |
| `motionBlur` | Angle [-360,360]°; distance [0,2000] px | Symmetric samples along the direction, repeated-edge bilinear sampling. |
| `radialBlur` | Amount [0,100]; Spin/Zoom; x/y center [0,1] | Arc/radial integration with up to 65 samples. |
| `surfaceBlur` | Radius [0,100]; threshold [0,255] | Alpha-weighted per-channel bilateral neighborhood with triangular range weight; source alpha retained. |
| `unsharpMask` | Amount [0,500]%; radius [0,1000]; threshold [0,255] | Per-channel `original + amount/100*(original-blur)`, gated by channel difference. Source alpha retained. |
| `filterSharpen`, `sharpenMore`, `sharpenEdges` | None | Unsharp recipes 60%/.5/0, 150%/.6/0 and 100%/.8/6. |
| `smartSharpen` | Amount [0,500]%; radius [0,64]; reduceNoise [0,100] | Unsharp difference with exponential noise attenuation; source alpha retained. |
| `highPass` | Radius [0,1000] | `.5 + original - blur` per channel; source alpha retained. |
| `despeckle` | None | Surface Blur radius 2, threshold 12. |
| `smartBlur` | Radius [.1,100]; threshold [.1,100]; Low/Medium/High; Normal/EdgeOnly/OverlayEdge | Bounded disc taps with RGB range cutoff. Quality selects sample stride. Edge modes derive a four-neighbor edge mask. Source alpha retained. |
| `shapeBlur` | Radius [0,1000]; Circle/Ring/Square/Diamond/Triangle/Hexagon/Star/Heart/Cross | Rasterized shape spans and row-prefix means, repeated edges; filters RGB and coverage. |
| `lensBlur` | Radius [0,100]; blades [3,8]; curvature [0,100]; rotation [0,360]°; depth None/Transparency/Map; focalDistance [0,255]; brightness/noise [0,100]; threshold [0,255]; Uniform/Gaussian noise | Polygonal aperture, depth-dependent defocus from nine radius levels with premultiplied interpolation, highlight boost and deterministic final noise. Optional depth map must match dimensions and normalized pixel contract. An uncovered depth-map sample is on the focal plane (zero defocus), independently of invertDepth; its hidden RGB is ignored. Radius below .5 with no highlight/noise effect is an exact copy after full validation. |
| `tiltShift` | Blur [0,500]; center [0,1]; angle [-90,90]°; focus [0,1]; transition [.01,1] | Sharp strip and smooth distance-based defocus; six geometric Gaussian levels. |
| `irisBlur` | Up to 64 pins: x/y [0,1], radii [.001,2], angle [-180,180]°, roundness [0,100], feather [0,.999], blur [0,500] | Elliptical/rounded-rectangular focus regions; sharpest pin wins. Empty pins copy exactly. |
| `fieldBlur` | Up to 64 pins: x/y [0,1], blur [0,500] | Inverse-distance `1/(distance²+1)` interpolation of pin blur radii; with several pins the value at one pin also includes the other pins. Empty pins copy exactly. |
| `spinBlur` | Up to 64 elliptical pins: x/y [0,1], radii [.001,2], angle [-180,180]°, blurAngle [0,360]° | Arc integration, bounded to 64 samples, feathered inside each ellipse. |
| `pathBlur` | Up to 64 paths, ≤4096 total normalized [0,1] points, at least 2 per path; speed [0,500], taper [0,100] | Segment-based closest tangent, length/inverse-distance direction mixture and longitudinal taper; up to 64 symmetric samples. Degenerate paths have no blur. |

Gaussian, box, motion, radial, shape, lens and blur-gallery operations filter transported alpha with RGB. Surface/Smart Blur, unsharp, Smart Sharpen, High Pass and Despeckle retain source coverage and invisible input values. Blur Gallery paths/pins are caller-supplied values; the SDK does not own interactive pin or path state. The SDK's bounded sigma levels and sampling differ from the reference's exact per-pixel render. This preserves the actual algorithm class without claiming exact bitwise output.

## Distortion

| Function/options | Accepted values | Processing |
| --- | --- | --- |
| `offset` | Signed int32 x/y pixels; Wrap/Repeat/Transparent | Exact integer transport with selected border handling. |
| `twirl` | Angle [-999,999]° | Inverse circular angle falloff around image center. |
| `pinch` | Amount [-100,100] | Radial power mapping. |
| `spherize` | Amount [-100,100]; Normal/HorizontalOnly/VerticalOnly | Spherical radial mapping, or one-axis mapping. |
| `wave` | Generators [1,64]; min/max wavelength [1,999], ordered; min/max amplitude [0,999], ordered; Sine/Triangle/Square; seed; border | Seeded harmonic displacement sum. CPU reference clamps generators to 64 despite a larger UI range; SDK rejects excess instead. |
| `ripple` | Amount [-999,999]; Small/Medium/Large | Cross-axis sine perturbation (x displaced by y phase, y by x phase) with sizes 8/16/32. |
| `polarCoordinates` | RectangularToPolar/PolarToRectangular | Inverse Cartesian/polar coordinate conversion, transparent outside. |
| `displace` | Map image; horizontal/vertical [-999,999]; stretch/tile; border | Map red/green relative to .5 controls displacement; 100% corresponds to 128 px. Invisible map samples are neutral. |
| `shear` | Up to 4096 points: sorted unique position [0,1], offset [-1,1], minimum separation 1e-9; border | Nonuniform Catmull–Rom spline horizontal displacement across normalized height. Invalid order is rejected rather than silently sorted. |
| `zigZag` | Amount [-100,100]; ridges [0,20]; AroundCenter/OutFromCenter/PondRipples | Angular or radial ring displacement. |

Geometry uses premultiplied bilinear encoded RGBA and source pixel centers; interpolation may change alpha. A zero displacement copies exact original values, including invisible colors. Exact integer Offset transports hidden colors with their coverage. Nonidentity zero-coverage interpolated output is canonical transparent black. All maps are supplied by the caller; document identifiers and image files are not loaded.

## Noise, morphology and utility kernels

| Function/options | Accepted values | Processing |
| --- | --- | --- |
| `average` | None | Alpha-weighted visible encoded RGB mean, assigned at each visible pixel; original coverage retained. |
| `filterAddNoise` | Amount [0,400]%; Uniform/Gaussian; monochromatic; uint32 seed | Reference uint32 coordinate hash, amplitude amount/200, Gaussian Box–Muller scaled .5; source coverage retained. |
| `median`, `dustAndScratches` | Radius [0,500]; Dust threshold [0,255] | Disk `distance² ≤ r²+r`, alpha-weighted visible channel medians; Dust replaces channels only above the threshold. |
| `minimum`, `maximum` | Radius [0,500]; Square/Round; affectAlpha default true | Per-channel visible extrema, optionally coverage extrema. Square uses separable monotone queues; Round uses a true disc. |
| `reduceNoise` | Strength [0,10]; preserveDetails/reduceColorNoise/sharpenDetails [0,100]; removeJpegArtifact boolean | Encoded Y/Cb/Cr edge-preserving means, separate chroma smoothing, detail sharpening and optional 8-pixel JPEG grid deblocking. |
| `customFilter` | 25 row-major 5×5 finite coefficients [-9999,9999]; scale [1,9999]; offset [-9999,9999] | Signed per-channel convolution, divide by scale and add offset/255; repeated edges, source coverage retained. All-zero kernel copies input as in the reference. |
| `hsbHsl` | Input/output Rgb/Hsb/Hsl | Converts normalized RGB channel payloads; hue is normalized turns. Alpha remains coverage. |
| `deInterlace` | eliminateEven false; interpolate true | Removes 1-based odd or even fields and rebuilds from neighboring retained fields by premultiplied mean or duplication. |
| `ntscColors` | None | RGB→YIQ, chroma constrained by NTSC signal min/max, RGB reconstruction. |
| `guidedBlur`, `crossGuidedBlur` | Radius [0,1000]; epsilon [1e-12,1]; optional guide image with matching dimensions | Two-stage alpha-weighted box moments and coefficient means, with encoded Rec.601 luminance guide. Original coverage retained. |

Median and morphology exclude invisible neighbor RGB; the reference's independent raw RGBA extrema/medians can read it. Minimum/Maximum `affectAlpha=false` preserves source coverage; true may expand/erode coverage, with colors from visible neighborhood samples. Custom convolution substitutes the visible center when a neighbor is fully invisible. Guided statistics use joint source/guide alpha, ignore uncovered data and leave pixels whose guide has no coverage unchanged. The public box-guided kernels are not aliases for the Gaussian-guided development/mask kernels already used by 0.3.

## Complexity and verification

Square box/Gaussian approximation/guided filtering and square morphology are linear in pixel count; explicit small Gaussian kernels, disc median/morphology and Surface Blur scale with neighborhood size. Shape Blur uses row prefix sums but still scales with radius; large-radius disc filters can be expensive on photo-size images. Public radius bounds prevent coordinate overflows, not a latency guarantee. Full-frame arrays can consume substantial memory and allocations return explicit errors.

`tests/core_filters.cpp` checks numerical box/weighted-alpha averages, threshold/detail behavior, exact offset permutations, neutral remaps, centered displacement maps, impulse medians/extrema, HSB/HSL round trips, retained-field reconstruction, seeded noise, hidden-RGB isolation, thin images, malformed geometry and all enum/number bounds. Legacy 0.1 effects and 0.2/0.3 operations retain their existing regression suites.
