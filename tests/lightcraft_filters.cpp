#include <algorithm>
#include <array>
#include <cmath>
#include <iiGraphicAdjust/LightcraftFilters.h>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
using namespace iiGraphicAdjust;
namespace {
using Operation = Result<Image> (*)(ImageView, double);
struct LookFixture {
    LightcraftLook look;
    Operation operation;
    const char *name;
};
const std::array<LookFixture, 22> looks{{
    {LightcraftLook::Color, lightcraftLookColor, "Color"},
    {LightcraftLook::Neutral, lightcraftLookNeutral, "Neutral"},
    {LightcraftLook::Vivid, lightcraftLookVivid, "Vivid"},
    {LightcraftLook::Landscape, lightcraftLookLandscape, "Landscape"},
    {LightcraftLook::Portrait, lightcraftLookPortrait, "Portrait"},
    {LightcraftLook::Mono, lightcraftLookMono, "Mono"},
    {LightcraftLook::FilmWarmPrint, lightcraftLookFilmWarmPrint, "FilmWarmPrint"},
    {LightcraftLook::FilmCoolFade, lightcraftLookFilmCoolFade, "FilmCoolFade"},
    {LightcraftLook::FilmGoldenHour, lightcraftLookFilmGoldenHour, "FilmGoldenHour"},
    {LightcraftLook::FilmFadedSlide, lightcraftLookFilmFadedSlide, "FilmFadedSlide"},
    {LightcraftLook::CineTealAmber, lightcraftLookCineTealAmber, "CineTealAmber"},
    {LightcraftLook::CineNightBlue, lightcraftLookCineNightBlue, "CineNightBlue"},
    {LightcraftLook::CineDesertHeat, lightcraftLookCineDesertHeat, "CineDesertHeat"},
    {LightcraftLook::CineNeonDusk, lightcraftLookCineNeonDusk, "CineNeonDusk"},
    {LightcraftLook::MutedMatteSoft, lightcraftLookMutedMatteSoft, "MutedMatteSoft"},
    {LightcraftLook::MutedBleached, lightcraftLookMutedBleached, "MutedBleached"},
    {LightcraftLook::MutedPastelHaze, lightcraftLookMutedPastelHaze, "MutedPastelHaze"},
    {LightcraftLook::MutedQuietGreen, lightcraftLookMutedQuietGreen, "MutedQuietGreen"},
    {LightcraftLook::BwMonoRich, lightcraftLookBwMonoRich, "BwMonoRich"},
    {LightcraftLook::BwRedFilter, lightcraftLookBwRedFilter, "BwRedFilter"},
    {LightcraftLook::BwSoft, lightcraftLookBwSoft, "BwSoft"},
    {LightcraftLook::BwSepia, lightcraftLookBwSepia, "BwSepia"},
}};
struct PresetFixture {
    LightcraftPreset preset;
    Operation operation;
    const char *name;
    std::vector<std::pair<std::string_view, double>> fields;
};
// Independent numeric fixture: the pinned upstream builtin() JSON, not SDK-produced values.
const std::array<PresetFixture, 41> presets{{
    {LightcraftPreset::WarmGlow,
     lightcraftPresetWarmGlow,
     "WarmGlow",
     {{"wb.temp", 7200.0},
      {"wb.tint", 8.0},
      {"light.contrast", 8.0},
      {"light.shadows", 12.0},
      {"color.vibrance", 18.0}}},
    {LightcraftPreset::CoolMorning,
     lightcraftPresetCoolMorning,
     "CoolMorning",
     {{"wb.temp", 5600.0},
      {"wb.tint", -4.0},
      {"light.highlights", -20.0},
      {"color.vibrance", 10.0},
      {"grading.shadows.hue", 215.0},
      {"grading.shadows.sat", 12.0},
      {"grading.shadows.lum", 0.0}}},
    {LightcraftPreset::TealOrange,
     lightcraftPresetTealOrange,
     "TealOrange",
     {{"grading.shadows.hue", 195.0},
      {"grading.shadows.sat", 28.0},
      {"grading.shadows.lum", 0.0},
      {"grading.highlights.hue", 35.0},
      {"grading.highlights.sat", 22.0},
      {"grading.highlights.lum", 0.0},
      {"grading.balance", 10.0},
      {"color.vibrance", 12.0},
      {"light.contrast", 14.0}}},
    {LightcraftPreset::VividPop,
     lightcraftPresetVividPop,
     "VividPop",
     {{"color.vibrance", 40.0},
      {"color.saturation", 8.0},
      {"light.contrast", 18.0},
      {"light.whites", 10.0},
      {"light.blacks", -10.0},
      {"effects.clarity", 12.0}}},
    {LightcraftPreset::SoftPastel,
     lightcraftPresetSoftPastel,
     "SoftPastel",
     {{"light.contrast", -25.0},
      {"light.highlights", -30.0},
      {"light.shadows", 35.0},
      {"light.blacks", 20.0},
      {"color.saturation", -18.0},
      {"color.vibrance", 10.0},
      {"curve.shadows", 25.0}}},
    {LightcraftPreset::FadedMatte,
     lightcraftPresetFadedMatte,
     "FadedMatte",
     {{"curve.shadows", 40.0},
      {"curve.highlights", -15.0},
      {"light.contrast", -10.0},
      {"color.saturation", -12.0},
      {"grain.amount", 18.0},
      {"grain.size", 25.0},
      {"grain.roughness", 50.0}}},
    {LightcraftPreset::WarmFilm,
     lightcraftPresetWarmFilm,
     "WarmFilm",
     {{"wb.temp", 6900.0},
      {"wb.tint", 5.0},
      {"curve.shadows", 22.0},
      {"grading.highlights.hue", 45.0},
      {"grading.highlights.sat", 15.0},
      {"grading.highlights.lum", 0.0},
      {"grain.amount", 22.0},
      {"grain.size", 30.0},
      {"grain.roughness", 55.0},
      {"vignette.amount", -12.0}}},
    {LightcraftPreset::MutedFilm,
     lightcraftPresetMutedFilm,
     "MutedFilm",
     {{"color.saturation", -25.0},
      {"color.vibrance", 5.0},
      {"light.contrast", 10.0},
      {"curve.shadows", 18.0},
      {"grain.amount", 15.0}}},
    {LightcraftPreset::BwHighContrast,
     lightcraftPresetBwHighContrast,
     "BwHighContrast",
     {{"treatment", 1.0},
      {"light.contrast", 45.0},
      {"light.whites", 20.0},
      {"light.blacks", -25.0},
      {"effects.clarity", 20.0},
      {"bw_mix.blue", -30.0},
      {"bw_mix.red", 15.0}}},
    {LightcraftPreset::BwSoft,
     lightcraftPresetBwSoft,
     "BwSoft",
     {{"treatment", 1.0},
      {"light.contrast", -15.0},
      {"light.shadows", 25.0},
      {"curve.shadows", 15.0}}},
    {LightcraftPreset::BwSelenium,
     lightcraftPresetBwSelenium,
     "BwSelenium",
     {{"treatment", 1.0},
      {"light.contrast", 20.0},
      {"grading.shadows.hue", 285.0},
      {"grading.shadows.sat", 18.0},
      {"grading.shadows.lum", 0.0},
      {"grading.highlights.hue", 40.0},
      {"grading.highlights.sat", 10.0},
      {"grading.highlights.lum", 0.0}}},
    {LightcraftPreset::CrispLandscape,
     lightcraftPresetCrispLandscape,
     "CrispLandscape",
     {{"light.highlights", -45.0},
      {"light.shadows", 30.0},
      {"light.contrast", 12.0},
      {"effects.clarity", 22.0},
      {"effects.dehaze", 12.0},
      {"effects.texture", 15.0},
      {"color.vibrance", 25.0}}},
    {LightcraftPreset::GoldenHour,
     lightcraftPresetGoldenHour,
     "GoldenHour",
     {{"wb.temp", 7600.0},
      {"wb.tint", 12.0},
      {"light.highlights", -35.0},
      {"light.shadows", 20.0},
      {"grading.highlights.hue", 38.0},
      {"grading.highlights.sat", 25.0},
      {"grading.highlights.lum", 5.0},
      {"vignette.amount", -15.0}}},
    {LightcraftPreset::BlueHour,
     lightcraftPresetBlueHour,
     "BlueHour",
     {{"wb.temp", 5200.0},
      {"wb.tint", 6.0},
      {"light.shadows", 25.0},
      {"light.exposure", 0.2},
      {"mixer.blue.hue", 0.0},
      {"mixer.blue.sat", 25.0},
      {"mixer.blue.lum", 0.0},
      {"mixer.purple.hue", 0.0},
      {"mixer.purple.sat", 15.0},
      {"mixer.purple.lum", 0.0}}},
    {LightcraftPreset::SoftSkin,
     lightcraftPresetSoftSkin,
     "SoftSkin",
     {{"effects.texture", -25.0},
      {"effects.clarity", -10.0},
      {"light.contrast", -5.0},
      {"light.shadows", 15.0},
      {"mixer.orange.hue", 0.0},
      {"mixer.orange.sat", -8.0},
      {"mixer.orange.lum", 10.0}}},
    {LightcraftPreset::BrightAiry,
     lightcraftPresetBrightAiry,
     "BrightAiry",
     {{"light.exposure", 0.45},
      {"light.contrast", -15.0},
      {"light.highlights", -40.0},
      {"light.shadows", 40.0},
      {"light.whites", 15.0},
      {"color.vibrance", 8.0},
      {"color.saturation", -8.0}}},
    {LightcraftPreset::Moody,
     lightcraftPresetMoody,
     "Moody",
     {{"light.exposure", -0.3},
      {"light.contrast", 20.0},
      {"light.highlights", -30.0},
      {"light.blacks", -15.0},
      {"color.saturation", -20.0},
      {"grading.shadows.hue", 200.0},
      {"grading.shadows.sat", 15.0},
      {"grading.shadows.lum", -5.0},
      {"vignette.amount", -30.0}}},
    {LightcraftPreset::Cinematic,
     lightcraftPresetCinematic,
     "Cinematic",
     {{"curve.shadows", 20.0},
      {"curve.highlights", -10.0},
      {"grading.shadows.hue", 190.0},
      {"grading.shadows.sat", 25.0},
      {"grading.shadows.lum", 0.0},
      {"grading.highlights.hue", 30.0},
      {"grading.highlights.sat", 18.0},
      {"grading.highlights.lum", 0.0},
      {"light.contrast", 15.0},
      {"vignette.amount", -20.0}}},
    {LightcraftPreset::MoodyPortrait,
     lightcraftPresetMoodyPortrait,
     "MoodyPortrait",
     {{"light.exposure", -0.25},
      {"light.contrast", 20.0},
      {"light.highlights", -30.0},
      {"light.blacks", -12.0},
      {"color.saturation", -15.0},
      {"vignette.amount", -25.0},
      {"vignette.midpoint", 40.0},
      {"mixer.orange.sat", 6.0}}},
    {LightcraftPreset::GoldenGlow,
     lightcraftPresetGoldenGlow,
     "GoldenGlow",
     {{"grading.highlights.hue", 40.0},
      {"grading.highlights.sat", 18.0},
      {"grading.highlights.lum", 0.0},
      {"grading.midtones.hue", 35.0},
      {"grading.midtones.sat", 8.0},
      {"grading.midtones.lum", 0.0},
      {"light.shadows", 15.0},
      {"effects.clarity", -8.0}}},
    {LightcraftPreset::CleanStudio,
     lightcraftPresetCleanStudio,
     "CleanStudio",
     {{"light.contrast", 10.0},
      {"light.whites", 15.0},
      {"light.blacks", -8.0},
      {"effects.texture", -10.0},
      {"color.vibrance", 5.0}}},
    {LightcraftPreset::CrispVista,
     lightcraftPresetCrispVista,
     "CrispVista",
     {{"effects.dehaze", 15.0},
      {"effects.clarity", 18.0},
      {"effects.texture", 12.0},
      {"light.contrast", 12.0},
      {"light.highlights", -25.0},
      {"light.shadows", 18.0},
      {"color.vibrance", 20.0}}},
    {LightcraftPreset::DeepSky,
     lightcraftPresetDeepSky,
     "DeepSky",
     {{"mixer.blue.lum", -25.0},
      {"mixer.blue.sat", 15.0},
      {"mixer.aqua.lum", -10.0},
      {"light.highlights", -30.0},
      {"effects.dehaze", 10.0}}},
    {LightcraftPreset::LushGreens,
     lightcraftPresetLushGreens,
     "LushGreens",
     {{"mixer.green.hue", 12.0},
      {"mixer.green.sat", 18.0},
      {"mixer.green.lum", 8.0},
      {"mixer.yellow.hue", 10.0},
      {"mixer.yellow.sat", 10.0},
      {"color.vibrance", 12.0},
      {"effects.clarity", 8.0}}},
    {LightcraftPreset::DesertWarmth,
     lightcraftPresetDesertWarmth,
     "DesertWarmth",
     {{"mixer.orange.sat", 15.0},
      {"mixer.orange.lum", 5.0},
      {"mixer.yellow.hue", -8.0},
      {"mixer.yellow.sat", 10.0},
      {"mixer.blue.sat", -10.0},
      {"grading.highlights.hue", 38.0},
      {"grading.highlights.sat", 12.0},
      {"grading.highlights.lum", 0.0},
      {"light.contrast", 10.0}}},
    {LightcraftPreset::MistyMorning,
     lightcraftPresetMistyMorning,
     "MistyMorning",
     {{"effects.dehaze", -18.0},
      {"effects.clarity", -12.0},
      {"light.contrast", -18.0},
      {"light.highlights", -10.0},
      {"grading.shadows.hue", 210.0},
      {"grading.shadows.sat", 10.0},
      {"grading.shadows.lum", 0.0},
      {"color.saturation", -10.0}}},
    {LightcraftPreset::GrittyStreet,
     lightcraftPresetGrittyStreet,
     "GrittyStreet",
     {{"effects.clarity", 35.0},
      {"effects.texture", 20.0},
      {"light.contrast", 25.0},
      {"light.blacks", -15.0},
      {"color.saturation", -30.0},
      {"vignette.amount", -18.0}}},
    {LightcraftPreset::NeonNight,
     lightcraftPresetNeonNight,
     "NeonNight",
     {{"mixer.magenta.sat", 20.0},
      {"mixer.purple.sat", 18.0},
      {"mixer.blue.sat", 12.0},
      {"mixer.aqua.sat", 15.0},
      {"light.contrast", 18.0},
      {"light.blacks", -12.0},
      {"color.vibrance", 15.0},
      {"grading.shadows.hue", 250.0},
      {"grading.shadows.sat", 15.0},
      {"grading.shadows.lum", 0.0}}},
    {LightcraftPreset::ConcreteCool,
     lightcraftPresetConcreteCool,
     "ConcreteCool",
     {{"grading.shadows.hue", 205.0},
      {"grading.shadows.sat", 14.0},
      {"grading.shadows.lum", 0.0},
      {"grading.highlights.hue", 200.0},
      {"grading.highlights.sat", 6.0},
      {"grading.highlights.lum", 0.0},
      {"color.saturation", -20.0},
      {"light.contrast", 12.0},
      {"effects.clarity", 15.0}}},
    {LightcraftPreset::FadedUrban,
     lightcraftPresetFadedUrban,
     "FadedUrban",
     {{"curve.shadows", 30.0},
      {"color.saturation", -22.0},
      {"light.contrast", -8.0},
      {"grain.amount", 12.0},
      {"grain.size", 25.0},
      {"grain.roughness", 45.0}}},
    {LightcraftPreset::FreshBright,
     lightcraftPresetFreshBright,
     "FreshBright",
     {{"light.exposure", 0.25},
      {"light.shadows", 20.0},
      {"light.whites", 10.0},
      {"color.vibrance", 22.0},
      {"effects.texture", 15.0}}},
    {LightcraftPreset::WarmTable,
     lightcraftPresetWarmTable,
     "WarmTable",
     {{"grading.midtones.hue", 32.0},
      {"grading.midtones.sat", 10.0},
      {"grading.midtones.lum", 0.0},
      {"mixer.orange.sat", 10.0},
      {"mixer.red.sat", 8.0},
      {"light.contrast", 8.0}}},
    {LightcraftPreset::DarkMoodyFood,
     lightcraftPresetDarkMoodyFood,
     "DarkMoodyFood",
     {{"light.exposure", -0.3},
      {"light.contrast", 22.0},
      {"light.highlights", -20.0},
      {"light.blacks", -15.0},
      {"effects.texture", 18.0},
      {"vignette.amount", -22.0}}},
    {LightcraftPreset::AutumnGold,
     lightcraftPresetAutumnGold,
     "AutumnGold",
     {{"mixer.green.hue", -30.0},
      {"mixer.green.sat", -10.0},
      {"mixer.yellow.hue", -15.0},
      {"mixer.yellow.sat", 15.0},
      {"mixer.orange.sat", 18.0},
      {"grading.highlights.hue", 40.0},
      {"grading.highlights.sat", 10.0},
      {"grading.highlights.lum", 0.0}}},
    {LightcraftPreset::WinterBlue,
     lightcraftPresetWinterBlue,
     "WinterBlue",
     {{"grading.shadows.hue", 215.0},
      {"grading.shadows.sat", 18.0},
      {"grading.shadows.lum", 0.0},
      {"grading.highlights.hue", 205.0},
      {"grading.highlights.sat", 8.0},
      {"grading.highlights.lum", 0.0},
      {"color.saturation", -12.0},
      {"light.whites", 12.0}}},
    {LightcraftPreset::SpringFresh,
     lightcraftPresetSpringFresh,
     "SpringFresh",
     {{"mixer.green.hue", 10.0},
      {"mixer.green.sat", 12.0},
      {"mixer.green.lum", 10.0},
      {"mixer.magenta.sat", 10.0},
      {"light.shadows", 15.0},
      {"color.vibrance", 15.0}}},
    {LightcraftPreset::SummerHaze,
     lightcraftPresetSummerHaze,
     "SummerHaze",
     {{"effects.dehaze", -12.0},
      {"curve.shadows", 18.0},
      {"grading.highlights.hue", 45.0},
      {"grading.highlights.sat", 14.0},
      {"grading.highlights.lum", 0.0},
      {"light.contrast", -10.0}}},
    {LightcraftPreset::Instant70s,
     lightcraftPresetInstant70s,
     "Instant70s",
     {{"curve.shadows", 28.0},
      {"curve.highlights", -18.0},
      {"grading.shadows.hue", 170.0},
      {"grading.shadows.sat", 14.0},
      {"grading.shadows.lum", 0.0},
      {"grading.highlights.hue", 45.0},
      {"grading.highlights.sat", 18.0},
      {"grading.highlights.lum", 0.0},
      {"color.saturation", -10.0},
      {"vignette.amount", -15.0},
      {"grain.amount", 20.0},
      {"grain.size", 30.0},
      {"grain.roughness", 55.0}}},
    {LightcraftPreset::CrossProcess,
     lightcraftPresetCrossProcess,
     "CrossProcess",
     {{"grading.shadows.hue", 230.0},
      {"grading.shadows.sat", 25.0},
      {"grading.shadows.lum", 0.0},
      {"grading.highlights.hue", 60.0},
      {"grading.highlights.sat", 25.0},
      {"grading.highlights.lum", 0.0},
      {"light.contrast", 22.0},
      {"color.saturation", 10.0}}},
    {LightcraftPreset::BleachBypass,
     lightcraftPresetBleachBypass,
     "BleachBypass",
     {{"color.saturation", -40.0},
      {"light.contrast", 35.0},
      {"light.highlights", -15.0},
      {"effects.clarity", 15.0}}},
    {LightcraftPreset::BwSepia,
     lightcraftPresetBwSepia,
     "BwSepia",
     {{"treatment", 1.0},
      {"grading.shadows.hue", 32.0},
      {"grading.shadows.sat", 22.0},
      {"grading.shadows.lum", 0.0},
      {"grading.highlights.hue", 42.0},
      {"grading.highlights.sat", 18.0},
      {"grading.highlights.lum", 0.0},
      {"curve.shadows", 10.0}}},
}};

std::vector<std::string_view> pathParts(std::string_view path) {
    std::vector<std::string_view> parts;
    for (;;) {
        auto dot = path.find('.');
        parts.push_back(path.substr(0, dot));
        if (dot == std::string_view::npos)
            return parts;
        path.remove_prefix(dot + 1);
    }
}
double field(const DevelopOptions &o, std::string_view name) {
    auto p = pathParts(name);
    if (p[0] == "treatment")
        return o.color.blackAndWhite ? 1 : 0;
    if (p[0] == "wb")
        return p[1] == "temp" ? o.whiteBalance.temperature : o.whiteBalance.tint;
    if (p[0] == "light") {
        if (p[1] == "exposure")
            return o.light.exposure;
        if (p[1] == "contrast")
            return o.light.contrast;
        if (p[1] == "highlights")
            return o.light.highlights;
        if (p[1] == "shadows")
            return o.light.shadows;
        if (p[1] == "whites")
            return o.light.whites;
        if (p[1] == "blacks")
            return o.light.blacks;
    }
    if (p[0] == "color")
        return p[1] == "saturation" ? o.color.saturation : o.color.vibrance;
    if (p[0] == "curve") {
        if (p[1] == "shadows")
            return o.curve.shadows;
        if (p[1] == "darks")
            return o.curve.darks;
        if (p[1] == "lights")
            return o.curve.lights;
        if (p[1] == "highlights")
            return o.curve.highlights;
    }
    if (p[0] == "effects") {
        if (p[1] == "texture")
            return o.effects.texture;
        if (p[1] == "clarity")
            return o.effects.clarity;
        if (p[1] == "dehaze")
            return o.effects.dehaze;
    }
    if (p[0] == "grain") {
        if (p[1] == "amount")
            return o.grain.amount;
        if (p[1] == "size")
            return o.grain.size;
        if (p[1] == "roughness")
            return o.grain.roughness;
    }
    if (p[0] == "vignette") {
        if (p[1] == "amount")
            return o.vignette.amount;
        if (p[1] == "midpoint")
            return o.vignette.midpoint;
    }
    if (p[0] == "grading") {
        if (p[1] == "balance")
            return o.color.grading.balance;
        if (p[1] == "blending")
            return o.color.grading.blending;
        auto wheel = p[1] == "shadows"      ? o.color.grading.shadows
                     : p[1] == "midtones"   ? o.color.grading.midtones
                     : p[1] == "highlights" ? o.color.grading.highlights
                                            : o.color.grading.global;
        return p[2] == "hue" ? wheel.hue : p[2] == "sat" ? wheel.saturation : wheel.luminance;
    }
    constexpr std::array<std::string_view, 8> colors{"red",  "orange", "yellow", "green",
                                                     "aqua", "blue",   "purple", "magenta"};
    if (p[0] == "mixer" || p[0] == "bw_mix") {
        auto i = std::find(colors.begin(), colors.end(), p[1]) - colors.begin();
        if (i == 8)
            return std::numeric_limits<double>::quiet_NaN();
        if (p[0] == "bw_mix")
            return o.color.blackAndWhiteMix[i];
        const auto band = o.color.mixer[i];
        return p[2] == "hue" ? band.hue : p[2] == "sat" ? band.saturation : band.luminance;
    }
    return std::numeric_limits<double>::quiet_NaN();
}
bool finiteNormalized(const Image &image) {
    return std::all_of(image.pixels.begin(), image.pixels.end(), [](Rgba p) {
        for (float c : {p.r, p.g, p.b, p.a})
            if (!std::isfinite(c) || c < 0 || c > 1)
                return false;
        return true;
    });
}
bool sameAlphaHidden(const Image &output, const Image &input) {
    for (std::size_t i = 0; i < input.pixels.size(); ++i)
        if (output.pixels[i].a != input.pixels[i].a ||
            (input.pixels[i].a == 0 && output.pixels[i] != input.pixels[i]))
            return false;
    return true;
}
} // namespace
int main(int argc, char **argv) {
    int failures = 0;
    auto check = [&](bool passed, const std::string &message) {
        if (!passed) {
            std::cerr << message << '\n';
            ++failures;
        }
    };
    auto near = [](double a, double b, double tolerance = 2e-6) {
        return std::abs(a - b) <= tolerance;
    };
    const std::string group = argc > 1 ? argv[1] : "all";
    auto run = [&](const char *name) { return group == "all" || group == name; };
    Image image{6, 4, {}};
    for (int i = 0; i < 24; ++i)
        image.pixels.push_back({((i * 17) % 29) / 28.F, ((i * 11 + 7) % 31) / 30.F,
                                ((i * 3 + 13) % 19) / 18.F,
                                i % 7 == 0   ? 0.F
                                : i % 5 == 0 ? .2F
                                             : 1.F});
    const auto original = image.pixels;
    if (run("looks")) {
        for (const auto &fixture : looks) {
            for (double amount : {0., .25, 1., 2.}) {
                auto output = fixture.operation(image.view(), amount);
                check(output && output->width == image.width && output->height == image.height &&
                          finiteNormalized(*output) && sameAlphaHidden(*output, image),
                      std::string(fixture.name) + " validated output and alpha");
                if (amount == 0)
                    check(output && output->pixels == image.pixels,
                          std::string(fixture.name) + " zero identity");
            }
        }
        auto color = lightcraftLookColor(image.view());
        check(color && color->pixels == original, "Color profile identity");
        const auto neutral = lightcraftLookOptions(LightcraftLook::Neutral);
        check(neutral && neutral->light.contrast == -18 && neutral->light.highlights == -10 &&
                  neutral->light.shadows == 8 && neutral->color.saturation == -10 &&
                  neutral->color.vibrance == 0,
              "Neutral recipe oracle");
        const auto landscape = lightcraftLookOptions(LightcraftLook::Landscape, 2);
        check(landscape && landscape->color.mixer[3].saturation == 28 &&
                  landscape->color.mixer[4].saturation == 20 &&
                  landscape->color.mixer[5].saturation == 28 &&
                  landscape->color.mixer[5].luminance == -12 && landscape->effects.clarity == 12,
              "Landscape mixer partition oracle");
        const auto print = lightcraftLookOptions(LightcraftLook::FilmWarmPrint, .5);
        check(print && near(print->curve.master.points.front().output, .02) &&
                  near(print->curve.master.points.back().output, .985) &&
                  print->color.grading.highlights.hue == 40 &&
                  print->color.grading.highlights.saturation == 9 &&
                  print->color.grading.shadows.hue == 25 &&
                  print->color.grading.shadows.saturation == 4,
              "Warm Print fade and wheel vector oracle");
        const auto slide = lightcraftLookOptions(LightcraftLook::FilmFadedSlide);
        check(slide && near(slide->curve.blue.points.front().output, .06) &&
                  near(slide->curve.red.points.back().output, .97) &&
                  slide->curve.blue.points.back().output == 1,
              "Faded Slide channel endpoint oracle");
        const auto red = lightcraftLookOptions(LightcraftLook::BwRedFilter, 2);
        check(red && red->color.blackAndWhite && red->color.blackAndWhiteMix[0] == 80 &&
                  red->color.blackAndWhiteMix[5] == -90 && red->color.blackAndWhiteMix[7] == 0,
              "B&W red mix oracle");
        auto mono = lightcraftLookMono(image.view(), .001);
        check(mono && mono->pixels[1].r == mono->pixels[1].g &&
                  mono->pixels[1].r == mono->pixels[1].b,
              "look treatment applies at positive amount");
        auto sepia = lightcraftLookBwSepia(Image{1, 1, {{.5F, .5F, .5F, 1}}}.view());
        check(sepia && sepia->pixels[0].r > sepia->pixels[0].b,
              "Sepia grayscale then warm grading");
    }
    if (run("presets")) {
        const DevelopOptions baseline;
        for (const auto &fixture : presets) {
            for (double amount : {0., .25, 1., 2.}) {
                const auto options = lightcraftPresetOptions(fixture.preset, amount);
                check(bool(options), std::string(fixture.name) + " valid recipe");
                if (options)
                    for (const auto &[name, target] : fixture.fields) {
                        double expected =
                            field(baseline, name) + (target - field(baseline, name)) * amount;
                        if (name == "treatment")
                            expected = amount >= .5 ? target : field(baseline, name);
                        else if (name.starts_with("wb.") && amount < .5)
                            expected = field(baseline, name);
                        else if (name.starts_with("grading.") && name.ends_with(".hue"))
                            expected = std::clamp(expected, 0., 360.);
                        else if (name.starts_with("grain.") || name == "vignette.midpoint" ||
                                 (name.starts_with("grading.") && name.ends_with(".sat")))
                            expected = std::clamp(expected, 0., 100.);
                        else if (name == "light.exposure")
                            expected = std::clamp(expected, -5., 5.);
                        else if (!name.starts_with("wb.") && name != "treatment")
                            expected = std::clamp(expected, -100., 100.);
                        check(near(field(*options, name), expected, 1e-10),
                              std::string(fixture.name) + " upstream recipe " + std::string(name));
                    }
                auto output = fixture.operation(image.view(), amount);
                check(output && finiteNormalized(*output) && sameAlphaHidden(*output, image),
                      std::string(fixture.name) + " finite output and alpha");
                if (amount == 0)
                    check(output && output->pixels == original,
                          std::string(fixture.name) + " zero identity");
            }
        }
        DevelopOptions warm;
        warm.whiteBalance.temperature = 7200;
        warm.whiteBalance.tint = 8;
        warm.light.contrast = 8;
        warm.light.shadows = 12;
        warm.color.vibrance = 18;
        const auto explicitOutput = develop(image.view(), warm),
                   namedOutput = lightcraftPresetWarmGlow(image.view());
        check(explicitOutput && namedOutput && explicitOutput->pixels == namedOutput->pixels,
              "Warm Glow explicit ordered Develop recipe");
        const auto lowBw = lightcraftPresetOptions(LightcraftPreset::BwSoft, .499),
                   highBw = lightcraftPresetOptions(LightcraftPreset::BwSoft, .5);
        check(lowBw && highBw && !lowBw->color.blackAndWhite && highBw->color.blackAndWhite,
              "preset treatment switches at half amount");
        const auto lowWb = lightcraftPresetOptions(LightcraftPreset::WarmGlow, .499),
                   highWb = lightcraftPresetOptions(LightcraftPreset::WarmGlow, .5);
        check(lowWb && highWb && lowWb->whiteBalance.temperature == 6500 &&
                  highWb->whiteBalance.temperature == 6850,
              "preset WB custom mode threshold follows reference");
    }
    using Resizer = Result<Image> (*)(ImageView, std::size_t, std::size_t);
    constexpr std::array<Resizer, 4> resizers{resizeBox, resizeBilinear, resizeMitchell,
                                              resizeLanczos3};
    if (run("resize")) {
        Image blackWhite{2, 1, {{0, 0, 0, 1}, {1, 1, 1, 1}}};
        // Closed-form two-pixel weights at quarter centers, followed by the sRGB transfer function.
        const std::array<std::array<double, 4>, 4> oracle{
            {{{0, 0, 1, 1}},
             {{0, .5370987304831942, .8808250210902997, 1}},
             {{0, .533785321943214, .8825615701682809, 1}},
             {{0, .519844404307731, .8896719764695399, 1}}}};
        for (std::size_t i = 0; i < resizers.size(); ++i) {
            const auto resized = resizers[i](blackWhite.view(), 4, 1);
            check(resized && resized->width == 4 && resized->height == 1,
                  "resampling oracle dimensions");
            if (resized)
                for (std::size_t x = 0; x < 4; ++x)
                    check(near(resized->pixels[x].r, oracle[i][x]) &&
                              resized->pixels[x].r == resized->pixels[x].g &&
                              resized->pixels[x].g == resized->pixels[x].b &&
                              resized->pixels[x].a == 1,
                          "resampling kernel oracle");
            const auto down = resizers[i](blackWhite.view(), 1, 1);
            check(down && near(down->pixels[0].r, .7353569830524495), "linear-light mean oracle");
            const auto identity = resizers[i](image.view(), image.width, image.height);
            check(identity && identity->pixels == original &&
                      identity->pixels.data() != image.pixels.data(),
                  "resize identity detached exact copy");
            Image constant{7, 5, std::vector<Rgba>(35, {.2F, .4F, .7F, .3F})};
            for (auto size : {std::pair{1U, 1U}, std::pair{13U, 9U}, std::pair{3U, 2U}}) {
                const auto output = resizers[i](constant.view(), size.first, size.second);
                check(output && finiteNormalized(*output), "constant resize valid");
                if (output)
                    for (auto p : output->pixels)
                        check(near(p.r, .2F) && near(p.g, .4F) && near(p.b, .7F) && near(p.a, .3F),
                              "constant signal and alpha conservation");
            }
            Image edge{2, 1, {{1, 0, 0, 0}, {0, .6F, 1, .4F}}};
            Image changed = edge;
            changed.pixels[0] = {0, 1, 1, 0};
            const auto a = resizers[i](edge.view(), 7, 3), b = resizers[i](changed.view(), 7, 3);
            check(a && b && a->pixels == b->pixels, "hidden RGB isolated from resize");
            if (a)
                for (auto p : a->pixels) {
                    if (p.a == 0)
                        check(p == Rgba{0, 0, 0, 0}, "new invisible resized pixel is black");
                    else
                        check(p.r == 0 && near(p.g, .6F) && near(p.b, 1),
                              "premultiplied edge keeps visible color");
                }
            const auto average = resizers[i](edge.view(), 1, 1);
            check(average && near(average->pixels[0].a, .2) && average->pixels[0].r == 0 &&
                      near(average->pixels[0].g, .6F),
                  "transparent-edge downsample alpha oracle");
            Image invisible{2, 1, {{1, 0, 0, 0}, {0, 0, 1, 0}}};
            const auto hidden = resizers[i](invisible.view(), 5, 1);
            check(hidden && std::all_of(hidden->pixels.begin(), hidden->pixels.end(),
                                        [](Rgba p) { return p == Rgba{0, 0, 0, 0}; }),
                  "fully transparent resized output has no hidden color");
            const auto vertical = resizers[i](Image{1, 2, blackWhite.pixels}.view(), 1, 4);
            check(vertical && resized && vertical->pixels == resized->pixels,
                  "vertical and horizontal separable symmetry");
        }
    }
    if (run("invalid")) {
        const auto nan = std::numeric_limits<double>::quiet_NaN();
        check(!applyLightcraftLook(image.view(), static_cast<LightcraftLook>(999), 0) &&
                  !applyLightcraftPreset(image.view(), static_cast<LightcraftPreset>(999), 0),
              "image recipe selectors reject unknown enums at zero amount");
        check(!applyLightcraftLook(image.view(), LightcraftLook::Color, nan) &&
                  !applyLightcraftPreset(image.view(), LightcraftPreset::WarmGlow, nan),
              "image recipe selectors validate amount");
        check(!applyLightcraftLook(ImageView{}, LightcraftLook::Color, 0) &&
                  !applyLightcraftPreset(ImageView{}, LightcraftPreset::WarmGlow, 0),
              "image recipe selectors validate zero-amount input");
        check(!lightcraftLookOptions(static_cast<LightcraftLook>(999)) &&
                  !lightcraftLookOptions(static_cast<LightcraftLook>(999), 0),
              "unknown look rejected at all amounts");
        check(!lightcraftPresetOptions(static_cast<LightcraftPreset>(999)) &&
                  !lightcraftPresetOptions(static_cast<LightcraftPreset>(999), 0),
              "unknown preset rejected at all amounts");
        for (double amount : {-1., 2.01, nan, std::numeric_limits<double>::infinity()}) {
            check(!lightcraftLookOptions(LightcraftLook::Color, amount),
                  "look invalid amount rejected");
            check(!lightcraftPresetOptions(LightcraftPreset::WarmGlow, amount),
                  "preset invalid amount rejected");
        }
        Image invalid = image;
        invalid.pixels[1].r = std::numeric_limits<float>::quiet_NaN();
        for (const auto &fixture : looks)
            check(!fixture.operation(invalid.view(), 0), "zero look validates pixels");
        for (const auto &fixture : presets)
            check(!fixture.operation(invalid.view(), 0), "zero preset validates pixels");
        for (auto resizer : resizers) {
            check(!resizer(image.view(), 0, 1) && !resizer(image.view(), 1, 0),
                  "zero resize dimensions rejected");
            check(!resizer(image.view(), std::numeric_limits<std::size_t>::max(), 2),
                  "resize size overflow rejected");
            check(!resizer(image.view(), 65537, 1) && !resizer(image.view(), 65536, 65536),
                  "axis and output pixel bounds");
            check(!resizer(ImageView{1, 65536, {}}, 65536, 1),
                  "intermediate pixel bound without allocation");
            check(!resizer(ImageView{2, 2, std::span<const Rgba>(image.pixels.data(), 3)}, 2, 2),
                  "short source buffer rejected");
            check(!resizer(invalid.view(), invalid.width, invalid.height),
                  "identity resize validates pixels");
            check(!resizer(ImageView{0, 1, {}}, 1, 1), "source dimensions validated");
        }
    }
    check(image.pixels == original, "all operations preserve borrowed source buffer");
    check(group == "all" || group == "looks" || group == "presets" || group == "resize" ||
              group == "invalid",
          "unknown test group");
    if (!failures)
        std::cout << "LightCraft " << group << ": passed\n";
    return failures ? 1 : 0;
}
