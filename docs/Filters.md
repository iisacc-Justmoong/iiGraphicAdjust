# Filter inventory and processing contract — 0.4.0

Both supplied repositories were re-fetched and inspected on 2026-10-08. PhotoCraft is pinned to [`5896f0b`](https://github.com/storytold/photocraft/tree/5896f0b63e6f342e0311920b4d2a0ea5a94330fc); LightCraft is pinned to [`629e3938`](https://github.com/storytold/lightcraft/tree/629e39380e296f588c64cd9c0053a8edc3528f36). The machine-readable [filter catalog](filter-catalog.json) preserves every registered reference command, its original parameter description, source location and corresponding SDK function.

## Inventory rules

- PhotoCraft: 120 distinct executable Filter-menu operations, including 47 Filter Gallery effects. Two executable Edit operations, Puppet Warp and Perspective Warp, are also recordable as smart filters. Their Smart Object command aliases are mapped to the same implementations and are not additional algorithms.
- `FilterParams` contains 59 variants: 58 pixel operations and the composite Gallery container. Invert and Desaturate are already implemented in 0.2 and remain available; these two enum operations are outside the Filter-menu count. The enum, extended dispatch, procedural render commands, lens/warp commands and all 47 artistic definitions were inspected, not just strings containing “filter”.
- LightCraft: 22 look entries (21 nonidentity looks plus identity Color), 41 built-in presets and four resampling kernels. Each look and preset has a named function; their full numeric recipes and all four kernels have independent fixtures. Existing 0.3 development/detail/local-mask kernels remain available. `guidedBlur` and `crossGuidedBlur` additionally expose reusable, box-window edge-preserving kernels.
- The Gallery container is available as `filterGallery(ImageView, span<const GalleryEffect>)`, a bounded, ordered, typed stack of up to 64 effects. It does not replace the 47 individual APIs.

Last Filter, Convert for Smart Filters, plugin hosting, document selection/layer creation, clipboard access and history replay belong to an application host. Their existing executable pixel operations are included; command orchestration is not a new pixel algorithm. LightCraft `engine/src/cmd/filters.rs` filters a catalog by metadata/search conditions rather than filtering image pixels. Neural filters, Lens Blur/Enhance declarations without an executable implementation and unsupported imported effects are not executable reference filters. RAW demosaicing, HDR/panorama merging, camera profile databases, CMS and GUI texture upload remain separate processing systems.

## Shared contract and intentional adaptations

Inputs and outputs use the existing normalized, straight-alpha encoded-sRGB `ImageView`/`Image` contract. Every operation validates finite normalized pixels, dimensions and all options, returns detached storage through `Result<T>`, and does not write documents or mutate input. PhotoCraft geometry uses premultiplied encoded-sRGB interpolation so invisible RGB does not enter visible output. The coverage-preserving color kernels and Gallery operations retain source alpha and hidden RGB; spatial remapping transports coverage, while newly uncovered resampling output is canonical transparent black (morphology retains original hidden RGB at eroded pixels as documented). A validated neutral operation copies the original storage values exactly where its contract defines an identity.

The new `filterGaussianBlur`, `filterSharpen` and `filterAddNoise` intentionally implement the menu's encoded-space/percentage contracts. Existing `gaussianBlur`, `sharpen` and `addNoise` retain their 0.1 linear-light and normalized-amount semantics. Large Gaussian windows use three variance-matched box passes; small sigma uses explicit separable Gaussian weights. This is approximate filtering, not bitwise PhotoCraft or Adobe parity.

Algorithms and procedural models are ported from the reference but adapted to this SDK's alpha, allocation and finite-input contract. Median neighborhoods use alpha-weighted visible samples; minimum/maximum ignore hidden RGB and can transport alpha; custom convolution replaces invisible neighbor color with the visible center. The fixed image canvas clips procedural and mesh output instead of expanding an application layer surface. Shape Blur clamps borders. Lens Blur uses bounded kernel/depth levels, blur-gallery operations use bounded samples/levels, Wave permits the 64 generators actually used by the reference CPU path. Unsupported host-only fields do not silently become pixel effects. See the module documents for individual constraints and differences.

`develop`-based look/preset output remains rendered SDR: intermediate stages normalize to sRGB. PhotoCraft Camera Raw has a separate relative-temperature pipeline in `CameraRawFilter`, rather than silently converting its relative temperature slider to the existing Kelvin `whiteBalance`. This SDK does not decode camera RAW files or claim Adobe renderer equivalence.

## PhotoCraft command mapping

| Reference command | SDK function | Public header | Pinned source |
| --- | --- | --- | --- |
| `edit.perspectiveWarp` | `perspectiveWarp` | `MeshFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/distort_cmds.rs#L402) |
| `edit.puppetWarp` | `puppetWarp` | `MeshFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/distort_cmds.rs#L382) |
| `filter.adaptiveWideAngle` | `adaptiveWideAngle` | `MeshFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/lens_cmds.rs#L401) |
| `filter.blur.average` | `average` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/extra_cmds.rs#L742) |
| `filter.blur.blur` | `blur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L276) |
| `filter.blur.blurMore` | `blurMore` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L277) |
| `filter.blur.boxBlur` | `boxBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L282) |
| `filter.blur.gaussianBlur` | `filterGaussianBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L275) |
| `filter.blur.lensBlur` | `lensBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L626) |
| `filter.blur.motionBlur` | `motionBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L283) |
| `filter.blur.radialBlur` | `radialBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L284) |
| `filter.blur.shapeBlur` | `shapeBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L632) |
| `filter.blur.smartBlur` | `smartBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L620) |
| `filter.blur.surfaceBlur` | `surfaceBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L290) |
| `filter.blurGallery.fieldBlur` | `fieldBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L651) |
| `filter.blurGallery.irisBlur` | `irisBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L645) |
| `filter.blurGallery.pathBlur` | `pathBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L663) |
| `filter.blurGallery.spinBlur` | `spinBlur` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L657) |
| `filter.blurGallery.tiltShift` | `tiltShift` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L639) |
| `filter.cameraRaw` | `cameraRawFilter` | `CameraRawFilter.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/lens_cmds.rs#L411) |
| `filter.distort.displace` | `displace` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L570) |
| `filter.distort.pinch` | `pinch` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L316) |
| `filter.distort.polarCoordinates` | `polarCoordinates` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L325) |
| `filter.distort.ripple` | `ripple` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L324) |
| `filter.distort.shear` | `shear` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L576) |
| `filter.distort.spherize` | `spherize` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L317) |
| `filter.distort.twirl` | `twirl` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L315) |
| `filter.distort.wave` | `wave` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L318) |
| `filter.distort.zigZag` | `zigZag` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L577) |
| `filter.gallery.accentedEdges` | `accentedEdges` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L56) |
| `filter.gallery.angledStrokes` | `angledStrokes` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L57) |
| `filter.gallery.basRelief` | `basRelief` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L69) |
| `filter.gallery.chalkCharcoal` | `chalkCharcoal` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L70) |
| `filter.gallery.charcoal` | `charcoal` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L71) |
| `filter.gallery.chrome` | `chrome` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L72) |
| `filter.gallery.coloredPencil` | `coloredPencil` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L40) |
| `filter.gallery.conteCrayon` | `conteCrayon` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L73) |
| `filter.gallery.craquelure` | `craquelure` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L86) |
| `filter.gallery.crosshatch` | `crosshatch` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L58) |
| `filter.gallery.cutout` | `cutout` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L41) |
| `filter.gallery.darkStrokes` | `darkStrokes` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L59) |
| `filter.gallery.diffuseGlow` | `diffuseGlow` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L65) |
| `filter.gallery.dryBrush` | `dryBrush` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L42) |
| `filter.gallery.filmGrain` | `galleryFilmGrain` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L43) |
| `filter.gallery.fresco` | `fresco` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L44) |
| `filter.gallery.glass` | `glass` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L66) |
| `filter.gallery.glowingEdges` | `glowingEdges` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L84) |
| `filter.gallery.grain` | `grain` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L87) |
| `filter.gallery.graphicPen` | `graphicPen` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L74) |
| `filter.gallery.halftonePattern` | `halftonePattern` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L75) |
| `filter.gallery.inkOutlines` | `inkOutlines` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L60) |
| `filter.gallery.mosaicTiles` | `mosaicTiles` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L88) |
| `filter.gallery.neonGlow` | `neonGlow` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L45) |
| `filter.gallery.notePaper` | `notePaper` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L76) |
| `filter.gallery.oceanRipple` | `oceanRipple` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L67) |
| `filter.gallery.paintDaubs` | `paintDaubs` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L46) |
| `filter.gallery.paletteKnife` | `paletteKnife` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L47) |
| `filter.gallery.patchwork` | `patchwork` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L89) |
| `filter.gallery.photocopy` | `photocopy` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L77) |
| `filter.gallery.plaster` | `plaster` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L78) |
| `filter.gallery.plasticWrap` | `plasticWrap` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L48) |
| `filter.gallery.posterEdges` | `posterEdges` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L49) |
| `filter.gallery.reticulation` | `reticulation` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L79) |
| `filter.gallery.roughPastels` | `roughPastels` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L50) |
| `filter.gallery.smudgeStick` | `smudgeStick` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L51) |
| `filter.gallery.spatter` | `spatter` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L61) |
| `filter.gallery.sponge` | `sponge` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L52) |
| `filter.gallery.sprayedStrokes` | `sprayedStrokes` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L62) |
| `filter.gallery.stainedGlass` | `stainedGlass` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L90) |
| `filter.gallery.stamp` | `stamp` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L80) |
| `filter.gallery.sumiE` | `sumiE` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L63) |
| `filter.gallery.texturizer` | `texturizer` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L91) |
| `filter.gallery.tornEdges` | `tornEdges` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L81) |
| `filter.gallery.underpainting` | `underpainting` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L53) |
| `filter.gallery.waterPaper` | `waterPaper` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L82) |
| `filter.gallery.watercolor` | `watercolor` | `GalleryFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/artistic.rs#L54) |
| `filter.lensCorrection` | `lensCorrection` | `LensFilter.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/lens_cmds.rs#L391) |
| `filter.liquify` | `liquify` | `MeshFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/distort_cmds.rs#L372) |
| `filter.noise.addNoise` | `filterAddNoise` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L303) |
| `filter.noise.despeckle` | `despeckle` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L281) |
| `filter.noise.dustAndScratches` | `dustAndScratches` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L310) |
| `filter.noise.median` | `median` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L309) |
| `filter.noise.reduceNoise` | `reduceNoise` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L613) |
| `filter.other.custom` | `customFilter` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L670) |
| `filter.other.highPass` | `highPass` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L326) |
| `filter.other.hsbHsl` | `hsbHsl` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L671) |
| `filter.other.maximum` | `maximum` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L328) |
| `filter.other.minimum` | `minimum` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L327) |
| `filter.other.offset` | `offset` | `DistortionFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L329) |
| `filter.pixelate.colorHalftone` | `colorHalftone` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L531) |
| `filter.pixelate.crystallize` | `crystallize` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L537) |
| `filter.pixelate.facet` | `facet` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L538) |
| `filter.pixelate.fragment` | `fragment` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L539) |
| `filter.pixelate.mezzotint` | `mezzotint` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L540) |
| `filter.pixelate.mosaic` | `mosaic` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L311) |
| `filter.pixelate.pointillize` | `pointillize` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L546) |
| `filter.render.clouds` | `clouds` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/extra_cmds.rs#L743) |
| `filter.render.differenceClouds` | `differenceClouds` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/extra_cmds.rs#L744) |
| `filter.render.fibers` | `fibers` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L584) |
| `filter.render.flame` | `flame` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/render_cmds.rs#L187) |
| `filter.render.lensFlare` | `lensFlare` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L590) |
| `filter.render.lightingEffects` | `lightingEffects` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L596) |
| `filter.render.pictureFrame` | `pictureFrame` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/render_cmds.rs#L197) |
| `filter.render.relight` | `relight` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L603) |
| `filter.render.tree` | `tree` | `RenderFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/render_cmds.rs#L207) |
| `filter.sharpen.sharpen` | `filterSharpen` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L278) |
| `filter.sharpen.sharpenEdges` | `sharpenEdges` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L280) |
| `filter.sharpen.sharpenMore` | `sharpenMore` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L279) |
| `filter.sharpen.smartSharpen` | `smartSharpen` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L297) |
| `filter.sharpen.unsharpMask` | `unsharpMask` | `BlurFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L291) |
| `filter.stylize.diffuse` | `diffuse` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L548) |
| `filter.stylize.emboss` | `emboss` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L312) |
| `filter.stylize.extrude` | `extrude` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L549) |
| `filter.stylize.findEdges` | `findEdges` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L313) |
| `filter.stylize.oilPaint` | `oilPaint` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L555) |
| `filter.stylize.solarize` | `solarize` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters.rs#L314) |
| `filter.stylize.tiles` | `tiles` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L561) |
| `filter.stylize.traceContour` | `traceContour` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L567) |
| `filter.stylize.wind` | `wind` | `PixelFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L568) |
| `filter.vanishingPoint` | `vanishingPoint` | `MeshFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/vp_cmds.rs#L199) |
| `filter.video.deInterlace` | `deInterlace` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L673) |
| `filter.video.ntscColors` | `ntscColors` | `UtilityFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/filters_ext.rs#L679) |
| `layer.smartObjects.perspectiveWarp` | `perspectiveWarp` | `MeshFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/distort_cmds.rs#L412) |
| `layer.smartObjects.puppetWarp` | `puppetWarp` | `MeshFilters.h` | [source](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/distort_cmds.rs#L392) |

## Additional LightCraft retouch and diagnostics

| Function | Executable source path | Contract |
| --- | --- | --- |
| `cloneSpot`, `healSpot`, `removeSpot`, `rankSpotSources` | [pipeline/spots.rs](https://github.com/storytold/lightcraft/blob/629e39380e296f588c64cd9c0053a8edc3528f36/crates/pipeline/src/spots.rs) | Patch transport, tone matching/inpainting and ranked source selection |
| `redEye`, `petEye`, `detectPupil` | [pipeline/redeye.rs](https://github.com/storytold/lightcraft/blob/629e39380e296f588c64cd9c0053a8edc3528f36/crates/pipeline/src/redeye.rs) | Actual pupil detection and eye-pixel repair |
| `visualizeSpots` | [pipeline/visualize.rs](https://github.com/storytold/lightcraft/blob/629e39380e296f588c64cd9c0053a8edc3528f36/crates/pipeline/src/visualize.rs) | Diagnostic rendered image for spot inspection |
| `detectDustSpots` | [pipeline/dust.rs](https://github.com/storytold/lightcraft/blob/629e39380e296f588c64cd9c0053a8edc3528f36/crates/pipeline/src/dust.rs) | Returns detected candidate positions/radii, not an invented image-look filter |

The additional retouch set has six image-processing functions and three analysis/selection functions. Its options and numerical adaptations are in [RetouchFilters.md](RetouchFilters.md).

## Additional display/selection diagnostics

`DiagnosticFilters.h` exposes `pointColorRangePreview`, five individually named mask composites (`maskColorOverlay`, `maskColorOnBwOverlay`, `maskImageOnBlack`, `maskImageOnWhite`, `maskWhiteOnBlack`) and `destinationGamutWarning`/`displayGamutWarning`. These port real image branches in LightCraft `pipeline/visualize.rs`, `pipeline/finish.rs` and the output module. The caller supplies the selection/mask, optional pre-mapping scene-linear colors and a named proof color space; the SDK does not retain GUI state or guess an ICC display profile. [DiagnosticFilters.md](DiagnosticFilters.md) specifies formulas and limits. Histogram-based highlight/shadow clipping indicators are GUI border decorations with no separate spatial pixel filter and are excluded.

## Module contracts

[Blur/distortion/utility contracts](CoreFilters.md), [Gallery effects](GalleryFilters.md), [Pixelate/Stylize](PixelFilters.md), [procedural/light rendering](RenderFilters.md), [Lens Correction](LensFilter.md), [Camera Raw](CameraRawFilter.md), [mesh/perspective filters](MeshFilters.md), and [LightCraft looks/presets/resampling](LightcraftFilters.md).

The inherited MIT licenses for the two references are distributed in `third_party/`. No Rust runtime, Qt, GUI framework or third-party processing dependency is linked. Final configuration, sanitizer and installed-consumer evidence is recorded in [Verification.md](Verification.md).
