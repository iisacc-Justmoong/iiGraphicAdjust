# LightCraft looks, presets and resampling filters

`<iiGraphicAdjust/LightcraftFilters.h>` adds individually callable LightCraft looks and builtin preset recipes, plus four raster resampling filters. All interfaces use C++23, `ImageView`, `Result<T>` and the existing Qt-free Develop implementation. Input images are borrowed; every image result owns its storage.

The reference is [LightCraft commit 629e39380e296f588c64cd9c0053a8edc3528f36](https://github.com/storytold/lightcraft/tree/629e39380e296f588c64cd9c0053a8edc3528f36). Its MIT notice is preserved in `third_party/lightcraft/LICENSE-MIT`. The recipes are LightCraft's own looks and builtin presets; their names do not claim Adobe rendering equivalence.

## Recipe interfaces and execution

Every named look or preset function has the signature `Result<Image> function(ImageView image, double amount = 1)`. Amount is finite in `[0,2]`, corresponding to the reference's 0–200 percent control. Zero produces an exact detached copy, including hidden RGB in transparent pixels. All amounts validate the image and selection. Unknown enum values return `InvalidParameter`, including at zero amount.

`lightcraftLookOptions(LightcraftLook, amount)` and `lightcraftPresetOptions(LightcraftPreset, amount)` return explicit `DevelopOptions` recipes. `applyLightcraftLook(ImageView, LightcraftLook, amount)` and `applyLightcraftPreset(ImageView, LightcraftPreset, amount)` select the same recipes. The named functions listed below are independently exported free functions; no string dispatcher or opaque style object is required. A host can retrieve a recipe, inspect or change its fields, then call `develop` or the individual Develop stages.

Each recipe starts from default Develop settings. The look formulas below are deltas from these defaults, not a merge into existing user settings. Grading wheel hues stay fixed while look saturation and luminance scale with amount. A point-curve fade uses `b = clamp(black*amount,0,.45)`, `w = clamp(1-(1-white)*amount,.55,1)`, then `y = b + y*(w-b)`. B&W look treatment is active at every positive amount; zero remains identity. No look adds grain.

Preset numeric fields interpolate from their default value: `default + (preset-default)*amount`, followed by each control's bounds. Preset B&W treatment and custom-WB mode switch at amount `.5`, matching the reference's nonnumeric patch rule. Below that threshold the selected WB remains as-shot, represented by the SDK's default 6500 K temperature and zero tint. Grading hue itself interpolates as a numeric preset field, unlike a profile wheel's fixed hue. Grain size/roughness interpolate from 25/50; vignette midpoint interpolates from 50. Fields omitted by a recipe retain their Develop defaults.

Recipes run in the SDK's explicit order: optics → white balance → detail → Texture/Clarity/Dehaze → calibration → Light → Color/B&W/grading → post-crop vignette → curve → grain. Each stage maps its result to rendered SDR sRGB. LightCraft's source pipeline keeps scene-linear Rec.2020 data between stages, applies DNG profiles before these looks, and supports wider-range data. This module preserves the recipe values and reuses the SDK's defined SDR algorithms; a rendered sRGB input cannot reproduce RAW recovery, DNG camera profiles, continuous HDR processing or exact upstream output. In particular, stage clipping and the chosen curve/grade order can alter saturated or extreme pixels.

```cpp
#include <iiGraphicAdjust/LightcraftFilters.h>
using namespace iiGraphicAdjust;
auto warm = lightcraftLookFilmWarmPrint(source.view(), .75);
auto food = lightcraftPresetFreshBright(source.view());
auto recipe = lightcraftPresetOptions(LightcraftPreset::GoldenHour);
if (recipe) {
    recipe->grain.seed = 42;
    auto result = develop(source.view(), *recipe);
}
auto thumbnail = resizeLanczos3(source.view(), 320, 240);
```

## All individually exported look functions

The 21 active look recipes and identity Color come from [`pipeline/src/profiles.rs`](https://github.com/storytold/lightcraft/blob/629e39380e296f588c64cd9c0053a8edc3528f36/crates/pipeline/src/profiles.rs#L45). Listed numbers are the values at amount 1. `grading` entries are `(hue, saturation, luminance)`; `fade` entries are `(black output, white output)`.

- Function: `lightcraftLookColor`; Reference profile ID: `lc.color`; Recipe: Identity
- Function: `lightcraftLookNeutral`; Reference profile ID: `lc.neutral`; Recipe: light.contrast=-18.0, color.saturation=-10.0, light.highlights=-10.0, light.shadows=8.0
- Function: `lightcraftLookVivid`; Reference profile ID: `lc.vivid`; Recipe: light.contrast=16.0, color.saturation=14.0, color.vibrance=14.0, light.blacks=-6.0
- Function: `lightcraftLookLandscape`; Reference profile ID: `lc.landscape`; Recipe: light.contrast=10.0, color.vibrance=10.0, mixer.green.sat=14.0, mixer.aqua.sat=10.0, mixer.blue.sat=14.0, mixer.blue.lum=-6.0, effects.clarity=6.0
- Function: `lightcraftLookPortrait`; Reference profile ID: `lc.portrait`; Recipe: light.contrast=-6.0, mixer.orange.sat=-8.0, mixer.orange.lum=6.0, mixer.red.sat=-5.0, effects.texture=-6.0
- Function: `lightcraftLookMono`; Reference profile ID: `lc.mono`; Recipe: B&W, light.contrast=8.0
- Function: `lightcraftLookFilmWarmPrint`; Reference profile ID: `lc.film.warm-print`; Recipe: light.contrast=6.0, color.saturation=-4.0, mixer.orange.lum=4.0, fade curve.master=(0.04,0.97), grading.highlights=(40.0,18.0,0.0), grading.shadows=(25.0,8.0,0.0)
- Function: `lightcraftLookFilmCoolFade`; Reference profile ID: `lc.film.cool-fade`; Recipe: light.contrast=-8.0, color.saturation=-10.0, fade curve.master=(0.07,0.95), grading.shadows=(210.0,16.0,0.0), grading.highlights=(190.0,6.0,0.0)
- Function: `lightcraftLookFilmGoldenHour`; Reference profile ID: `lc.film.golden-hour`; Recipe: color.vibrance=10.0, curve.highlights=-8.0, mixer.orange.sat=8.0, mixer.yellow.hue=-6.0, grading.global=(45.0,10.0,0.0), grading.highlights=(35.0,20.0,0.0)
- Function: `lightcraftLookFilmFadedSlide`; Reference profile ID: `lc.film.faded-slide`; Recipe: light.contrast=14.0, color.vibrance=8.0, color.saturation=6.0, curve.shadows=10.0, fade curve.blue=(0.06,1.0), fade curve.red=(0.0,0.97)
- Function: `lightcraftLookCineTealAmber`; Reference profile ID: `lc.cine.teal-amber`; Recipe: light.contrast=10.0, mixer.orange.sat=6.0, mixer.green.hue=20.0, mixer.green.sat=-20.0, mixer.blue.hue=-8.0, grading.shadows=(195.0,30.0,0.0), grading.highlights=(38.0,24.0,0.0)
- Function: `lightcraftLookCineNightBlue`; Reference profile ID: `lc.cine.night-blue`; Recipe: light.contrast=12.0, color.saturation=-12.0, curve.darks=-8.0, grading.shadows=(220.0,28.0,-4.0), grading.midtones=(215.0,10.0,0.0)
- Function: `lightcraftLookCineDesertHeat`; Reference profile ID: `lc.cine.desert-heat`; Recipe: light.contrast=8.0, curve.lights=6.0, mixer.blue.sat=-25.0, mixer.aqua.sat=-20.0, grading.global=(32.0,16.0,0.0), grading.highlights=(48.0,12.0,0.0)
- Function: `lightcraftLookCineNeonDusk`; Reference profile ID: `lc.cine.neon-dusk`; Recipe: light.contrast=10.0, color.vibrance=18.0, mixer.magenta.sat=12.0, mixer.purple.sat=12.0, grading.shadows=(265.0,22.0,0.0), grading.highlights=(330.0,14.0,0.0)
- Function: `lightcraftLookMutedMatteSoft`; Reference profile ID: `lc.muted.matte-soft`; Recipe: light.contrast=-12.0, color.saturation=-14.0, curve.highlights=-6.0, fade curve.master=(0.08,0.96)
- Function: `lightcraftLookMutedBleached`; Reference profile ID: `lc.muted.bleached`; Recipe: light.contrast=22.0, color.saturation=-38.0, curve.lights=6.0, effects.clarity=8.0
- Function: `lightcraftLookMutedPastelHaze`; Reference profile ID: `lc.muted.pastel-haze`; Recipe: light.contrast=-18.0, color.saturation=-20.0, color.vibrance=12.0, fade curve.master=(0.1,1.0), grading.highlights=(330.0,6.0,0.0), grading.shadows=(200.0,6.0,0.0)
- Function: `lightcraftLookMutedQuietGreen`; Reference profile ID: `lc.muted.quiet-green`; Recipe: light.contrast=-4.0, color.saturation=-8.0, mixer.green.sat=-30.0, mixer.green.hue=-10.0, mixer.yellow.sat=-18.0, mixer.aqua.sat=-15.0, fade curve.master=(0.04,1.0)
- Function: `lightcraftLookBwMonoRich`; Reference profile ID: `lc.bw.mono-rich`; Recipe: B&W, light.contrast=18.0, curve.darks=-8.0, curve.lights=6.0, bw_mix.red=8.0, bw_mix.orange=10.0, bw_mix.blue=-12.0
- Function: `lightcraftLookBwRedFilter`; Reference profile ID: `lc.bw.red-filter`; Recipe: B&W, light.contrast=12.0, bw_mix.red=40.0, bw_mix.orange=30.0, bw_mix.yellow=10.0, bw_mix.green=-20.0, bw_mix.aqua=-30.0, bw_mix.blue=-45.0
- Function: `lightcraftLookBwSoft`; Reference profile ID: `lc.bw.soft`; Recipe: B&W, light.contrast=-14.0, bw_mix.orange=12.0, fade curve.master=(0.06,0.97)
- Function: `lightcraftLookBwSepia`; Reference profile ID: `lc.bw.sepia`; Recipe: B&W, light.contrast=6.0, fade curve.master=(0.04,0.98), grading.shadows=(28.0,18.0,0.0), grading.highlights=(45.0,14.0,0.0)

## All individually exported preset functions

All 41 builtin recipes come from [`engine/src/presets.rs::builtin`](https://github.com/storytold/lightcraft/blob/629e39380e296f588c64cd9c0053a8edc3528f36/crates/engine/src/presets.rs#L143). Each row exposes its own image function. The numeric fixture test covers every provided recipe field, amount interpolation and named-function execution. The JSON below is the upstream recipe at amount 1; `wb.mode` and `treatment` follow the threshold rule above.

- Function: `lightcraftPresetWarmGlow`; Reference ID and group: `lc.warm-glow` (Color); Recipe: `{"wb":{"mode":"custom","temp":7200.0,"tint":8.0},"light":{"contrast":8.0,"shadows":12.0},"color":{"vibrance":18.0}}`
- Function: `lightcraftPresetCoolMorning`; Reference ID and group: `lc.cool-morning` (Color); Recipe: `{"wb":{"mode":"custom","temp":5600.0,"tint":-4.0},"light":{"highlights":-20.0},"color":{"vibrance":10.0},"grading":{"shadows":{"hue":215.0,"sat":12.0,"lum":0.0}}}`
- Function: `lightcraftPresetTealOrange`; Reference ID and group: `lc.teal-orange` (Color); Recipe: `{"grading":{"shadows":{"hue":195.0,"sat":28.0,"lum":0.0},"highlights":{"hue":35.0,"sat":22.0,"lum":0.0},"balance":10.0},"color":{"vibrance":12.0},"light":{"contrast":14.0}}`
- Function: `lightcraftPresetVividPop`; Reference ID and group: `lc.vivid-pop` (Color); Recipe: `{"color":{"vibrance":40.0,"saturation":8.0},"light":{"contrast":18.0,"whites":10.0,"blacks":-10.0},"effects":{"clarity":12.0}}`
- Function: `lightcraftPresetSoftPastel`; Reference ID and group: `lc.soft-pastel` (Color); Recipe: `{"light":{"contrast":-25.0,"highlights":-30.0,"shadows":35.0,"blacks":20.0},"color":{"saturation":-18.0,"vibrance":10.0},"curve":{"shadows":25.0}}`
- Function: `lightcraftPresetFadedMatte`; Reference ID and group: `lc.faded-matte` (Film); Recipe: `{"curve":{"shadows":40.0,"highlights":-15.0},"light":{"contrast":-10.0},"color":{"saturation":-12.0},"grain":{"amount":18.0,"size":25.0,"roughness":50.0}}`
- Function: `lightcraftPresetWarmFilm`; Reference ID and group: `lc.warm-film` (Film); Recipe: `{"wb":{"mode":"custom","temp":6900.0,"tint":5.0},"curve":{"shadows":22.0},"grading":{"highlights":{"hue":45.0,"sat":15.0,"lum":0.0}},"grain":{"amount":22.0,"size":30.0,"roughness":55.0},"vignette":{"amount":-12.0}}`
- Function: `lightcraftPresetMutedFilm`; Reference ID and group: `lc.muted-film` (Film); Recipe: `{"color":{"saturation":-25.0,"vibrance":5.0},"light":{"contrast":10.0},"curve":{"shadows":18.0},"grain":{"amount":15.0}}`
- Function: `lightcraftPresetBwHighContrast`; Reference ID and group: `lc.bw-high-contrast` (B&W); Recipe: `{"treatment":"bw","light":{"contrast":45.0,"whites":20.0,"blacks":-25.0},"effects":{"clarity":20.0},"bw_mix":{"blue":-30.0,"red":15.0}}`
- Function: `lightcraftPresetBwSoft`; Reference ID and group: `lc.bw-soft` (B&W); Recipe: `{"treatment":"bw","light":{"contrast":-15.0,"shadows":25.0},"curve":{"shadows":15.0}}`
- Function: `lightcraftPresetBwSelenium`; Reference ID and group: `lc.bw-selenium` (B&W); Recipe: `{"treatment":"bw","light":{"contrast":20.0},"grading":{"shadows":{"hue":285.0,"sat":18.0,"lum":0.0},"highlights":{"hue":40.0,"sat":10.0,"lum":0.0}}}`
- Function: `lightcraftPresetCrispLandscape`; Reference ID and group: `lc.crisp-landscape` (Landscape); Recipe: `{"light":{"highlights":-45.0,"shadows":30.0,"contrast":12.0},"effects":{"clarity":22.0,"dehaze":12.0,"texture":15.0},"color":{"vibrance":25.0}}`
- Function: `lightcraftPresetGoldenHour`; Reference ID and group: `lc.golden-hour` (Landscape); Recipe: `{"wb":{"mode":"custom","temp":7600.0,"tint":12.0},"light":{"highlights":-35.0,"shadows":20.0},"grading":{"highlights":{"hue":38.0,"sat":25.0,"lum":5.0}},"vignette":{"amount":-15.0}}`
- Function: `lightcraftPresetBlueHour`; Reference ID and group: `lc.blue-hour` (Landscape); Recipe: `{"wb":{"mode":"custom","temp":5200.0,"tint":6.0},"light":{"shadows":25.0,"exposure":0.2},"mixer":{"blue":{"hue":0.0,"sat":25.0,"lum":0.0},"purple":{"hue":0.0,"sat":15.0,"lum":0.0}}}`
- Function: `lightcraftPresetSoftSkin`; Reference ID and group: `lc.soft-skin` (Portrait); Recipe: `{"effects":{"texture":-25.0,"clarity":-10.0},"light":{"contrast":-5.0,"shadows":15.0},"mixer":{"orange":{"hue":0.0,"sat":-8.0,"lum":10.0}}}`
- Function: `lightcraftPresetBrightAiry`; Reference ID and group: `lc.bright-airy` (Portrait); Recipe: `{"light":{"exposure":0.45,"contrast":-15.0,"highlights":-40.0,"shadows":40.0,"whites":15.0},"color":{"vibrance":8.0,"saturation":-8.0}}`
- Function: `lightcraftPresetMoody`; Reference ID and group: `lc.moody` (Style); Recipe: `{"light":{"exposure":-0.3,"contrast":20.0,"highlights":-30.0,"blacks":-15.0},"color":{"saturation":-20.0},"grading":{"shadows":{"hue":200.0,"sat":15.0,"lum":-5.0}},"vignette":{"amount":-30.0}}`
- Function: `lightcraftPresetCinematic`; Reference ID and group: `lc.cinematic` (Style); Recipe: `{"curve":{"shadows":20.0,"highlights":-10.0},"grading":{"shadows":{"hue":190.0,"sat":25.0,"lum":0.0},"highlights":{"hue":30.0,"sat":18.0,"lum":0.0}},"light":{"contrast":15.0},"vignette":{"amount":-20.0}}`
- Function: `lightcraftPresetMoodyPortrait`; Reference ID and group: `lc.moody-portrait` (Portrait); Recipe: `{"light":{"exposure":-0.25,"contrast":20.0,"highlights":-30.0,"blacks":-12.0},"color":{"saturation":-15.0},"vignette":{"amount":-25.0,"midpoint":40.0},"mixer":{"orange":{"sat":6.0}}}`
- Function: `lightcraftPresetGoldenGlow`; Reference ID and group: `lc.golden-glow` (Portrait); Recipe: `{"grading":{"highlights":{"hue":40.0,"sat":18.0,"lum":0.0},"midtones":{"hue":35.0,"sat":8.0,"lum":0.0}},"light":{"shadows":15.0},"effects":{"clarity":-8.0}}`
- Function: `lightcraftPresetCleanStudio`; Reference ID and group: `lc.clean-studio` (Portrait); Recipe: `{"light":{"contrast":10.0,"whites":15.0,"blacks":-8.0},"effects":{"texture":-10.0},"color":{"vibrance":5.0}}`
- Function: `lightcraftPresetCrispVista`; Reference ID and group: `lc.crisp-vista` (Landscape); Recipe: `{"effects":{"dehaze":15.0,"clarity":18.0,"texture":12.0},"light":{"contrast":12.0,"highlights":-25.0,"shadows":18.0},"color":{"vibrance":20.0}}`
- Function: `lightcraftPresetDeepSky`; Reference ID and group: `lc.deep-sky` (Landscape); Recipe: `{"mixer":{"blue":{"lum":-25.0,"sat":15.0},"aqua":{"lum":-10.0}},"light":{"highlights":-30.0},"effects":{"dehaze":10.0}}`
- Function: `lightcraftPresetLushGreens`; Reference ID and group: `lc.lush-greens` (Landscape); Recipe: `{"mixer":{"green":{"hue":12.0,"sat":18.0,"lum":8.0},"yellow":{"hue":10.0,"sat":10.0}},"color":{"vibrance":12.0},"effects":{"clarity":8.0}}`
- Function: `lightcraftPresetDesertWarmth`; Reference ID and group: `lc.desert-warmth` (Landscape); Recipe: `{"mixer":{"orange":{"sat":15.0,"lum":5.0},"yellow":{"hue":-8.0,"sat":10.0},"blue":{"sat":-10.0}},"grading":{"highlights":{"hue":38.0,"sat":12.0,"lum":0.0}},"light":{"contrast":10.0}}`
- Function: `lightcraftPresetMistyMorning`; Reference ID and group: `lc.misty-morning` (Landscape); Recipe: `{"effects":{"dehaze":-18.0,"clarity":-12.0},"light":{"contrast":-18.0,"highlights":-10.0},"grading":{"shadows":{"hue":210.0,"sat":10.0,"lum":0.0}},"color":{"saturation":-10.0}}`
- Function: `lightcraftPresetGrittyStreet`; Reference ID and group: `lc.gritty-street` (Urban); Recipe: `{"effects":{"clarity":35.0,"texture":20.0},"light":{"contrast":25.0,"blacks":-15.0},"color":{"saturation":-30.0},"vignette":{"amount":-18.0}}`
- Function: `lightcraftPresetNeonNight`; Reference ID and group: `lc.neon-night` (Urban); Recipe: `{"mixer":{"magenta":{"sat":20.0},"purple":{"sat":18.0},"blue":{"sat":12.0},"aqua":{"sat":15.0}},"light":{"contrast":18.0,"blacks":-12.0},"color":{"vibrance":15.0},"grading":{"shadows":{"hue":250.0,"sat":15.0,"lum":0.0}}}`
- Function: `lightcraftPresetConcreteCool`; Reference ID and group: `lc.concrete-cool` (Urban); Recipe: `{"grading":{"shadows":{"hue":205.0,"sat":14.0,"lum":0.0},"highlights":{"hue":200.0,"sat":6.0,"lum":0.0}},"color":{"saturation":-20.0},"light":{"contrast":12.0},"effects":{"clarity":15.0}}`
- Function: `lightcraftPresetFadedUrban`; Reference ID and group: `lc.faded-urban` (Urban); Recipe: `{"curve":{"shadows":30.0},"color":{"saturation":-22.0},"light":{"contrast":-8.0},"grain":{"amount":12.0,"size":25.0,"roughness":45.0}}`
- Function: `lightcraftPresetFreshBright`; Reference ID and group: `lc.fresh-bright` (Food); Recipe: `{"light":{"exposure":0.25,"shadows":20.0,"whites":10.0},"color":{"vibrance":22.0},"effects":{"texture":15.0}}`
- Function: `lightcraftPresetWarmTable`; Reference ID and group: `lc.warm-table` (Food); Recipe: `{"grading":{"midtones":{"hue":32.0,"sat":10.0,"lum":0.0}},"mixer":{"orange":{"sat":10.0},"red":{"sat":8.0}},"light":{"contrast":8.0}}`
- Function: `lightcraftPresetDarkMoodyFood`; Reference ID and group: `lc.dark-moody-food` (Food); Recipe: `{"light":{"exposure":-0.3,"contrast":22.0,"highlights":-20.0,"blacks":-15.0},"effects":{"texture":18.0},"vignette":{"amount":-22.0}}`
- Function: `lightcraftPresetAutumnGold`; Reference ID and group: `lc.autumn-gold` (Seasons); Recipe: `{"mixer":{"green":{"hue":-30.0,"sat":-10.0},"yellow":{"hue":-15.0,"sat":15.0},"orange":{"sat":18.0}},"grading":{"highlights":{"hue":40.0,"sat":10.0,"lum":0.0}}}`
- Function: `lightcraftPresetWinterBlue`; Reference ID and group: `lc.winter-blue` (Seasons); Recipe: `{"grading":{"shadows":{"hue":215.0,"sat":18.0,"lum":0.0},"highlights":{"hue":205.0,"sat":8.0,"lum":0.0}},"color":{"saturation":-12.0},"light":{"whites":12.0}}`
- Function: `lightcraftPresetSpringFresh`; Reference ID and group: `lc.spring-fresh` (Seasons); Recipe: `{"mixer":{"green":{"hue":10.0,"sat":12.0,"lum":10.0},"magenta":{"sat":10.0}},"light":{"shadows":15.0},"color":{"vibrance":15.0}}`
- Function: `lightcraftPresetSummerHaze`; Reference ID and group: `lc.summer-haze` (Seasons); Recipe: `{"effects":{"dehaze":-12.0},"curve":{"shadows":18.0},"grading":{"highlights":{"hue":45.0,"sat":14.0,"lum":0.0}},"light":{"contrast":-10.0}}`
- Function: `lightcraftPresetInstant70s`; Reference ID and group: `lc.instant-70s` (Vintage); Recipe: `{"curve":{"shadows":28.0,"highlights":-18.0},"grading":{"shadows":{"hue":170.0,"sat":14.0,"lum":0.0},"highlights":{"hue":45.0,"sat":18.0,"lum":0.0}},"color":{"saturation":-10.0},"vignette":{"amount":-15.0},"grain":{"amount":20.0,"size":30.0,"roughness":55.0}}`
- Function: `lightcraftPresetCrossProcess`; Reference ID and group: `lc.cross-process` (Vintage); Recipe: `{"grading":{"shadows":{"hue":230.0,"sat":25.0,"lum":0.0},"highlights":{"hue":60.0,"sat":25.0,"lum":0.0}},"light":{"contrast":22.0},"color":{"saturation":10.0}}`
- Function: `lightcraftPresetBleachBypass`; Reference ID and group: `lc.bleach-bypass` (Vintage); Recipe: `{"color":{"saturation":-40.0},"light":{"contrast":35.0,"highlights":-15.0},"effects":{"clarity":15.0}}`
- Function: `lightcraftPresetBwSepia`; Reference ID and group: `lc.bw-sepia` (B&W); Recipe: `{"treatment":"bw","grading":{"shadows":{"hue":32.0,"sat":22.0,"lum":0.0},"highlights":{"hue":42.0,"sat":18.0,"lum":0.0}},"curve":{"shadows":10.0}}`

## Resampling functions

[`raster/src/resample.rs`](https://github.com/storytold/lightcraft/blob/629e39380e296f588c64cd9c0053a8edc3528f36/crates/raster/src/resample.rs#L7) defines these four actual CPU kernels. The GPU uploads the same contribution tables. Every function takes `(ImageView image, std::size_t width, std::size_t height)` and returns `Result<Image>`.

- Function: `resizeBox`; Kernel and support: Rectangular window, `abs(x)<=.5`
- Function: `resizeBilinear`; Kernel and support: Triangle, `max(0,1-abs(x))`, support 1
- Function: `resizeMitchell`; Kernel and support: Mitchell-Netravali cubic with `B=C=1/3`, support 2
- Function: `resizeLanczos3`; Kernel and support: `sinc(x)*sinc(x/3)`, support 3

The center of output coordinate `i` is `(i+.5)*source/destination`. Downscaling widens support by `max(1,source/destination)` for anti-aliasing. Each axis normalizes its cropped in-bounds taps; it neither wraps nor pads colors beyond the source. Filtering is separable, horizontal then vertical, using double-precision accumulation. All channels and alpha are filtered in linear-light premultiplied RGBA. RGB is divided by the unbounded filtered alpha before alpha and RGB are clipped, so negative cubic/sinc lobes do not generate a dark straight-alpha halo. Newly invisible output uses `{0,0,0,0}`. Same dimensions return an exact detached copy instead of filtering.

Each input/output axis must be in `[1,65536]`. The input image, output image and horizontal intermediate (`output width * input height`) must each contain no more than `64*1024*1024` pixels. The contribution table is bounded to `1024*1024` taps per axis. Empty or overflowing dimensions, bounds violations, mismatched buffers, nonfinite channels and RGBA outside `[0,1]` are rejected before processing. Allocation failures return `AllocationFailure`. The reference clamps zero output dimensions to one; this SDK rejects zero to preserve its image-validation contract.

These are post-render raster filters. Upstream texture sampling in the GUI, Bayer/X-Trans demosaicing and internal pyramid resampling are separate execution contexts. `resizeBox` is a size-changing reconstruction kernel, distinct from a fixed-size box blur.

## Verification

`tests/lightcraft_filters.cpp` offers `looks`, `presets`, `resize`, `invalid` and `all` groups. It calls every named function at zero, quarter, full and double amount; verifies finite normalized output, alpha and hidden-RGB preservation; compares every builtin preset field with fixed upstream JSON fixtures; checks selected look deltas, fade endpoints, B&W treatment and grading; and verifies an explicitly assembled Develop recipe. Resampling tests use closed-form two-pixel reference values, the linear-light mean, constant-signal conservation, axis symmetry, detached identity, transparent-edge color isolation and validation without allocating huge images.
