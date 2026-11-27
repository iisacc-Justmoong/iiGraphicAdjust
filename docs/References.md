# Photoshop adjustment reference and implementation scope

Inspected on 2026-10-08 (Asia/Seoul). The user supplied [storytold/photocraft](https://github.com/storytold/photocraft) as a Rust Photoshop clone reference. The inspection is pinned to commit `5896f0b63e6f342e0311920b4d2a0ea5a94330fc`. The original 0.2 adjustment inspection did not run the upstream Rust project or Photoshop renderer. Version 0.4 includes a separately extracted Camera Raw Rust-kernel oracle; its limited scope is documented in [CameraRawFilter.md](CameraRawFilter.md). No full upstream test suite or Adobe render corpus was run.

Adobe's [adjustment-layer list](https://helpx.adobe.com/ca/photoshop/desktop/create-manage-layers/color-adjustment-fill-layers/work-with-adjustment-and-fill-layers.html) and [color adjustment command reference](https://helpx.adobe.com/photoshop/desktop/adjust-color/color-corrections/color-adjustment-commands.html) were checked to establish the feature categories. These references describe behavior and purpose, not a public specification of Photoshop's exact processing algorithms.

## Operation mapping

PhotoCraft registers 16 layer adjustment kinds and 6 additional Image > Adjustments commands. The SDK provides the following corresponding operation categories, plus the three automatic Image-menu commands.

- Reference command: Brightness/Contrast; SDK API: `brightnessContrast`; Contract documentation: Tone.md
- Reference command: Levels; SDK API: `levels`; Contract documentation: Tone.md
- Reference command: Curves; SDK API: `curves`; Contract documentation: Tone.md
- Reference command: Exposure; SDK API: `exposure`; Contract documentation: Tone.md
- Reference command: Vibrance; SDK API: `vibrance`; Contract documentation: Color.md
- Reference command: Hue/Saturation; SDK API: `hueSaturation`; Contract documentation: Color.md
- Reference command: Color Balance; SDK API: `colorBalance`; Contract documentation: Color.md
- Reference command: Black & White; SDK API: `blackAndWhite`; Contract documentation: Color.md
- Reference command: Photo Filter; SDK API: `photoFilter`; Contract documentation: Color.md
- Reference command: Channel Mixer; SDK API: `channelMixer`; Contract documentation: Color.md
- Reference command: Invert; SDK API: `invert`; Contract documentation: Tone.md
- Reference command: Posterize; SDK API: `posterize`; Contract documentation: Tone.md
- Reference command: Threshold; SDK API: `threshold`; Contract documentation: Tone.md
- Reference command: Gradient Map; SDK API: `gradientMap`; Contract documentation: Mapping.md
- Reference command: Selective Color; SDK API: `selectiveColor`; Contract documentation: Color.md
- Reference command: Color Lookup; SDK API: `colorLookup`, `parseCubeLut`; Contract documentation: Mapping.md
- Reference command: Desaturate; SDK API: `desaturate`; Contract documentation: Color.md
- Reference command: Equalize; SDK API: `equalize`; Contract documentation: Tone.md
- Reference command: Shadows/Highlights; SDK API: `shadowsHighlights`; Contract documentation: Tone.md
- Reference command: Replace Color; SDK API: `replaceColor`; Contract documentation: Color.md
- Reference command: Match Color; SDK API: `matchColor`; Contract documentation: Color.md
- Reference command: HDR Toning; SDK API: `hdrToning` (ExposureGamma, HighlightCompression); Contract documentation: Mapping.md
- Reference command: Auto Tone / Auto Levels; SDK API: `autoLevels`; Contract documentation: Tone.md
- Reference command: Auto Contrast; SDK API: `autoContrast`; Contract documentation: Tone.md
- Reference command: Auto Color; SDK API: `autoColor`; Contract documentation: Tone.md

This is category coverage, not all options or exact Photoshop pixel parity. Curves uses documented linear segments instead of Photoshop's spline. Levels/Posterize do not reproduce document-depth quantization. Shadow/Highlights uses tonal masks without spatial adaptation. Match Color uses alpha-weighted linear RGB statistics rather than Photoshop's proprietary matching. HDR Toning has two global methods, not Local Adaptation or HDR histogram equalization. Color Lookup supports numeric tables and the documented `.cube` subset, not ICC or the reference application's other LUT file formats. RGB-derived Selective Color weights are not a CMYK print-separation engine.

## Source inspection and attribution

Relevant pinned upstream files:

- [compose/src/adjust.rs](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/compose/src/adjust.rs): adjustment dispatch, levels/curves, hue ranges, blending-independent color math, gradient map and LUT interpolation.
- [algo/src/tone.rs](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/algo/src/tone.rs): shadows/highlights, replace/match color and local HDR toning.
- [cms/src/lutfile.rs](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/cms/src/lutfile.rs): LUT row ordering and format conventions.
- [engine/src/edit_cmds.rs](https://github.com/storytold/photocraft/blob/5896f0b63e6f342e0311920b4d2a0ea5a94330fc/crates/engine/src/edit_cmds.rs): automatic corrections.

The C++ implementation was written for iiGraphicAdjust's existing detached/normalized pixel contract. No Rust runtime, GUI, branding or other upstream dependencies are incorporated. The reference itself calls its processing formulas approximations; it is not a guarantee of Photoshop behavior. The upstream MIT license is preserved in `third_party/photocraft/LICENSE-MIT` and installed with this package as reference attribution. This does not change the license of unrelated SDK code or add a runtime dependency.

## Current Adobe features beyond this reference

Version 0.3 adds separate Lightroom-style Light/Color/Clarity/Dehaze/Grain operations, selected from [LightCraft source](https://github.com/storytold/lightcraft/tree/629e39380e296f588c64cd9c0053a8edc3528f36). See [Lightroom development coverage](Lightroom.md). This adds processing APIs and does not prove current Photoshop adjustment-layer or Adobe pixel parity.

Current Adobe documentation also describes newer [Light](https://helpx.adobe.com/photoshop/desktop/create-manage-layers/color-adjustment-fill-layers/adjust-image-lighting-with-light.html), [Color and vibrance](https://helpx.adobe.com/photoshop/desktop/create-manage-layers/color-adjustment-fill-layers/correct-color-balance-with-color-and-vibrance.html), [Clarity and Dehaze](https://helpx.adobe.com/photoshop/desktop/create-manage-layers/color-adjustment-fill-layers/adjust-contrast-with-clarity-and-dehaze.html), and [Grain](https://helpx.adobe.com/photoshop/desktop/create-manage-layers/color-adjustment-fill-layers/enhance-texture-with-grain.html) layers beyond PhotoCraft's adjustment enum. iiGraphicAdjust 0.2.0 does not claim complete current Photoshop layer/dialog coverage. Existing exposure, contrast, tonal masks, temperature/tint, vibrance/saturation and noise can be composed by a host, but do not reproduce those newer layer definitions.

Version 0.4 re-inspects both repositories and implements their executable filtering paths. The complete named API/command mapping is in [Filters.md](Filters.md), including smart-filter aliases, looks/presets, retouch, resampling and diagnostics.
