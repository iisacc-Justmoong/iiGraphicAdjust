# Render filters

The nine APIs in `<iiGraphicAdjust/RenderFilters.h>` implement actual PhotoCraft render paths inspected at commit `5896f0b63e6f342e0311920b4d2a0ea5a94330fc`. Fibers, Lens Flare and Lighting Effects are in `crates/algo/src/render.rs`; photographic Relight is in `relight.rs`; Clouds and Difference Clouds are registered and implemented in `crates/engine/src/extra_cmds.rs`; Flame, Tree and Picture Frame are real seeded geometry renderers in `crates/algo/src/render2.rs`, registered by `crates/engine/src/render_cmds.rs`. They are not declarations or placeholders.

Every API validates normalized finite straight sRGB RGBA, owns the returned `Image`, leaves the source unchanged and reports failures using `Result<Image>`. All render color math and compositing are encoded sRGB. Coordinates, bounds and seeds are image-local. Maximum coordinate extent per axis is 100,000,000 pixels, checked before integer conversions. Invalid options are rejected even when their branch is inactive.

## Individual processing APIs

| API | Contract | Processing |
| --- | --- | --- |
| `clouds` | Normalized foreground black/background white; uint32 seed 0 | Eight octaves of smooth value noise, base scale `clamp(long edge/4,16,512)`, range stretched between colors. Replaces RGB and coverage with the generated color. |
| `differenceClouds` | Same options | Absolute per-channel difference between input and the cloud field; source coverage and fully hidden input values remain exact. |
| `fibers` | Variance 16 and strength 4, each [1,64]; foreground/background and seed | Four anisotropic noise octaves plus fine column grain; variance sets cross-fiber frequency, strength stretches fibers vertically. Replaces color and coverage. |
| `lensFlare` | Brightness 100%, [0,300]; center x/y 0.5, [-0.5,1.5]; Zoom/Prime35/Prime105/MoviePrime | Hot core, broad glow, angular rays, lens-specific ghosts, Prime35 ring or MoviePrime horizontal streak. Screen composite preserves alpha and white. Brightness zero returns the exact input. |
| `lightingEffects` | Up to 64 typed `FilterLight`s; gloss/metallic/exposure/ambience [-100,100]; texture None/Red/Green/Blue/Alpha/Luminance; height [0,100], white-is-high boolean | Height-gradient surface normals; Spot cone/hotspot, Point radial attenuation or Infinite directional light; Lambertian color, gloss-based specular exponent, metallic tint, ambient and exposure. |
| `relight` | Angle 45°, [-180,180]; elevation 40°, [0,90]; intensity 40 and ambient 55, [0,100]; warmth 0, [-100,100]; softness 25, [1,100] | Alpha-weighted blurred Rec.709 encoded luminance estimates shading. Finite differences at the blur scale produce normals; a replacement Lambertian lamp multiplies recovered albedo and warmth tint. Intensity zero is exact identity. |

Each `FilterLight` validates kind Spot/Point/Infinite, normalized x/y/target coordinates [-2,3], z [0,10] short-edge units, angle [-180,180], elevation [0,90], normalized three-channel color, intensity [-100,100], cone [1,89], hotspot [0,100], radius [0.01,10] short-edge units. These fields describe separate light branches, so cone/hotspot apply to Spot, radius to Point, angle/elevation to Infinite. Defaults match the source command's light.

Relight's blur sigma is `clamp(softness/100 * 0.08 * short edge,2,96)`. Its finite difference step is rounded sigma, bounded to [1,32], preventing directional shading from disappearing on larger images. The SDK excludes hidden RGB with alpha-normalized whole-image blur. Any positive accumulated alpha contributes to the normalized field, including representable coverage below `1e-12`; uniform alpha scaling therefore preserves the computed shading. Broad Gaussian windows use the SDK's variance-matched box approximation; the reference uses exact Gaussian on a grown tile and has an 8-million-pixel temporary guard. The SDK does not silently return identity when that reference guard would have fired.

## Procedural render APIs

Flame, Tree and Picture Frame convert their typed options into antialiased tapered capsules and shaded ellipses, then composite the generated premultiplied paint over the input. They can therefore paint into transparent areas. The implementation retains the reference's seeded SplitMix64 stream, source-over/additive/maximum strand blending, and original geometric models. Colors and final coverage are bounded to the SDK's normalized contract; intermediate additive light may exceed one. Generated or composited pixels whose coverage rounds down to zero float alpha have canonical black RGB; untouched transparent source pixels retain their exact value. The reference's tiled raster infrastructure is not imported.

### Flame

`FlameOptions` supports all six source placements: OneAlongPath, MultipleAlongPath, PathDirections, VariousLengths, CandleLight, OneDirection. Options:

- Length 150 px and width 40 px, each [1,1000]; interval 60 px [1,1000]; angle 0° [-180,180]. Angle applies to OneDirection; length controls generated straight flames and candles, while a supplied OneAlongPath spine sets that flame's length.
- RandomizeLength false; AdjustIntervalForLoops true, for multiple placements.
- Optional normalized custom RGBA color; absent selects the five-stop natural flame gradient.
- Turbulence 25, jaggedness 25, opacity 75 and bottomAlignment 20, each [0,100]; flameLines 20 [1,100].
- Styles Normal/Violent/Flat; shapes Parallel/ToCenter/Spread/Oval/Pointed; RandomizeShapes false; seed 0; quality 2 [0,4]. CandleLight uses the source's oval shape and reduced turbulence.
- Up to 256 flattened paths, at most 4,096 total points; point coordinates [-1,000,000,1,000,000] pixels. Empty paths select the source default arc, or a default candle origin. Complexity exceeding the documented 500,000 primitive estimate is rejected; a generated default path that exceeds the same budget reports allocation failure.

The flame model uses arc-length spines, two-octave sway, shorter outer lanes, per-line flicker, base fade and tapered strand tips. Multiple closed-path placement can spread the interval evenly. Opacity zero is an exact identity.

### Tree

`TreeOptions` includes all 34 source tree types through 1-based `baseTreeType`, and lightDirection [1,5] from left to right via overhead. The types are Oak, Maple, Pine, Palm, Birch, Weeping Willow, Cherry Blossom, Bamboo, Cypress, Baobab, Ginkgo, Spruce, Bare Winter Tree, Apple, Acacia, Poplar, Elm, Magnolia, Banana, Bonsai, Jacaranda, Autumn Maple, Golden Autumn, Olive, Fir, Cedar, Lemon, Wisteria, Young Sapling, Dead Tree, Coconut Palm, Shrub, Plum Blossom, Cloud Pine.

LeavesAmount 50 [0,100], LeavesSize 100 [0,200], BranchesHeight 100 [50,300], BranchesThickness 100 [50,200]; defaultLeaves true, customBranchColor false, normalized leaf/bark colors; flatShading false; seed 0; base x=0.5/y=0.95, each [-2,3]; height size=0.8 [0.05,2] times canvas height.

The source type table supplies four architectural models: recursive branching, conifer whorls, palm fronds, bamboo stalks. Per-type spread, droop, taper, bark, leaf form and fruit/blossom accents are retained. Darker leaves paint first. The SDK applies zero leaf size/amount consistently to palm foliage and zero leaf size to other leaf shapes, where the source floors some leaves to a minimum size. Custom leaf/bark alpha is retained by this SDK, whereas the reference's shading helper makes those colors opaque.

### Picture Frame

`PictureFrameOptions` provides VineWithFlowers, VineWithLeaves, Ivy, Roses, Daisies, Berries, Bamboo, Waves, Zigzag, Dots, Rope, Scallops, DoubleLine, Snowflakes, Stars, Hearts. Margin 4% [0,30] of short edge; size 50, arrangement 50, thickness 30, each [1,100]; lines 1 [1,5]; fade 0 [0,100]; normalized vine/flower/leaf colors; seed 0.

The renderer walks a clockwise rectangle by arc length, places flowers/leaves/ornaments or parallel line patterns, then fades ornaments according to how far their primitive anchor reaches inward. `lines` applies to Waves, Zigzag, DoubleLine and Rope, as in the reference. Custom color alpha is retained; this corrects the reference shade helper's unconditional opaque alpha. Degenerate inset line rectangles are bounded to a positive extent for small canvases. The exact capsule/ellipse count of the selected recipe is computed before allocating or generating geometry. More than 500,000 primitives returns `InvalidParameter`; fade and transparent colors do not waive this budget. This bounds long thin canvases and dense ornament spacing without changing accepted recipes.

## Scope and verification

These APIs implement source-derived independent looks, not exact Adobe output. They contain no path editor, document selection, layer creation, undo history, smart-object storage, color management, RAW decode, GPU or external file I/O. Hosts supply flattened Flame paths and call the filter on an image; layer/document services remain above this SDK. The same seed, image size and options give deterministic results.

`tests/render_filters.cpp` has `noise`, `light`, `procedural`, and `validation` modes. It checks independent absolute-difference, ambient-light, exposure, Lambertian and flat-field Relight numerical oracles; all four flare lenses; all six flame placements; all 34 tree types; all 16 frame designs; opacity, seed, leaf amount and fade semantics; hidden RGB isolation; nested option validation and thin images. Regression checks cover canonical generated alpha underflow, tiny-alpha Relight invariance, and all sixteen Picture Frame primitive-count budgets, including transparent colors and full fade. Both normal and ASan/UBSan standalone test executables were run during implementation. An Adobe renderer and the reference Rust executable were not run as pixel oracles.
