# Mesh and Perspective Filters

`MeshFilters.h` implements five individual image operations from Photocraft's actual CPU sources, and four geometry inspection functions used by the same algorithms. The reference is [Photocraft commit `5896f0b63e6f342e0311920b4d2a0ea5a94330fc`](https://github.com/storytold/photocraft/tree/5896f0b63e6f342e0311920b4d2a0ea5a94330fc), specifically `crates/algo/src/liquify.rs`, `wideangle.rs`, `vanishing.rs`, `puppet.rs`, `perspective.rs`, `warp.rs` and `photo_util.rs`. These are replayable CPU implementations, with no GUI, Qt, Rust dependency or Adobe pixel-parity claim.

## Public API

| Image function | Options | Geometry inspection |
|---|---|---|
| `liquify` | `LiquifyOptions` | Stroke replay produces the internal inverse field; no separate persistent field handle is exposed. |
| `adaptiveWideAngle` | `AdaptiveWideAngleOptions` | `solveAdaptiveWideAngle` returns the actual source/destination mesh, source constraint curves and maximum mapped line residual. |
| `puppetWarp` | `PuppetWarpOptions` | `solvePuppetWarp` returns the coverage mesh and its actual deformed vertices and depth-ordered triangles. |
| `perspectiveWarp` | `PerspectiveWarpOptions` | `mapPerspectiveWarpPoint` evaluates the same continuous piecewise homography used by rendering. |
| `vanishingPoint` | `VanishingPointOptions` | `vanishingPointPlanes` returns the original planes followed by actual projected 3-D tear-off planes. |

All image functions return `Result<Image>`. Geometry functions return their typed `Result` value. `MeshGeometry` contains source and destination positions, triangle vertex indices in drawing order, and wide-angle source curves/residual. Geometry calls are needed to display actual constraints and meshes in a host editor and to inspect the numerical solve.

## Image, coordinates and bounds

`MeshPoint` is a pair of finite **image-pixel coordinates**, with pixel centers at `(x + 0.5, y + 0.5)`. `MeshQuad` has four corners clockwise from the plane's top-left. Strictly convex counterclockwise quads are also accepted; crossed, collinear, duplicate or nearly degenerate corners are rejected. Point input coordinates are limited to `[-1000000,1000000]` on each axis, and images to one million pixels per axis. Output retains the input dimensions and top-left origin. **Warped content outside that fixed canvas is clipped**, whereas Photocraft's sparse `Surface` can grow to new document bounds.

Images use normalized encoded sRGB and straight RGBA. Interpolation operates on premultiplied encoded RGB and alpha. `Nearest`, `Bilinear` and Catmull-Rom `Bicubic` are selectable for mesh rendering; Liquify uses the reference's bicubic field and bicubic source sampling. Geometry warps move alpha with the content. Uncovered output pixels are transparent black. Unchanged/neutral operations return detached exact copies, retaining zero-alpha RGB; interpolated invisible pixels are canonical transparent black. Invisible RGB never contributes to visible interpolated color. Bicubic overshoot is clamped to the normalized output contract.

Options are validated even when neutral or inactive. Invalid enums, NaN/Inf, numeric limits, cardinality limits and degenerate geometry produce structured errors. Image/storage errors retain their corresponding image error code. Allocation/container failures produce `AllocationFailure`. An empty operation list is valid and returns a detached copy after validation.

## Liquify

The regular field stores inverse displacement `d(p)` so destination `p` reads source `p + d(p)`. Each brush dab composes its inverse map with a frozen snapshot of the previous field: `d'(p) = q + d(q) - p`. This is the original compositional algorithm, rather than adding unrelated displacement vectors.

| Field | Range / default |
|---|---|
| `cell` | `0` chooses 2 pixels/node at up to 4.2 MP and 4 beyond; explicitly `[1,64]` pixels/node. Field limit: four million nodes. |
| `strokes` | At most 64, at most 4096 recorded points in total, and at most 16384 interpolated brush dabs. |
| Stroke `size` | Diameter `[1,15000]` pixels; default 100. |
| Stroke `density` | `[0,100]`; default 50. Higher values increase the radial quadratic feathering. |
| Stroke `pressure` | `[0,100]`; default 100. Multiplied by per-point pressure. |
| Stroke `rate` | `[0,100]`; default 80. Controls stationary tool speed. |
| Point `pressure` | `[0,1]`; default 1. |
| Stroke `amount` | Optional. Reconstruct All `[0,100]`, default 100%; Lasso Mask `[0,1]`, default 1. Lasso values above 0.5 freeze; the rest thaw. |

All **15 tools** are implemented: Forward Warp, Reconstruct, Smooth, Twirl Clockwise, Twirl Counterclockwise, Pucker, Bloat, Push Left, Freeze, Thaw, Lasso Mask, Reconstruct All, Freeze All, Thaw All and Invert Freeze. Global tools ignore points after validating them. Lasso is a closed even-odd polygon; fewer than three points do nothing. Freeze scales deformation strength, but does not block Freeze/Thaw mask painting. Moving tools dab at a fifth of the brush radius, at least half a pixel; Forward Warp centers each dab where the brush was. Stationary tools also apply the first recorded dab and repeated still points.

Snapshots cover affected nodes plus the required composed sampling margin. The field is composed with bilinear sampling and rendered with Catmull-Rom upsampling. The source edge repeats beyond image bounds, matching Liquify's reference edge behavior. Face-aware AI, hand/zoom controls, preview proxies and UI state serialization are not part of this kernel API.

## Adaptive Wide Angle

This implements the reference's **content-preserving least-squares mesh**, not a generic inverse-distance warp. The camera projects image points into unit rays using equidistant Fisheye, rectilinear Perspective or full equirectangular Spherical geometry. A scene line is sampled at 25 points on the great circle between its endpoint rays. Antipodal endpoint rays are rejected because they do not define a unique great circle. A constraint whose derived focal length underflows to zero or whose camera projection is nonfinite also returns `InvalidParameter`; nonfinite coordinates never reach grid-index conversion. Explicit positive focal lengths do not fall back to the zero-value default when the crop-factor product underflows.

Each source grid triangle supplies similarity-preserving equations of the Igarashi/Carroll form. Both diagonals are included in the shape energy. Weak source anchors fix the global similarity. Every constrained curve sample supplies a line equation relative to its first sample, with a fixed horizontal/vertical normal or an iteratively re-estimated free normal. Direct CGLS solves up to four outer rounds. The mesh is then scaled about the image center and rasterized with source-coordinate barycentric interpolation.

| Field | Range / default |
|---|---|
| `model` | Auto, Fisheye, Perspective, FullSpherical; Auto default. Auto resolves 2:1 frames to Spherical, positive 35-mm-equivalent focal lengths below 17 mm to Fisheye, otherwise Perspective. |
| `focalLength` | `[0,1000]` mm; default 0. Zero selects 12 mm for Fisheye or 24 mm otherwise. |
| `cropFactor` | `[0.1,10]`; default 1. Multiplies focal length to its 35-mm equivalent. Pixel focal length uses a 36-mm long image side. |
| `scale` | `[10,1000]` percent; default 100. |
| `constraints` | At most 32; distinct finite endpoint pairs, each Free/Horizontal/Vertical. |
| `meshCells` | Long-side grid target `[2,96]`; default 48. Each axis has at least two cells. |
| `solverIterations` | `[1,2000]`; default 1000. Caps each reference round's 400, 600, 800 or 1000 CGLS iterations. |
| `interpolation` | Nearest/Bilinear/Bicubic; default Bicubic. |

With no constraints, the lens model alone does not remap pixels; only output Scale can change the image, matching the source's mesh solve. `constraintResidual` measures the maximum mapped deviation from each curve's endpoint chord in pixels. It is a diagnostic, not an enforced success threshold.

## Puppet Warp

This ports the reference's **coverage-driven triangle mesh and cotangent-weighted ARAP solver**. Alpha greater than 0.05 marks coverage. Coverage is accumulated on blocks at roughly one quarter of the requested mesh spacing, then grown or eroded with a round stamp. Alternating diagonals split every grid cell touching coverage. Exact squared Euclidean distance transforms replace radius-squared stamp scans while preserving round integer-block membership, including uncovered space beyond the grid boundary.

Pins bind to the barycentric point beneath them; outside the mesh they bind to the nearest vertex. Components without any pin remain anchored at their source positions. The global matrix combines cotangent edges, stiff barycentric pin constraints and unpinned-component anchors. The initial solve uses identity rotations, or an angle initialization from explicitly rotated pins. Every local step fits each triangle's covariance rotation and mode-dependent scale; every global step solves the resulting position system. Explicit pin rotations fix the triangles around the nearest bound vertex. Triangle depth comes from its nearest source pin, and stable back-to-front sorting preserves depth behavior.

| Field | Range / default |
|---|---|
| `pins` | At most 64. Each has finite source/destination image positions, optional rotation and depth. |
| Pin `rotation` | Optional `[-360,360]` degrees; automatic when absent. Zero remains an explicit fixed rotation. |
| Pin `depth` | Integer `[-10000,10000]`; default 0. Higher values render later in overlaps. |
| `mode` | Rigid fits rotation; Normal fits rotation times square-root similarity scale; Distort fits full similarity scale. Normal default. |
| `density` | Fewer/Normal/More: approximately 12/20/32 cells along the long side, with spacing at least two pixels. Normal default. |
| `expansion` | `[-200,200]` pixels; default 2. Positive grows coverage; negative erodes it. |
| `iterations` | `[1,100]` local/global rounds; default 40. |
| `solverIterations` | `[1,2000]`; default 2000. Applies to the iterative fallback; direct factorization needs no iteration cap. |
| `interpolation` | Nearest/Bilinear/Bicubic; default Bicubic. |

The source's banded Cholesky fast path is retained, with a four-million-double factor-storage cap (32 MB, lower than the source's 40-million cap). If the band is too large or cannot be factored, the solver uses the source's Jacobi-preconditioned conjugate-gradient fallback, warm-started from the current positions. The coverage grid is limited to sixteen million blocks and the complete candidate mesh grid to 16384 nodes. Extreme expansion on a very small image can exceed this explicitly bounded processing contract and returns `InvalidParameter`.

## Perspective Warp

Each plane maps source quad → unit square → destination quad through actual projective homographies. Source corners within half a pixel are linked and averaged in both source and destination. Adjacent planes identify shared edges. To prevent differently parameterized homographies from tearing along a shared edge, each plane adds the difference between its edge map and the mean of both edge maps, fading linearly across the plane. Points outside all planes use the nearest source quad. This is the original continuous forward mapping.

| Field | Range / default |
|---|---|
| `planes` | At most 16 convex nondegenerate source/destination quads; empty means identity. |
| `straighten` | None/Horizontal/Vertical/Auto; None default. Linked destination corners are leveled/plumbed together based on edge direction. |
| `cell` | `[1,64]` source pixels; default 4. Forward tessellation has at most 512 cells per axis. |
| `interpolation` | Nearest/Bilinear/Bicubic; default Bicubic. |

The same `mapPerspectiveWarpPoint()` map drives source-grid vertices. Each cell is rasterized as two textured triangles; later triangles win folded overlaps, matching the reference's lower-right draw order. Nearly degenerate linked/straightened quads are rejected after preparation as well as before it.

## Vanishing Point

This implements perspective plane construction, metric image paste and metric clone strokes. Opposite edge intersections supply vanishing directions; when both are finite, `f² = -(v1 - center)·(v2 - center)` estimates focal length for orthogonal plane directions. If it cannot be estimated, the input's long side is used. Rays back-project into a 3-D plane normal to the two directions, yielding metric side lengths and source-aspect preservation. Tear-offs rotate an outward plane about a shared 3-D edge using Rodrigues' formula and choose the valid camera-facing projection, preserving the shared edge.

| Field | Range / default |
|---|---|
| `planes` | At most 16 initial convex planes; default empty. Total after tear-offs at most 32. |
| `tearOffs` | At most 16, applied in order. `from` refers to a plane already present, including earlier tear-offs. |
| Tear `edge` | Top/Right/Bottom/Left; default Top. |
| Tear `angle`, `depth` | `[1,179]` degrees, default 90; `[0.05,10]` parent-adjacent-side lengths, default 1. Invalid or degenerate projected planes return an error. |
| `focalLength` | `0` estimates/falls back; explicitly `[1,100000]` pixels. |
| `pastes` | At most 32. Borrowed source `ImageView` must remain alive throughout the call. |
| Paste `plane`, `at`, `width`, `opacity` | Valid plane index; plane coordinates `[-2,3]`, default `(0,0)`; metric plane-width fraction `[0.001,10]`, default 1; opacity `[0,1]`, default 1. |
| `clones` | At most 64, at most 4096 recorded dabs in total, applied after pastes. |
| Clone `source`, `points` | Finite image positions. Source and first destination point select planes; when either point is outside every plane, the stroke has no effect. |
| Clone `size`, `hardness`, `opacity` | Diameter `[1,4096]` pixels at first dab, default 100; hardness `[0,1]`, default 0.5; opacity `[0,1]`, default 1. |

A paste maps its source rectangle into the selected plane's metric coordinates, preserving source proportions, and clips to the plane. **The SDK composites ordered pastes over the input with premultiplied source-over**, while the upstream low-level `paste()` returns a separate transparent warped surface for its caller to composite. Clone strokes keep a fixed metric source/destination offset, including cross-plane metric scales. Each stroke snapshots the current image once, so dabs never recursively sample that same stroke's newly painted pixels. The image-space dab footprint is obtained by projecting 16 metric-circle samples; footprints wider/taller than 4096 pixels are skipped as in the reference. Clone interpolation and blending use premultiplied encoded RGBA, improving hidden-color isolation over the reference clone's straight-channel interpolation.

## Verification

`tests/mesh_filters.cpp` supports `liquify`, `wide`, `puppet`, `perspective`, `vanishing`, `validation` or `all` selections. Tests exercise all Liquify tools and mask/global operations, recorded pressure, exact full reconstruction, every camera/orientation model, real fisheye source curvature and solved line residual, every Puppet mode/density, fixed and moved barycentric pins, a rigid single-pin translation oracle, explicit rotation/depth paths, disconnected unpinned components, and signed coverage expansion.

Independent perspective oracles check all quad corners and a known trapezoid's exact center `(50,37.5)`, linked-edge continuity, straighten behavior and a premultiplied fractional-translation alpha case. Vanishing tests check source-aspect-preserving paste, plane clipping, a projected tilted plane's 3-D tear-off/shared edge and known metric clone transfer. All five operations check neutral identity, finite normalized output, deterministic replay, hidden RGB isolation, malformed input/controls and thin images. These are source/SDK and numerical guarantees; no Adobe screenshot parity, extended-document-canvas output, editor interaction, other-platform runtime or RAW camera-metadata workflow is claimed.

The validation selection checks all four camera models with subnormal focal lengths and three crop factors, both with and without a line constraint. It verifies finite geometry/constraint coordinates, normalized output, exact unconstrained identity and structured projection errors, including the fisheye overflow and focal-product-underflow regressions.
