# Retouch filters

`<iiGraphicAdjust/RetouchFilters.h>` implements the executable Lightcraft retouch and spot-diagnostic algorithms at commit `629e39380e296f588c64cd9c0053a8edc3528f36`. The reference sources are `crates/pipeline/src/spots.rs`, `redeye.rs`, `visualize.rs`, `dust.rs`, and the three-box blur in `crates/raster/src/blur.rs`. `pipeline/src/lib.rs::lin_cpu` actually applies the spot and eye kernels; `visualize::apply` runs on finished CPU and GPU renders; `engine/src/cmd/masks.rs` exposes spot editing, source refresh, eye correction and `spot.findDust`. None of the seven operations below is a declaration-only feature.

| API | Result | Actual operation |
| --- | --- | --- |
| `cloneSpot` | `Result<Image>` | Bilinear source patch copied through the point-stroke feather mask. |
| `healSpot` | `Result<Image>` | Source detail plus the normalized low-frequency target–source difference from outside the stroke. |
| `removeSpot` | `Result<Image>` | Heal using the best nearby ring-matched source when no offset is supplied. |
| `redEye` | `Result<Image>` | Detect the red pupil, gate by red dominance, neutralize and darken the pupil. |
| `petEye` | `Result<Image>` | Detect the brightest pupil glow, replace its disc with a dark neutral pupil and optionally add a catchlight. |
| `visualizeSpots` | `Result<Image>` | Threshold the absolute high-pass of encoded luminance into black/white. |
| `detectDustSpots` | `Result<vector<DustSpot>>` | Find small, dark, round connected components on locally smooth image regions. |

Two helper APIs expose the algorithms needed for editor interaction: `rankSpotSources` returns scored source offsets, and `detectPupil` returns the pupil center, radius and `found` flag before the size slider is applied. The APIs do not create layers, change document selections or store undo records.

## Image, coordinates and alpha

Images use the normal SDK contract: finite normalized sRGB with straight alpha. All image results preserve dimensions and every input alpha value exactly. The host supplies an already oriented, cropped image; normalized positions refer to that image's width and height. No EXIF transform or reference `Frame` crop transformation is implicit in this module.

Clone, Heal, Remove and eye correction decode sRGB, convert linear sRGB to linear Rec.2020 using the reference primary/white-point construction, execute the scene-linear formulas, then convert back and clamp to the SDR sRGB contract. The reference pipeline can process HDR values; this SDK accepts and returns SDR only. Spot and dust diagnostic luminance is computed directly in encoded sRGB with weights 0.2126, 0.7152 and 0.0722. The reference diagnostics use 8-bit finished renders; this API uses the provided float channels without introducing an 8-bit quantization step.

The RGB-only reference has no alpha policy. This port extends its spatial operations with coverage-weighted statistics and premultiplied bilinear sampling. Fully transparent pixels never supply hidden RGB to source scores, healing, eye detection or diagnostics. Source coverage scales the spot blend, and transparent source patches do not alter a visible target. Transparent target pixels retain their original hidden RGB and alpha. Diagnostic output retains input coverage instead of making the image opaque. These extensions preserve the source formulas for opaque images.

## Spot options and source search

`SpotOptions` contains:

- `points`: 0–4,096 normalized `[x,y]` samples, each in [0,1]. The default is one point at `[.5,.5]`. The source model is the union of circular dabs centered on the supplied points; it does not interpolate missing brush points into segments. Empty points are an exact identity.
- `size`: radius divided by the image's long edge, default .02 and range [.001,.25]. The actual radius is at least one pixel.
- `feather`: default 50, range [0,100]. The mask is `1−smoothstep(r*(1−.8*feather/100), r, distance)` times opacity. Zero feather has a defined hard boundary.
- `opacity`: default 100, range [0,100]. Zero is an exact identity.
- `sourceOffset`: optional `[dx,dy]` in width/height fractions, each in [-1,1]. All three spot APIs accept an explicit source or run source search when it is absent. An explicit source makes Remove and Heal equivalent, matching the reference kernel.

Source sampling uses the reference pixel-center convention `(x+.5,y+.5)` and clamps at image edges. Heal processes the stroke bounds with a one-radius padding. It blurs `(target−source)*outsideWeight` and `outsideWeight` with sigma `max(1,.6*r)`, divides by `max(1e−4, blurredWeight)`, and adds the correction to the source patch. Outside pixels have brush alpha below .02; their weight also excludes invisible source/target coverage. This is the reference frequency-separation/membrane approximation; it is not a gradient-domain Poisson solver or generative inpainting model.

`SpotSourceOptions` contains normalized `target` (default center), `size` as above, and optional `avoidOffset` in [-1,1]. The search checks 16 angles on each of three radius rings, 2.4, 3.2 and 4.2 times the spot radius. Each score compares 72 bilinear annulus samples at 1.05–1.6 radii in square-root scene-linear RGB, plus a source-interior texture penalty at .2–.9 radii. Candidates whose full annulus falls outside the image are omitted. Stable ascending scores preserve candidate order on ties. Refresh excludes offsets within one radius of `avoidOffset`. The helper returns up to 48 candidates and can return an empty vector. The image filter uses the reference fallback offset `(2.5*r,0)` if the search is empty.

## Eye options and pupil detection

`EyeOptions` contains normalized `center` (default `[.5,.5]`), `radiusX` .02 and `radiusY` .015 as fractions of the long edge, each in [.0001,.5], `pupilSize` 50 and `darken` 50 in [0,100], and an optional catchlight offset with both coordinates in [-1,1] pupil radii. Catchlight is active only for Pet Eye and is validated by both APIs. `detectPupil` accepts the same options plus `EyeKind::Red` or `EyeKind::Pet`.

The detector samples a fixed 40×40 grid inside the ellipse. Red weights are `smoothstep(.22,.42,(R−max(G,B))/(R+G+B+1e−6))` for R > .001. Pet weights are `smoothstep(.5,.8,Y/Ymax)` after requiring a maximum at least 1.5 times the mean. The weighted centroid and ellipse-area fraction determine the radius. Detection falls back to the ellipse center and half its smaller radius when evidence is insufficient. `DetectedPupil::found` distinguishes this fallback; the renderer still applies the reference fallback disc.

The size slider multiplies the detected radius by `.5+pupilSize/100`, capped at 1.1 times the larger ellipse radius, with a minimum render radius of .5 px. Red Eye uses a soft disc ending at 1.35 radii and red dominance gate .25–.5. The neutral pupil value is `.5*(G+B)*(1−.9*darken/100)`. Pet Eye uses a disc ending at 1.2 radii, with neutral value `min(Y,.04)*(1−.85*darken/100)`. Its catchlight radius is `max(.5,.22*r)` and its center reaches .9 in scene-linear light. These are photographic pupil models and require a host-selected eye ellipse; no face-detection model is bundled.

## Spot visualization and dust detection

`VisualizeSpotsOptions::sensitivity` defaults to 50 and accepts [0,100]. Blur sigma is `max(.8,.006*longEdge)`, and the threshold is `.01+.2*(1−sensitivity/100)^2`. A larger sensitivity marks weaker bright or dark specks. Pixels with absolute luminance high-pass above the threshold become white; the rest become black.

`DustDetectionOptions` has the same sensitivity range/default and `maxSpots` 60 in [1,60]. Images with either axis below 32 produce an empty result. The surrounding blur uses sigma `max(2,.012*longEdge)`; the local fine-detail texture blur uses `max(3,.02*longEdge)`. A candidate must have luminance above .08, darkening greater than `.03−.022*sensitivity/100`, and darkening greater than three times its local texture. Four-connected blobs must have radius between .0015 and .02 of the long edge, bounding-box aspect ratio at most 1.8, and at least 45% occupancy. Results are sorted by descending peak darkening, contain normalized centers, and use a suggested healing radius 1.6 times the blob radius.

The port completes traversal of oversized candidate blobs and discards them, so a large connected dark region cannot be split into accidental dust detections. This deliberately fixes the reference's early-break traversal. All blur operations retain the source three-box radius formula and clamped boundaries; their implementation uses double-precision running sums and does not substitute a truncated Gaussian for small sigma.

## Validation and work limits

All fields, including inactive optional fields, must be finite and within the above ranges; invalid options return `InvalidParameter`. Images must meet the SDK buffer/pixel validation contract and contain at most 64,000,000 pixels; larger images return `InvalidDimensions`. A spot is rejected when its padded patch area times point count exceeds 128,000,000 point-distance evaluations. The limit bounds dense brush work without silently reducing point density or processing resolution. Allocation failures return `AllocationFailure`. Processing is deterministic and uses no random generator, Qt, GPU, Rust runtime or external file I/O.

## Example and verification

```cpp
#include <iiGraphicAdjust/RetouchFilters.h>
using namespace iiGraphicAdjust;
SpotOptions spot;
spot.points = {{.42, .57}};
spot.size = .015;
spot.feather = 40;
auto repaired = removeSpot(input.view(), spot);
auto candidates = rankSpotSources(input.view(), {{.42, .57}, .015, std::nullopt});
auto dust = detectDustSpots(input.view(), {65, 60});
```

`tests/retouch_filters.cpp` has `spots`, `eyes`, `dust`, and `validation` modes. Independent numerical checks cover scene-linear opacity/coverage blending, constant surrounding-tone healing, shifted clone coordinates, automatic dark-dot removal, 48 source candidates and refresh exclusions, exact grid pupil geometry, Rec.2020 red neutralization, the pet luminance ceiling and .9 catchlight. A planted three-spot sky fixture checks dust count and coordinates; diagnostic tests cover high-pass sensitivity and constant fields. Invalid fields, brush work bounds, hidden RGB exclusion, dimensions, buffers, and 1×1/1×N/N×1 inputs are checked. Normal and ASan/UBSan standalone builds were run. The Adobe renderer and original Rust executable were not run as pixel oracles.
