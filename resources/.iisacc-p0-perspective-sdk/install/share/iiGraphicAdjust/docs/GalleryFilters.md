# Photocraft Filter Gallery

The SDK exposes all **47 Filter Gallery effects as separate C++23 functions and typed option structs** in `GalleryFilters.h`, plus `filterGallery()` for ordered composition. Formulas are adapted from Photocraft's actual `artistic_fx.rs` branches at commit [`5896f0b63e6f342e0311920b4d2a0ea5a94330fc`](https://github.com/storytold/photocraft/tree/5896f0b63e6f342e0311920b4d2a0ea5a94330fc). The upstream metadata is `crates/algo/src/artistic.rs`; kernel code is `artistic_fx.rs`, deterministic hashes are in `noise.rs`, and box radii/cell points are in `fxutil.rs`.

These are Photocraft's artistic approximations, not Adobe pixel-parity implementations. No Rust or Qt is linked. The MIT attribution is retained in `third_party/photocraft`.

## Common contract

- Input and output are row-major normalized **encoded sRGB**, with straight RGBA. Each call returns detached storage. Luma for these formulas is `0.299 R + 0.587 G + 0.114 B` in encoded sRGB, matching this gallery's reference.
- All 47 effects retain the original alpha, including displacement effects (`glass`, `oceanRipple`, `spatter`, `sprayedStrokes`). Zero-alpha pixels retain their complete original RGBA. The geometry of the alpha mask is not displaced.
- Spatial means, Gaussian smoothing, directional blurs, Kuwahara moments and bilinear samples normalize color weights by alpha. Zero-alpha RGB does not influence visible results. A fully invisible sample neighborhood falls back to the destination color. Sobel reads at invisible neighbors fall back to the current center value.
- Colors use `GalleryColor = std::array<double, 3>` with three finite normalized encoded RGB channels. Colors contain no alpha because these effects preserve source alpha. Foreground defaults to black, background to white, and Neon Glow color to `{0, 0.62, 1}`.
- Numeric ranges are inclusive and finite. Out-of-range numeric values, NaN/Inf, invalid enum casts and invalid colors return `InvalidParameter`; values are not silently clamped. Fractional slider values are accepted. Number Of Levels rounds; quantities used as pixel radii truncate as specified by the formula.
- Empty/overflow dimensions, dimensions above `INT32_MAX`, mismatched pixel storage and unnormalized/nonfinite input channels return structured errors. Allocation and container-limit failures return `AllocationFailure`.
- Procedural controls expose a `uint32_t seed`, default `0`, XORed into the upstream fixed hash salts. The same input/options/seed reproduces the same result. The coordinate origin is the top-left of the complete input image. There is no document-origin or independent-tile API in this version.
- A seed affects only procedural components that the selected effect/brush/texture enables. For example, Paint Daubs' Simple brush has no noise component, Watercolor's texture level 1 sets noise amplitude to zero, and Glass Blocks/Tiny Lens are deterministic geometric textures.
- Every output RGB is clamped to `[0,1]` after its individual effect. `filterGallery()` accepts at most 64 typed effects, applies them in order with `std::visit`, and stops on the first error. Empty stacks still validate input and return detached copies. **Unlike Photocraft's unclamped internal gallery stack, SDK stacks clamp between effects to honor the normalized `Image` contract.** This can change overshoot behavior across a multi-effect stack.

```cpp
#include <iiGraphicAdjust/GalleryFilters.h>

using namespace iiGraphicAdjust;
GalleryColoredPencilOptions pencil;
pencil.pencilWidth = 6;
pencil.paperBrightness = 40;
auto drawn = coloredPencil(input.view(), pencil);

const std::array<GalleryEffect, 2> effects{
    GalleryCutoutOptions{},
    GalleryPosterEdgesOptions{}
};
auto stacked = filterGallery(input.view(), effects);
```

## Effect parameters

The notation `field [min,max] = default` gives the public range and default. Named choices are typed enums; the first choice shown is the default. All procedural `seed` fields are `uint32_t` and default to zero. Each effect has its own `Gallery{Name}Options` struct. `galleryFilmGrain` is named explicitly to distinguish this Filter Gallery effect from the existing Lightroom-style `filmGrain` API.

| Category | Function / option type | Parameters |
|---|---|---|
| Artistic | `coloredPencil` / `GalleryColoredPencilOptions` | `pencilWidth` [1,24] = 4; `strokePressure` [0,15] = 8; `paperBrightness` [0,50] = 25; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Artistic | `cutout` / `GalleryCutoutOptions` | `numberOfLevels` [2,8] = 4; `edgeSimplicity` [0,10] = 4; `edgeFidelity` [1,3] = 2 |
| Artistic | `dryBrush` / `GalleryDryBrushOptions` | `brushSize` [0,10] = 2; `brushDetail` [0,10] = 8; `texture` [1,3] = 1; `seed` = 0 |
| Artistic | `galleryFilmGrain` / `GalleryFilmGrainOptions` | `grain` [0,20] = 4; `highlightArea` [0,20] = 0; `intensity` [0,10] = 10; `seed` = 0 |
| Artistic | `fresco` / `GalleryFrescoOptions` | `brushSize` [0,10] = 2; `brushDetail` [0,10] = 8; `texture` [1,3] = 1; `seed` = 0 |
| Artistic | `neonGlow` / `GalleryNeonGlowOptions` | `glowSize` [-24,24] = 5; `glowBrightness` [0,50] = 15; `glowColor` RGB = `{0,.62,1}`; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}` |
| Artistic | `paintDaubs` / `GalleryPaintDaubsOptions` | `brushSize` [1,50] = 8; `sharpness` [0,40] = 7; `brushType` (GalleryBrushType): `Simple`, `LightRough`, `DarkRough`, `WideSharp`, `WideBlurry`, `Sparkle`; `seed` = 0 |
| Artistic | `paletteKnife` / `GalleryPaletteKnifeOptions` | `strokeSize` [1,50] = 25; `strokeDetail` [1,3] = 3; `softness` [0,10] = 0 |
| Artistic | `plasticWrap` / `GalleryPlasticWrapOptions` | `highlightStrength` [0,20] = 15; `detail` [1,15] = 9; `smoothness` [1,15] = 7 |
| Artistic | `posterEdges` / `GalleryPosterEdgesOptions` | `edgeThickness` [0,10] = 2; `edgeIntensity` [0,10] = 1; `posterization` [0,6] = 2 |
| Artistic | `roughPastels` / `GalleryRoughPastelsOptions` | `strokeLength` [0,40] = 6; `strokeDetail` [1,20] = 4; `texture` (GalleryTexture): `Canvas`, `Brick`, `Burlap`, `Sandstone`; `scaling` [50,200] = 100; `relief` [0,50] = 20; `light` (GalleryLightDirection): `Bottom`, `BottomLeft`, `Left`, `TopLeft`, `Top`, `TopRight`, `Right`, `BottomRight`; `invert` = false; `seed` = 0 |
| Artistic | `smudgeStick` / `GallerySmudgeStickOptions` | `strokeLength` [0,10] = 2; `highlightArea` [0,20] = 0; `intensity` [0,10] = 10 |
| Artistic | `sponge` / `GallerySpongeOptions` | `brushSize` [0,10] = 2; `definition` [0,25] = 12; `smoothness` [1,15] = 5; `seed` = 0 |
| Artistic | `underpainting` / `GalleryUnderpaintingOptions` | `brushSize` [0,40] = 6; `textureCoverage` [0,40] = 16; `texture` (GalleryTexture): `Canvas`, `Brick`, `Burlap`, `Sandstone`; `scaling` [50,200] = 100; `relief` [0,50] = 4; `light` (GalleryLightDirection): `Top`, `TopRight`, `Right`, `BottomRight`, `Bottom`, `BottomLeft`, `Left`, `TopLeft`; `invert` = false; `seed` = 0 |
| Artistic | `watercolor` / `GalleryWatercolorOptions` | `brushDetail` [1,14] = 9; `shadowIntensity` [0,10] = 1; `texture` [1,3] = 1; `seed` = 0 |
| Brush Strokes | `accentedEdges` / `GalleryAccentedEdgesOptions` | `edgeWidth` [1,14] = 2; `edgeBrightness` [0,50] = 38; `smoothness` [1,15] = 5 |
| Brush Strokes | `angledStrokes` / `GalleryAngledStrokesOptions` | `directionBalance` [0,100] = 50; `strokeLength` [3,50] = 15; `sharpness` [0,10] = 3; `seed` = 0 |
| Brush Strokes | `crosshatch` / `GalleryCrosshatchOptions` | `strokeLength` [3,50] = 9; `sharpness` [0,20] = 6; `strength` [1,3] = 1; `seed` = 0 |
| Brush Strokes | `darkStrokes` / `GalleryDarkStrokesOptions` | `balance` [0,10] = 5; `blackIntensity` [0,10] = 6; `whiteIntensity` [0,10] = 2 |
| Brush Strokes | `inkOutlines` / `GalleryInkOutlinesOptions` | `strokeLength` [1,50] = 4; `darkIntensity` [0,50] = 20; `lightIntensity` [0,50] = 10 |
| Brush Strokes | `spatter` / `GallerySpatterOptions` | `sprayRadius` [0,25] = 10; `smoothness` [1,15] = 5; `seed` = 0 |
| Brush Strokes | `sprayedStrokes` / `GallerySprayedStrokesOptions` | `strokeLength` [0,20] = 12; `sprayRadius` [0,25] = 7; `strokeDirection` (GalleryStrokeDirection): `RightDiagonal`, `Horizontal`, `LeftDiagonal`, `Vertical`; `seed` = 0 |
| Brush Strokes | `sumiE` / `GallerySumiEOptions` | `strokeWidth` [3,15] = 10; `strokePressure` [0,15] = 2; `contrast` [0,40] = 16 |
| Distort | `diffuseGlow` / `GalleryDiffuseGlowOptions` | `graininess` [0,10] = 6; `glowAmount` [0,20] = 10; `clearAmount` [0,20] = 15; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Distort | `glass` / `GalleryGlassOptions` | `distortion` [0,20] = 5; `smoothness` [1,15] = 3; `texture` (GalleryGlassTexture): `Frosted`, `Blocks`, `Canvas`, `TinyLens`; `scaling` [50,200] = 100; `invert` = false; `seed` = 0 |
| Distort | `oceanRipple` / `GalleryOceanRippleOptions` | `rippleSize` [1,15] = 9; `rippleMagnitude` [0,20] = 9; `seed` = 0 |
| Sketch | `basRelief` / `GalleryBasReliefOptions` | `detail` [1,15] = 13; `smoothness` [1,15] = 3; `light` (GalleryLightDirection): `Bottom`, `BottomLeft`, `Left`, `TopLeft`, `Top`, `TopRight`, `Right`, `BottomRight`; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}` |
| Sketch | `chalkCharcoal` / `GalleryChalkCharcoalOptions` | `charcoalArea` [0,20] = 6; `chalkArea` [0,20] = 6; `strokePressure` [0,5] = 1; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Sketch | `charcoal` / `GalleryCharcoalOptions` | `charcoalThickness` [1,7] = 1; `detail` [0,5] = 5; `lightDarkBalance` [0,100] = 50; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Sketch | `chrome` / `GalleryChromeOptions` | `detail` [0,10] = 4; `smoothness` [0,10] = 7 |
| Sketch | `conteCrayon` / `GalleryConteCrayonOptions` | `foregroundLevel` [1,15] = 11; `backgroundLevel` [1,15] = 7; `texture` (GalleryTexture): `Canvas`, `Brick`, `Burlap`, `Sandstone`; `scaling` [50,200] = 100; `relief` [0,50] = 4; `light` (GalleryLightDirection): `Top`, `TopRight`, `Right`, `BottomRight`, `Bottom`, `BottomLeft`, `Left`, `TopLeft`; `invert` = false; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Sketch | `graphicPen` / `GalleryGraphicPenOptions` | `strokeLength` [1,15] = 15; `lightDarkBalance` [0,100] = 50; `strokeDirection` (GalleryStrokeDirection): `RightDiagonal`, `Horizontal`, `LeftDiagonal`, `Vertical`; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Sketch | `halftonePattern` / `GalleryHalftonePatternOptions` | `size` [1,12] = 1; `contrast` [0,50] = 5; `patternType` (GalleryPatternType): `Dot`, `Circle`, `Line`; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}` |
| Sketch | `notePaper` / `GalleryNotePaperOptions` | `imageBalance` [0,50] = 25; `graininess` [0,20] = 10; `relief` [0,25] = 11; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Sketch | `photocopy` / `GalleryPhotocopyOptions` | `detail` [1,24] = 7; `darkness` [1,50] = 8; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}` |
| Sketch | `plaster` / `GalleryPlasterOptions` | `imageBalance` [0,50] = 20; `smoothness` [1,15] = 2; `light` (GalleryLightDirection): `Top`, `TopRight`, `Right`, `BottomRight`, `Bottom`, `BottomLeft`, `Left`, `TopLeft`; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}` |
| Sketch | `reticulation` / `GalleryReticulationOptions` | `density` [0,50] = 12; `foregroundLevel` [0,50] = 40; `backgroundLevel` [0,50] = 5; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Sketch | `stamp` / `GalleryStampOptions` | `lightDarkBalance` [0,50] = 25; `smoothness` [1,50] = 5; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}` |
| Sketch | `tornEdges` / `GalleryTornEdgesOptions` | `imageBalance` [0,50] = 25; `smoothness` [1,15] = 11; `contrast` [1,25] = 17; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Sketch | `waterPaper` / `GalleryWaterPaperOptions` | `fiberLength` [3,50] = 15; `brightness` [0,100] = 60; `contrast` [0,100] = 80; `seed` = 0 |
| Stylize | `glowingEdges` / `GalleryGlowingEdgesOptions` | `edgeWidth` [1,14] = 2; `edgeBrightness` [0,20] = 6; `smoothness` [1,15] = 5 |
| Texture | `craquelure` / `GalleryCraquelureOptions` | `crackSpacing` [2,100] = 15; `crackDepth` [0,10] = 6; `crackBrightness` [0,10] = 9; `seed` = 0 |
| Texture | `grain` / `GalleryGrainOptions` | `intensity` [0,100] = 40; `contrast` [0,100] = 50; `grainType` (GalleryGrainType): `Regular`, `Soft`, `Sprinkles`, `Clumped`, `Contrasty`, `Enlarged`, `Stippled`, `Horizontal`, `Vertical`, `Speckle`; `foreground` RGB = `{0,0,0}`; `background` RGB = `{1,1,1}`; `seed` = 0 |
| Texture | `mosaicTiles` / `GalleryMosaicTilesOptions` | `tileSize` [2,100] = 12; `groutWidth` [1,15] = 3; `lightenGrout` [0,10] = 9; `seed` = 0 |
| Texture | `patchwork` / `GalleryPatchworkOptions` | `squareSize` [0,10] = 4; `relief` [0,25] = 8; `seed` = 0 |
| Texture | `stainedGlass` / `GalleryStainedGlassOptions` | `cellSize` [2,50] = 10; `borderThickness` [1,20] = 4; `lightIntensity` [0,10] = 3; `foreground` RGB = `{0,0,0}`; `seed` = 0 |
| Texture | `texturizer` / `GalleryTexturizerOptions` | `texture` (GalleryTexture): `Canvas`, `Brick`, `Burlap`, `Sandstone`; `scaling` [50,200] = 100; `relief` [0,50] = 4; `light` (GalleryLightDirection): `Top`, `TopRight`, `Right`, `BottomRight`, `Bottom`, `BottomLeft`, `Left`, `TopLeft`; `invert` = false; `seed` = 0 |

## Implemented mechanisms

- Colored Pencil combines two elongated hatch directions, source luma/edges and tinted paper. Cutout smooths and quantizes luminance-scaled channels. Dry Brush and Fresco use minimum-variance quadrant means, unsharp detail and quantization; Fresco additionally darkens edges.
- Film Grain adds luma-shaped deterministic monochrome grain with optional highlight lifting. Neon Glow creates a foreground/background luma base and screens a blurred positive or inverted mask with the glow color. Paint Daubs selects six real brush behaviors. Palette Knife uses Kuwahara color regions and quantization. Plastic Wrap uses blurred luma normals and a specular half-vector. Poster Edges combines quantization and gradient ink.
- Rough Pastels and Angled Strokes sample the middle of staggered discrete strokes, rather than substituting a generic blur. Rough Pastels applies luminance-weighted procedural relief. Smudge Stick uses diagonal means and highlight lifting. Sponge uses fractal holes. Underpainting mixes softened colors and saturation with coverage-scaled texture relief. Watercolor darkens edges and applies texture-dependent pigment noise.
- Accented Edges brightens or darkens blurred gradients. Crosshatch blends opposite diagonal stroke means and sharpening. Dark Strokes combines dark/light directional means. Ink Outlines combines source gradients and directional paint. Spatter and Sprayed Strokes use value-noise displacement, with an extra directional mean for Sprayed Strokes. Sumi-e combines blurred stroke ink and contrast.
- Diffuse Glow selects luma highlights, sparse grain events and background-color glow. Glass uses a selected procedural texture gradient for displacement. Ocean Ripple uses two smooth value-noise displacement fields.
- Sketch filters each implement their own threshold, tonal, noise, line, embossing or wave mechanism. Bas Relief, Note Paper and Plaster derive directional shading from luma/mask gradients. Chalk & Charcoal and Conté Crayon combine independent ink/paper coverage. Charcoal uses pressure-like strokes and edges. Chrome uses periodic grayscale tonal waves. Graphic Pen thresholds directional noise. Halftone Pattern supports Dot, Circle and Line geometry. Photocopy uses luma high-pass ink. Reticulation combines fine noise and ink/paper levels. Stamp thresholds smoothed luma. Torn Edges perturbs a threshold with fractal texture. Water Paper combines crossing fibers and directional color means.
- Glowing Edges computes separate RGB gradient magnitudes and spreads them before brightness scaling. Craquelure uses the separation of nearest and second-nearest jittered Voronoi points for cracks and shading. Grain implements all ten named distributions. Mosaic Tiles uses jittered grout geometry and bevels. Patchwork samples square centers and shades independent cell heights. Stained Glass samples jittered cell colors with borders and centered light. Texturizer supplies canvas, brick, burlap and sandstone relief under eight light directions.

## Kernel behavior and parameter interactions

The gallery Gaussian approximation is the reference's three variance-matched box convolutions, including its smallest nonzero blur fallback below sigma about 0.577. Three horizontal passes precede three vertical passes. Running sums make box and directional means linear in the image size; Kuwahara also uses precomputed box moments instead of radius-squared scans. Derivatives use clamped image edges.

Some controls intentionally affect conditional mechanisms, as in the reference: Film Grain and Smudge Stick's Intensity controls highlight lifting and is inactive when Highlight Area is zero; Paint Daubs' Wide Blurry brush uses smoothing instead of its Sharpness mechanism; foreground/background colors in Grain matter for the named ink/paper distributions. Rough Pastels' Stroke Detail acts only when Stroke Length is at least one. Relief zero disables the texture and its Scaling/Light/Invert controls. The validation contract still checks every declared field when its mechanism is inactive.

## Verification

`tests/gallery_filters.cpp` supports 47 function-name selections, `oracles`, `stack`, or `all`. Every effect checks default pixel change, normalized finite output, exact alpha and hidden-RGB retention, deterministic repeated execution, hidden-RGB isolation, thin and fully transparent images, invalid input, each slider's invalid bounds/NaN/Inf, enum/color rejection, and minimum/maximum numeric settings. Each numeric slider is exercised at both endpoints with its mechanism enabled; procedural seeds are checked in active noise configurations.

Independent one-pixel oracles cover Cutout's quantization, Film Grain's highlight blend, Neon Glow's color base, Chrome's sine-wave tonal mapping and Stamp's threshold. Zero-strength displacement/relief tests require exact identity. Stack tests compare sequential results, show observable ordering, check empty detached copies, invalid-effect failure and the 64-effect limit. These establish SDK contracts and formulas; they do not establish Adobe visual parity, document-coordinate tile equivalence or other-platform runtime proof.
