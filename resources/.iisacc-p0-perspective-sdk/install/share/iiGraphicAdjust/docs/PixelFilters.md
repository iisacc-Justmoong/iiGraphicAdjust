# Pixelate and stylize filters

These sixteen detached C++23 operations implement the pixel kernels and recipes inspected in PhotoCraft commit `5896f0b63e6f342e0311920b4d2a0ea5a94330fc`. The source algorithms are the reference project's own visual approximations, not Adobe's numerical specification. See `crates/algo/src/pixelate.rs`, `stylize.rs`, `stylize2.rs`, and `oil.rs`; commands and dialog ranges are registered in `crates/engine/src/filters.rs` and `filters_ext.rs`.

Include `<iiGraphicAdjust/PixelFilters.h>`. Every function returns `Result<Image>`, validates dimensions and normalized finite straight sRGB RGBA input, owns its output, and leaves its input untouched. Filter colors, statistics, luminance and geometry use **encoded sRGB**, unlike the existing linear-light `gaussianBlur` and `sharpen` APIs. Edge samples repeat the closest image pixel. Coordinates are image-local, with centers at `(x + 0.5, y + 0.5)`; dimensions greater than 100,000,000 along either axis are rejected before coordinate conversions.

## Individual APIs and parameter contracts

| API | Parameters and defaults | Pixel work |
| --- | --- | --- |
| `colorHalftone` | `maximumRadius` 8 px, [4,127]; three RGB `angles` {108,162,90}, each [-360,360] degrees | Three separately rotated screens; five alpha-weighted taps determine ink. Dot area follows ink until dots touch, then radius grows toward the cell corners; coverage uses antialiased disc edges. |
| `crystallize` | `cellSize` 10 px, [3,300]; `seed` 0, uint32 | Jittered Voronoi cells, owner selected from a 3×3 lattice neighborhood; each cell becomes its premultiplied average. |
| `facet` | None | Radius-two Kuwahara smoothing: choose the least-variable alpha-weighted luminance quadrant and its RGB mean. |
| `fragment` | None | Premultiplied average of four copies displaced 4 px left, right, up and down. |
| `mezzotint` | `pattern` FineDots; `seed` 0 | Each RGB channel becomes 0 or 1 according to a coordinate hash threshold. Patterns: FineDots, MediumDots, GrainyDots, CoarseDots, ShortLines, MediumLines, LongLines, ShortStrokes, MediumStrokes, LongStrokes. |
| `pointillize` | `cellSize` 5 px, [3,300]; `seed` 0; normalized `background` opaque white | Jittered dots with random radius, sampled center color, small brightness jitter and antialiased coverage. The deepest overlapping dot wins. |
| `mosaic` | Scalar `cellSize` 10 px, [2,200], rounded to integer pixels | Origin-anchored square cells replaced by their premultiplied average; partial edge cells use only their actual pixels. |
| `emboss` | `angle` 135°, [-180,180]; `height` 3 px, [1,100]; `amount` 100%, [1,500] | Directional luminance difference around neutral gray 0.5. |
| `findEdges` | None | Per-channel Sobel gradient magnitude, divided by four, inverted to dark edges on white. |
| `solarize` | None | Encoded channels greater than 0.5 become `1-v`; all other channels remain unchanged. |
| `diffuse` | `mode` Normal; `seed` 0 | Normal shuffles one random neighbor; DarkenOnly/LightenOnly accept only the corresponding luminance direction. Anisotropic finds the most similar of eight neighbors, then moves halfway toward it. |
| `extrude` | `shape` Blocks or Pyramids; `size` 30 px, [2,255]; `depth` 30, [1,255]; `levelBased`, `solidFrontFaces`, `maskIncompleteBlocks` false; `seed` 0 | Cells raise shaded pyramids or swept square blocks. The highest candidate block wins; front faces may retain shifted source texture or use the cell mean. Depth controls displacement or pyramid slope; level-based height uses the cell's luminance. |
| `oilPaint` | `stylization` 4, [0.1,10]; `cleanliness` 5, [0,10]; `scale` 1, [0.1,10]; `bristleDetail` 5, [0,10]; `lighting` true; `angle` -60°, [-180,180]; `shine` 1, [0,10] | Clean premultiplied color, Gaussian structure tensor, minor-eigenvector stroke field, bidirectional line integral convolution, convolved bristle noise, then optional directional diffuse/specular relief. |
| `tiles` | `count` 10, [1,99] along the shorter side; `maximumOffset` 10%, [0,99]; `fill` Background; normalized foreground black/background white; `seed` 0 | Randomly shifted cells, highest row-major tile on top. Gaps use Background, Foreground, Inverse, or Unaltered. Zero displacement is an identity for visible input. |
| `traceContour` | `level` 128, [0,255]; `upper` false | Per-channel four-neighbor threshold crossing, traced on the selected lower or upper side. |
| `wind` | `method` Wind, Blast, or Stagger; `fromRight` true; `seed` 0 | Edge-launched random streaks with 16 px/40 px maximum reach; Stagger displaces random row segments up to 12 px. |

All numeric and enum fields are checked, including fields whose branch is inactive. Unknown enum values, NaN/Infinity, malformed colors, empty/overflowed dimensions and mismatched buffers produce explicit errors. No SDK function silently substitutes a default for invalid parameters.

## Alpha and source differences

Spatial averages and bilinear samples operate on premultiplied encoded RGB, so hidden RGB never becomes a visible neighbor color. Crystallize, Mosaic and Fragment also average coverage. Normal Diffuse, Wind, Tiles and Extrude may move or blend coverage with their colors. Pointillize first establishes its background coverage and then premultiplied blends its dot; this corrects the reference's straight-color interpolation of partially transparent samples. Newly transparent spatial pixels use canonical black RGB, including when a positive double-precision average rounds down to zero float alpha.

Facet, Emboss, Find Edges, Anisotropic Diffuse, Oil Paint, Mezzotint, Solarize, Color Halftone and Trace Contour keep source coverage and retain the exact source value when alpha is zero. Their weighted color ratios use double precision before restoring the source alpha, so representable tiny coverage still receives the filter. Derivative filters use the center's visible color when a neighbor is fully transparent; partial-alpha averaging operations weight its RGB by alpha. Directional Diffuse compares a fully hidden center as black, keeping its acceptance decision independent of that center's hidden RGB. The reference reads hidden RGB in some of these paths; this SDK deliberately excludes it.

Oil Paint's private Gaussian helper uses exact truncated kernels for sigma below three and three variance-matched box passes for larger sigma. Both the reference and SDK use box approximations for broad smoothing, but floating precision, boundary treatment and pass selection can differ. Tree/texture appearance is not involved in Oil Paint's structure-tensor/LIC model.

Color Halftone retains the reference's maximum-of-disc-coverage approximation; antialiasing at full-ink intersections can leave small light seams. It is an RGB screen effect, not CMYK separation or a print raster engine. Its three angles correspond to the three channels supported by this SDK; the reference's fourth angle belongs to CMYK. Mezzotint, Voronoi and tile random streams use the reference's uint32 hash and are deterministic for the same input, size and seed.

Extrude currently tests nearby cells for every output pixel. Small blocks at maximum depth can be expensive because candidate reach grows relative to block size. Oil Paint and the other spatial kernels retain whole-image working arrays; no streaming, GPU, selection, document history or smart-object storage is introduced by these APIs.

## Usage and tests

```cpp
#include <iiGraphicAdjust/PixelFilters.h>

iiGraphicAdjust::CrystallizeOptions cells;
cells.cellSize = 14;
cells.seed = 42;
auto crystal = iiGraphicAdjust::crystallize(source.view(), cells);
if (!crystal) return crystal.error();

iiGraphicAdjust::OilPaintOptions paint;
paint.stylization = 6;
paint.lighting = false;
auto result = iiGraphicAdjust::oilPaint(crystal->view(), paint);
```

`tests/pixel_filters.cpp` has `pixelate`, `stylize`, and `validation` modes. Numeric checks include an independent premultiplied cell mean, four-offset Fragment oracle, integrated Voronoi RGB conservation, binary Mezzotint output, Sobel step edges, lower/upper threshold contours, flat-field Oil Paint, directional diffuse acceptance, and seeded layout changes. Every operation is checked against hidden-RGB substitution, 1×1/1×N/N×1 input, malformed dimensions and invalid parameters. Regression checks cover subnormal alpha rounding, tiny-alpha Oil Paint smoothing, source-alpha-preserving averages, and both directional Diffuse modes with hidden center substitution. Reference Rust tests and an Adobe application have not been executed as an oracle.
