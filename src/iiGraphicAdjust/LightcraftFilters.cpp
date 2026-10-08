// Algorithms and recipes adapted from LightCraft; see third_party/lightcraft/LICENSE-MIT.
#include "Internal.h"
#include <array>
#include <iiGraphicAdjust/LightcraftFilters.h>
#include <numbers>

namespace iiGraphicAdjust {
namespace {
Error invalid(const char *message) {
    return {ErrorCode::InvalidParameter, message};
}
void fade(CurveChannel &curve, double black, double white, double amount) {
    const auto b = std::clamp(black * amount, 0., .45);
    const auto w = std::clamp(1 - (1 - white) * amount, .55, 1.);
    for (auto &point : curve.points)
        point.output = b + point.output * (w - b);
}
} // namespace

Result<DevelopOptions> lightcraftLookOptions(LightcraftLook look, double amount) {
    if (!detail::inRange(amount, 0, 2))
        return std::unexpected(invalid("LightCraft look amount must be [0,2]"));
    auto allocated = detail::allocated([&]() -> Result<DevelopOptions> {
        DevelopOptions options;
        if (amount == 0) {
            switch (look) {
            case LightcraftLook::Color:
            case LightcraftLook::Neutral:
            case LightcraftLook::Vivid:
            case LightcraftLook::Landscape:
            case LightcraftLook::Portrait:
            case LightcraftLook::Mono:
            case LightcraftLook::FilmWarmPrint:
            case LightcraftLook::FilmCoolFade:
            case LightcraftLook::FilmGoldenHour:
            case LightcraftLook::FilmFadedSlide:
            case LightcraftLook::CineTealAmber:
            case LightcraftLook::CineNightBlue:
            case LightcraftLook::CineDesertHeat:
            case LightcraftLook::CineNeonDusk:
            case LightcraftLook::MutedMatteSoft:
            case LightcraftLook::MutedBleached:
            case LightcraftLook::MutedPastelHaze:
            case LightcraftLook::MutedQuietGreen:
            case LightcraftLook::BwMonoRich:
            case LightcraftLook::BwRedFilter:
            case LightcraftLook::BwSoft:
            case LightcraftLook::BwSepia:
                return options;
            default:
                return std::unexpected(invalid("Unknown LightCraft look"));
            }
        }
        switch (look) {
        case LightcraftLook::Color: // lc.color
            break;
        case LightcraftLook::Neutral: // lc.neutral
            options.light.contrast = std::clamp(-18.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-10.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-10.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(8.0 * amount, -100., 100.);
            break;
        case LightcraftLook::Vivid: // lc.vivid
            options.light.contrast = std::clamp(16.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(14.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(14.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-6.0 * amount, -100., 100.);
            break;
        case LightcraftLook::Landscape: // lc.landscape
            options.light.contrast = std::clamp(10.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(10.0 * amount, -100., 100.);
            options.color.mixer[3].saturation = std::clamp(14.0 * amount, -100., 100.);
            options.color.mixer[4].saturation = std::clamp(10.0 * amount, -100., 100.);
            options.color.mixer[5].saturation = std::clamp(14.0 * amount, -100., 100.);
            options.color.mixer[5].luminance = std::clamp(-6.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(6.0 * amount, -100., 100.);
            break;
        case LightcraftLook::Portrait: // lc.portrait
            options.light.contrast = std::clamp(-6.0 * amount, -100., 100.);
            options.color.mixer[1].saturation = std::clamp(-8.0 * amount, -100., 100.);
            options.color.mixer[1].luminance = std::clamp(6.0 * amount, -100., 100.);
            options.color.mixer[0].saturation = std::clamp(-5.0 * amount, -100., 100.);
            options.effects.texture = std::clamp(-6.0 * amount, -100., 100.);
            break;
        case LightcraftLook::Mono: // lc.mono
            options.light.contrast = std::clamp(8.0 * amount, -100., 100.);
            options.color.blackAndWhite = amount > 0;
            break;
        case LightcraftLook::FilmWarmPrint: // lc.film.warm-print
            options.light.contrast = std::clamp(6.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-4.0 * amount, -100., 100.);
            options.color.mixer[1].luminance = std::clamp(4.0 * amount, -100., 100.);
            fade(options.curve.master, 0.04, 0.97, amount);
            options.color.grading.highlights = {40.0, std::min(100., 18.0 * amount),
                                                std::clamp(0.0 * amount, -100., 100.)};
            options.color.grading.shadows = {25.0, std::min(100., 8.0 * amount),
                                             std::clamp(0.0 * amount, -100., 100.)};
            break;
        case LightcraftLook::FilmCoolFade: // lc.film.cool-fade
            options.light.contrast = std::clamp(-8.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-10.0 * amount, -100., 100.);
            fade(options.curve.master, 0.07, 0.95, amount);
            options.color.grading.shadows = {210.0, std::min(100., 16.0 * amount),
                                             std::clamp(0.0 * amount, -100., 100.)};
            options.color.grading.highlights = {190.0, std::min(100., 6.0 * amount),
                                                std::clamp(0.0 * amount, -100., 100.)};
            break;
        case LightcraftLook::FilmGoldenHour: // lc.film.golden-hour
            options.color.vibrance = std::clamp(10.0 * amount, -100., 100.);
            options.curve.highlights = std::clamp(-8.0 * amount, -100., 100.);
            options.color.mixer[1].saturation = std::clamp(8.0 * amount, -100., 100.);
            options.color.mixer[2].hue = std::clamp(-6.0 * amount, -100., 100.);
            options.color.grading.global = {45.0, std::min(100., 10.0 * amount),
                                            std::clamp(0.0 * amount, -100., 100.)};
            options.color.grading.highlights = {35.0, std::min(100., 20.0 * amount),
                                                std::clamp(0.0 * amount, -100., 100.)};
            break;
        case LightcraftLook::FilmFadedSlide: // lc.film.faded-slide
            options.light.contrast = std::clamp(14.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(8.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(6.0 * amount, -100., 100.);
            options.curve.shadows = std::clamp(10.0 * amount, -100., 100.);
            fade(options.curve.blue, 0.06, 1.0, amount);
            fade(options.curve.red, 0.0, 0.97, amount);
            break;
        case LightcraftLook::CineTealAmber: // lc.cine.teal-amber
            options.light.contrast = std::clamp(10.0 * amount, -100., 100.);
            options.color.mixer[1].saturation = std::clamp(6.0 * amount, -100., 100.);
            options.color.mixer[3].hue = std::clamp(20.0 * amount, -100., 100.);
            options.color.mixer[3].saturation = std::clamp(-20.0 * amount, -100., 100.);
            options.color.mixer[5].hue = std::clamp(-8.0 * amount, -100., 100.);
            options.color.grading.shadows = {195.0, std::min(100., 30.0 * amount),
                                             std::clamp(0.0 * amount, -100., 100.)};
            options.color.grading.highlights = {38.0, std::min(100., 24.0 * amount),
                                                std::clamp(0.0 * amount, -100., 100.)};
            break;
        case LightcraftLook::CineNightBlue: // lc.cine.night-blue
            options.light.contrast = std::clamp(12.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-12.0 * amount, -100., 100.);
            options.curve.darks = std::clamp(-8.0 * amount, -100., 100.);
            options.color.grading.shadows = {220.0, std::min(100., 28.0 * amount),
                                             std::clamp(-4.0 * amount, -100., 100.)};
            options.color.grading.midtones = {215.0, std::min(100., 10.0 * amount),
                                              std::clamp(0.0 * amount, -100., 100.)};
            break;
        case LightcraftLook::CineDesertHeat: // lc.cine.desert-heat
            options.light.contrast = std::clamp(8.0 * amount, -100., 100.);
            options.curve.lights = std::clamp(6.0 * amount, -100., 100.);
            options.color.mixer[5].saturation = std::clamp(-25.0 * amount, -100., 100.);
            options.color.mixer[4].saturation = std::clamp(-20.0 * amount, -100., 100.);
            options.color.grading.global = {32.0, std::min(100., 16.0 * amount),
                                            std::clamp(0.0 * amount, -100., 100.)};
            options.color.grading.highlights = {48.0, std::min(100., 12.0 * amount),
                                                std::clamp(0.0 * amount, -100., 100.)};
            break;
        case LightcraftLook::CineNeonDusk: // lc.cine.neon-dusk
            options.light.contrast = std::clamp(10.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(18.0 * amount, -100., 100.);
            options.color.mixer[7].saturation = std::clamp(12.0 * amount, -100., 100.);
            options.color.mixer[6].saturation = std::clamp(12.0 * amount, -100., 100.);
            options.color.grading.shadows = {265.0, std::min(100., 22.0 * amount),
                                             std::clamp(0.0 * amount, -100., 100.)};
            options.color.grading.highlights = {330.0, std::min(100., 14.0 * amount),
                                                std::clamp(0.0 * amount, -100., 100.)};
            break;
        case LightcraftLook::MutedMatteSoft: // lc.muted.matte-soft
            options.light.contrast = std::clamp(-12.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-14.0 * amount, -100., 100.);
            options.curve.highlights = std::clamp(-6.0 * amount, -100., 100.);
            fade(options.curve.master, 0.08, 0.96, amount);
            break;
        case LightcraftLook::MutedBleached: // lc.muted.bleached
            options.light.contrast = std::clamp(22.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-38.0 * amount, -100., 100.);
            options.curve.lights = std::clamp(6.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(8.0 * amount, -100., 100.);
            break;
        case LightcraftLook::MutedPastelHaze: // lc.muted.pastel-haze
            options.light.contrast = std::clamp(-18.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-20.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(12.0 * amount, -100., 100.);
            fade(options.curve.master, 0.1, 1.0, amount);
            options.color.grading.highlights = {330.0, std::min(100., 6.0 * amount),
                                                std::clamp(0.0 * amount, -100., 100.)};
            options.color.grading.shadows = {200.0, std::min(100., 6.0 * amount),
                                             std::clamp(0.0 * amount, -100., 100.)};
            break;
        case LightcraftLook::MutedQuietGreen: // lc.muted.quiet-green
            options.light.contrast = std::clamp(-4.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-8.0 * amount, -100., 100.);
            options.color.mixer[3].saturation = std::clamp(-30.0 * amount, -100., 100.);
            options.color.mixer[3].hue = std::clamp(-10.0 * amount, -100., 100.);
            options.color.mixer[2].saturation = std::clamp(-18.0 * amount, -100., 100.);
            options.color.mixer[4].saturation = std::clamp(-15.0 * amount, -100., 100.);
            fade(options.curve.master, 0.04, 1.0, amount);
            break;
        case LightcraftLook::BwMonoRich: // lc.bw.mono-rich
            options.light.contrast = std::clamp(18.0 * amount, -100., 100.);
            options.curve.darks = std::clamp(-8.0 * amount, -100., 100.);
            options.curve.lights = std::clamp(6.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[0] = std::clamp(8.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[1] = std::clamp(10.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[5] = std::clamp(-12.0 * amount, -100., 100.);
            options.color.blackAndWhite = amount > 0;
            break;
        case LightcraftLook::BwRedFilter: // lc.bw.red-filter
            options.light.contrast = std::clamp(12.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[0] = std::clamp(40.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[1] = std::clamp(30.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[2] = std::clamp(10.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[3] = std::clamp(-20.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[4] = std::clamp(-30.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[5] = std::clamp(-45.0 * amount, -100., 100.);
            options.color.blackAndWhite = amount > 0;
            break;
        case LightcraftLook::BwSoft: // lc.bw.soft
            options.light.contrast = std::clamp(-14.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[1] = std::clamp(12.0 * amount, -100., 100.);
            fade(options.curve.master, 0.06, 0.97, amount);
            options.color.blackAndWhite = amount > 0;
            break;
        case LightcraftLook::BwSepia: // lc.bw.sepia
            options.light.contrast = std::clamp(6.0 * amount, -100., 100.);
            fade(options.curve.master, 0.04, 0.98, amount);
            options.color.grading.shadows = {28.0, std::min(100., 18.0 * amount),
                                             std::clamp(0.0 * amount, -100., 100.)};
            options.color.grading.highlights = {45.0, std::min(100., 14.0 * amount),
                                                std::clamp(0.0 * amount, -100., 100.)};
            options.color.blackAndWhite = amount > 0;
            break;
        default:
            return std::unexpected(invalid("Unknown LightCraft look"));
        }
        return options;
    });
    if (!allocated)
        return std::unexpected(allocated.error());
    return std::move(*allocated);
}

Result<DevelopOptions> lightcraftPresetOptions(LightcraftPreset preset, double amount) {
    if (!detail::inRange(amount, 0, 2))
        return std::unexpected(invalid("LightCraft preset amount must be [0,2]"));
    auto allocated = detail::allocated([&]() -> Result<DevelopOptions> {
        DevelopOptions options;
        switch (preset) {
        case LightcraftPreset::WarmGlow: // Warm Glow (Color)
            if (amount >= .5)
                options.whiteBalance.temperature = 6500 + (7200.0 - 6500) * amount;
            if (amount >= .5)
                options.whiteBalance.tint = 8.0 * amount;
            options.light.contrast = std::clamp(8.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(12.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(18.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::CoolMorning: // Cool Morning (Color)
            if (amount >= .5)
                options.whiteBalance.temperature = 6500 + (5600.0 - 6500) * amount;
            if (amount >= .5)
                options.whiteBalance.tint = -4.0 * amount;
            options.light.highlights = std::clamp(-20.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(10.0 * amount, -100., 100.);
            options.color.grading.shadows.hue = std::clamp(215.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(12.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::TealOrange: // Teal & Orange (Color)
            options.color.grading.shadows.hue = std::clamp(195.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(28.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(35.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(22.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.balance = std::clamp(0. + (10.0 - 0) * amount, -100., 100.);
            options.color.vibrance = std::clamp(12.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(14.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::VividPop: // Vivid Pop (Color)
            options.color.vibrance = std::clamp(40.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(8.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(18.0 * amount, -100., 100.);
            options.light.whites = std::clamp(10.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-10.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(12.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::SoftPastel: // Soft Pastel (Color)
            options.light.contrast = std::clamp(-25.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-30.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(35.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(20.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-18.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(10.0 * amount, -100., 100.);
            options.curve.shadows = std::clamp(25.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::FadedMatte: // Faded Matte (Film)
            options.curve.shadows = std::clamp(40.0 * amount, -100., 100.);
            options.curve.highlights = std::clamp(-15.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(-10.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-12.0 * amount, -100., 100.);
            options.grain.amount = std::clamp(0. + (18.0 - 0) * amount, 0., 100.);
            options.grain.size = std::clamp(25. + (25.0 - 25) * amount, 0., 100.);
            options.grain.roughness = std::clamp(50. + (50.0 - 50) * amount, 0., 100.);
            break;
        case LightcraftPreset::WarmFilm: // Warm Film (Film)
            if (amount >= .5)
                options.whiteBalance.temperature = 6500 + (6900.0 - 6500) * amount;
            if (amount >= .5)
                options.whiteBalance.tint = 5.0 * amount;
            options.curve.shadows = std::clamp(22.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(45.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(15.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.grain.amount = std::clamp(0. + (22.0 - 0) * amount, 0., 100.);
            options.grain.size = std::clamp(25. + (30.0 - 25) * amount, 0., 100.);
            options.grain.roughness = std::clamp(50. + (55.0 - 50) * amount, 0., 100.);
            options.vignette.amount = std::clamp(0. + (-12.0 - 0) * amount, -100., 100.);
            break;
        case LightcraftPreset::MutedFilm: // Muted Film (Film)
            options.color.saturation = std::clamp(-25.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(5.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(10.0 * amount, -100., 100.);
            options.curve.shadows = std::clamp(18.0 * amount, -100., 100.);
            options.grain.amount = std::clamp(0. + (15.0 - 0) * amount, 0., 100.);
            break;
        case LightcraftPreset::BwHighContrast: // High Contrast B&W (B&W)
            options.color.blackAndWhite = amount >= .5;
            options.light.contrast = std::clamp(45.0 * amount, -100., 100.);
            options.light.whites = std::clamp(20.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-25.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(20.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[5] = std::clamp(-30.0 * amount, -100., 100.);
            options.color.blackAndWhiteMix[0] = std::clamp(15.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::BwSoft: // Soft B&W (B&W)
            options.color.blackAndWhite = amount >= .5;
            options.light.contrast = std::clamp(-15.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(25.0 * amount, -100., 100.);
            options.curve.shadows = std::clamp(15.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::BwSelenium: // Selenium Tone (B&W)
            options.color.blackAndWhite = amount >= .5;
            options.light.contrast = std::clamp(20.0 * amount, -100., 100.);
            options.color.grading.shadows.hue = std::clamp(285.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(18.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(40.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(10.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::CrispLandscape: // Crisp Landscape (Landscape)
            options.light.highlights = std::clamp(-45.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(30.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(12.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(22.0 * amount, -100., 100.);
            options.effects.dehaze = std::clamp(12.0 * amount, -100., 100.);
            options.effects.texture = std::clamp(15.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(25.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::GoldenHour: // Golden Hour (Landscape)
            if (amount >= .5)
                options.whiteBalance.temperature = 6500 + (7600.0 - 6500) * amount;
            if (amount >= .5)
                options.whiteBalance.tint = 12.0 * amount;
            options.light.highlights = std::clamp(-35.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(20.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(38.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(25.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(5.0 * amount, -100., 100.);
            options.vignette.amount = std::clamp(0. + (-15.0 - 0) * amount, -100., 100.);
            break;
        case LightcraftPreset::BlueHour: // Blue Hour Boost (Landscape)
            if (amount >= .5)
                options.whiteBalance.temperature = 6500 + (5200.0 - 6500) * amount;
            if (amount >= .5)
                options.whiteBalance.tint = 6.0 * amount;
            options.light.shadows = std::clamp(25.0 * amount, -100., 100.);
            options.light.exposure = std::clamp(0.2 * amount, -5., 5.);
            options.color.mixer[5].hue = std::clamp(0.0 * amount, -100., 100.);
            options.color.mixer[5].saturation = std::clamp(25.0 * amount, -100., 100.);
            options.color.mixer[5].luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.mixer[6].hue = std::clamp(0.0 * amount, -100., 100.);
            options.color.mixer[6].saturation = std::clamp(15.0 * amount, -100., 100.);
            options.color.mixer[6].luminance = std::clamp(0.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::SoftSkin: // Soft Skin (Portrait)
            options.effects.texture = std::clamp(-25.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(-10.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(-5.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(15.0 * amount, -100., 100.);
            options.color.mixer[1].hue = std::clamp(0.0 * amount, -100., 100.);
            options.color.mixer[1].saturation = std::clamp(-8.0 * amount, -100., 100.);
            options.color.mixer[1].luminance = std::clamp(10.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::BrightAiry: // Bright & Airy (Portrait)
            options.light.exposure = std::clamp(0.45 * amount, -5., 5.);
            options.light.contrast = std::clamp(-15.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-40.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(40.0 * amount, -100., 100.);
            options.light.whites = std::clamp(15.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(8.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-8.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::Moody: // Moody (Style)
            options.light.exposure = std::clamp(-0.3 * amount, -5., 5.);
            options.light.contrast = std::clamp(20.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-30.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-15.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-20.0 * amount, -100., 100.);
            options.color.grading.shadows.hue = std::clamp(200.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(15.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(-5.0 * amount, -100., 100.);
            options.vignette.amount = std::clamp(0. + (-30.0 - 0) * amount, -100., 100.);
            break;
        case LightcraftPreset::Cinematic: // Cinematic (Style)
            options.curve.shadows = std::clamp(20.0 * amount, -100., 100.);
            options.curve.highlights = std::clamp(-10.0 * amount, -100., 100.);
            options.color.grading.shadows.hue = std::clamp(190.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(25.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(30.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(18.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(15.0 * amount, -100., 100.);
            options.vignette.amount = std::clamp(0. + (-20.0 - 0) * amount, -100., 100.);
            break;
        case LightcraftPreset::MoodyPortrait: // Moody Portrait (Portrait)
            options.light.exposure = std::clamp(-0.25 * amount, -5., 5.);
            options.light.contrast = std::clamp(20.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-30.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-12.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-15.0 * amount, -100., 100.);
            options.vignette.amount = std::clamp(0. + (-25.0 - 0) * amount, -100., 100.);
            options.vignette.midpoint = std::clamp(50. + (40.0 - 50) * amount, 0., 100.);
            options.color.mixer[1].saturation = std::clamp(6.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::GoldenGlow: // Golden Glow (Portrait)
            options.color.grading.highlights.hue = std::clamp(40.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(18.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.midtones.hue = std::clamp(35.0 * amount, 0., 360.);
            options.color.grading.midtones.saturation = std::clamp(8.0 * amount, 0., 100.);
            options.color.grading.midtones.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(15.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(-8.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::CleanStudio: // Clean Studio (Portrait)
            options.light.contrast = std::clamp(10.0 * amount, -100., 100.);
            options.light.whites = std::clamp(15.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-8.0 * amount, -100., 100.);
            options.effects.texture = std::clamp(-10.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(5.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::CrispVista: // Crisp Vista (Landscape)
            options.effects.dehaze = std::clamp(15.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(18.0 * amount, -100., 100.);
            options.effects.texture = std::clamp(12.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(12.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-25.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(18.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(20.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::DeepSky: // Deep Sky (Landscape)
            options.color.mixer[5].luminance = std::clamp(-25.0 * amount, -100., 100.);
            options.color.mixer[5].saturation = std::clamp(15.0 * amount, -100., 100.);
            options.color.mixer[4].luminance = std::clamp(-10.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-30.0 * amount, -100., 100.);
            options.effects.dehaze = std::clamp(10.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::LushGreens: // Lush Greens (Landscape)
            options.color.mixer[3].hue = std::clamp(12.0 * amount, -100., 100.);
            options.color.mixer[3].saturation = std::clamp(18.0 * amount, -100., 100.);
            options.color.mixer[3].luminance = std::clamp(8.0 * amount, -100., 100.);
            options.color.mixer[2].hue = std::clamp(10.0 * amount, -100., 100.);
            options.color.mixer[2].saturation = std::clamp(10.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(12.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(8.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::DesertWarmth: // Desert Warmth (Landscape)
            options.color.mixer[1].saturation = std::clamp(15.0 * amount, -100., 100.);
            options.color.mixer[1].luminance = std::clamp(5.0 * amount, -100., 100.);
            options.color.mixer[2].hue = std::clamp(-8.0 * amount, -100., 100.);
            options.color.mixer[2].saturation = std::clamp(10.0 * amount, -100., 100.);
            options.color.mixer[5].saturation = std::clamp(-10.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(38.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(12.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(10.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::MistyMorning: // Misty Morning (Landscape)
            options.effects.dehaze = std::clamp(-18.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(-12.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(-18.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-10.0 * amount, -100., 100.);
            options.color.grading.shadows.hue = std::clamp(210.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(10.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-10.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::GrittyStreet: // Gritty Street (Urban)
            options.effects.clarity = std::clamp(35.0 * amount, -100., 100.);
            options.effects.texture = std::clamp(20.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(25.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-15.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-30.0 * amount, -100., 100.);
            options.vignette.amount = std::clamp(0. + (-18.0 - 0) * amount, -100., 100.);
            break;
        case LightcraftPreset::NeonNight: // Neon Night (Urban)
            options.color.mixer[7].saturation = std::clamp(20.0 * amount, -100., 100.);
            options.color.mixer[6].saturation = std::clamp(18.0 * amount, -100., 100.);
            options.color.mixer[5].saturation = std::clamp(12.0 * amount, -100., 100.);
            options.color.mixer[4].saturation = std::clamp(15.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(18.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-12.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(15.0 * amount, -100., 100.);
            options.color.grading.shadows.hue = std::clamp(250.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(15.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::ConcreteCool: // Concrete Cool (Urban)
            options.color.grading.shadows.hue = std::clamp(205.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(14.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(200.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(6.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-20.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(12.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(15.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::FadedUrban: // Faded Urban (Urban)
            options.curve.shadows = std::clamp(30.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-22.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(-8.0 * amount, -100., 100.);
            options.grain.amount = std::clamp(0. + (12.0 - 0) * amount, 0., 100.);
            options.grain.size = std::clamp(25. + (25.0 - 25) * amount, 0., 100.);
            options.grain.roughness = std::clamp(50. + (45.0 - 50) * amount, 0., 100.);
            break;
        case LightcraftPreset::FreshBright: // Fresh & Bright (Food)
            options.light.exposure = std::clamp(0.25 * amount, -5., 5.);
            options.light.shadows = std::clamp(20.0 * amount, -100., 100.);
            options.light.whites = std::clamp(10.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(22.0 * amount, -100., 100.);
            options.effects.texture = std::clamp(15.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::WarmTable: // Warm Table (Food)
            options.color.grading.midtones.hue = std::clamp(32.0 * amount, 0., 360.);
            options.color.grading.midtones.saturation = std::clamp(10.0 * amount, 0., 100.);
            options.color.grading.midtones.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.mixer[1].saturation = std::clamp(10.0 * amount, -100., 100.);
            options.color.mixer[0].saturation = std::clamp(8.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(8.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::DarkMoodyFood: // Dark & Moody (Food)
            options.light.exposure = std::clamp(-0.3 * amount, -5., 5.);
            options.light.contrast = std::clamp(22.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-20.0 * amount, -100., 100.);
            options.light.blacks = std::clamp(-15.0 * amount, -100., 100.);
            options.effects.texture = std::clamp(18.0 * amount, -100., 100.);
            options.vignette.amount = std::clamp(0. + (-22.0 - 0) * amount, -100., 100.);
            break;
        case LightcraftPreset::AutumnGold: // Autumn Gold (Seasons)
            options.color.mixer[3].hue = std::clamp(-30.0 * amount, -100., 100.);
            options.color.mixer[3].saturation = std::clamp(-10.0 * amount, -100., 100.);
            options.color.mixer[2].hue = std::clamp(-15.0 * amount, -100., 100.);
            options.color.mixer[2].saturation = std::clamp(15.0 * amount, -100., 100.);
            options.color.mixer[1].saturation = std::clamp(18.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(40.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(10.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::WinterBlue: // Winter Blue (Seasons)
            options.color.grading.shadows.hue = std::clamp(215.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(18.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(205.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(8.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-12.0 * amount, -100., 100.);
            options.light.whites = std::clamp(12.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::SpringFresh: // Spring Fresh (Seasons)
            options.color.mixer[3].hue = std::clamp(10.0 * amount, -100., 100.);
            options.color.mixer[3].saturation = std::clamp(12.0 * amount, -100., 100.);
            options.color.mixer[3].luminance = std::clamp(10.0 * amount, -100., 100.);
            options.color.mixer[7].saturation = std::clamp(10.0 * amount, -100., 100.);
            options.light.shadows = std::clamp(15.0 * amount, -100., 100.);
            options.color.vibrance = std::clamp(15.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::SummerHaze: // Summer Haze (Seasons)
            options.effects.dehaze = std::clamp(-12.0 * amount, -100., 100.);
            options.curve.shadows = std::clamp(18.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(45.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(14.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(-10.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::Instant70s: // Instant ’70s (Vintage)
            options.curve.shadows = std::clamp(28.0 * amount, -100., 100.);
            options.curve.highlights = std::clamp(-18.0 * amount, -100., 100.);
            options.color.grading.shadows.hue = std::clamp(170.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(14.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(45.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(18.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(-10.0 * amount, -100., 100.);
            options.vignette.amount = std::clamp(0. + (-15.0 - 0) * amount, -100., 100.);
            options.grain.amount = std::clamp(0. + (20.0 - 0) * amount, 0., 100.);
            options.grain.size = std::clamp(25. + (30.0 - 25) * amount, 0., 100.);
            options.grain.roughness = std::clamp(50. + (55.0 - 50) * amount, 0., 100.);
            break;
        case LightcraftPreset::CrossProcess: // Cross Process (Vintage)
            options.color.grading.shadows.hue = std::clamp(230.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(25.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(60.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(25.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(22.0 * amount, -100., 100.);
            options.color.saturation = std::clamp(10.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::BleachBypass: // Bleach Bypass (Vintage)
            options.color.saturation = std::clamp(-40.0 * amount, -100., 100.);
            options.light.contrast = std::clamp(35.0 * amount, -100., 100.);
            options.light.highlights = std::clamp(-15.0 * amount, -100., 100.);
            options.effects.clarity = std::clamp(15.0 * amount, -100., 100.);
            break;
        case LightcraftPreset::BwSepia: // Sepia Tone (B&W)
            options.color.blackAndWhite = amount >= .5;
            options.color.grading.shadows.hue = std::clamp(32.0 * amount, 0., 360.);
            options.color.grading.shadows.saturation = std::clamp(22.0 * amount, 0., 100.);
            options.color.grading.shadows.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.color.grading.highlights.hue = std::clamp(42.0 * amount, 0., 360.);
            options.color.grading.highlights.saturation = std::clamp(18.0 * amount, 0., 100.);
            options.color.grading.highlights.luminance = std::clamp(0.0 * amount, -100., 100.);
            options.curve.shadows = std::clamp(10.0 * amount, -100., 100.);
            break;
        default:
            return std::unexpected(invalid("Unknown LightCraft preset"));
        }
        return options;
    });
    if (!allocated)
        return std::unexpected(allocated.error());
    return std::move(*allocated);
}

Result<Image> applyLightcraftLook(ImageView image, LightcraftLook look, double amount) {
    if (auto checked = detail::validate(image); !checked)
        return std::unexpected(checked.error());
    auto options = lightcraftLookOptions(look, amount);
    if (!options)
        return std::unexpected(options.error());
    return develop(image, *options);
}

Result<Image> applyLightcraftPreset(ImageView image, LightcraftPreset preset, double amount) {
    if (auto checked = detail::validate(image); !checked)
        return std::unexpected(checked.error());
    auto options = lightcraftPresetOptions(preset, amount);
    if (!options)
        return std::unexpected(options.error());
    return develop(image, *options);
}

Result<Image> lightcraftLookColor(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::Color, amount);
}
Result<Image> lightcraftLookNeutral(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::Neutral, amount);
}
Result<Image> lightcraftLookVivid(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::Vivid, amount);
}
Result<Image> lightcraftLookLandscape(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::Landscape, amount);
}
Result<Image> lightcraftLookPortrait(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::Portrait, amount);
}
Result<Image> lightcraftLookMono(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::Mono, amount);
}
Result<Image> lightcraftLookFilmWarmPrint(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::FilmWarmPrint, amount);
}
Result<Image> lightcraftLookFilmCoolFade(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::FilmCoolFade, amount);
}
Result<Image> lightcraftLookFilmGoldenHour(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::FilmGoldenHour, amount);
}
Result<Image> lightcraftLookFilmFadedSlide(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::FilmFadedSlide, amount);
}
Result<Image> lightcraftLookCineTealAmber(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::CineTealAmber, amount);
}
Result<Image> lightcraftLookCineNightBlue(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::CineNightBlue, amount);
}
Result<Image> lightcraftLookCineDesertHeat(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::CineDesertHeat, amount);
}
Result<Image> lightcraftLookCineNeonDusk(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::CineNeonDusk, amount);
}
Result<Image> lightcraftLookMutedMatteSoft(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::MutedMatteSoft, amount);
}
Result<Image> lightcraftLookMutedBleached(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::MutedBleached, amount);
}
Result<Image> lightcraftLookMutedPastelHaze(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::MutedPastelHaze, amount);
}
Result<Image> lightcraftLookMutedQuietGreen(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::MutedQuietGreen, amount);
}
Result<Image> lightcraftLookBwMonoRich(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::BwMonoRich, amount);
}
Result<Image> lightcraftLookBwRedFilter(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::BwRedFilter, amount);
}
Result<Image> lightcraftLookBwSoft(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::BwSoft, amount);
}
Result<Image> lightcraftLookBwSepia(ImageView image, double amount) {
    return applyLightcraftLook(image, LightcraftLook::BwSepia, amount);
}

Result<Image> lightcraftPresetWarmGlow(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::WarmGlow, amount);
}
Result<Image> lightcraftPresetCoolMorning(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::CoolMorning, amount);
}
Result<Image> lightcraftPresetTealOrange(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::TealOrange, amount);
}
Result<Image> lightcraftPresetVividPop(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::VividPop, amount);
}
Result<Image> lightcraftPresetSoftPastel(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::SoftPastel, amount);
}
Result<Image> lightcraftPresetFadedMatte(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::FadedMatte, amount);
}
Result<Image> lightcraftPresetWarmFilm(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::WarmFilm, amount);
}
Result<Image> lightcraftPresetMutedFilm(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::MutedFilm, amount);
}
Result<Image> lightcraftPresetBwHighContrast(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::BwHighContrast, amount);
}
Result<Image> lightcraftPresetBwSoft(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::BwSoft, amount);
}
Result<Image> lightcraftPresetBwSelenium(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::BwSelenium, amount);
}
Result<Image> lightcraftPresetCrispLandscape(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::CrispLandscape, amount);
}
Result<Image> lightcraftPresetGoldenHour(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::GoldenHour, amount);
}
Result<Image> lightcraftPresetBlueHour(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::BlueHour, amount);
}
Result<Image> lightcraftPresetSoftSkin(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::SoftSkin, amount);
}
Result<Image> lightcraftPresetBrightAiry(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::BrightAiry, amount);
}
Result<Image> lightcraftPresetMoody(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::Moody, amount);
}
Result<Image> lightcraftPresetCinematic(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::Cinematic, amount);
}
Result<Image> lightcraftPresetMoodyPortrait(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::MoodyPortrait, amount);
}
Result<Image> lightcraftPresetGoldenGlow(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::GoldenGlow, amount);
}
Result<Image> lightcraftPresetCleanStudio(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::CleanStudio, amount);
}
Result<Image> lightcraftPresetCrispVista(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::CrispVista, amount);
}
Result<Image> lightcraftPresetDeepSky(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::DeepSky, amount);
}
Result<Image> lightcraftPresetLushGreens(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::LushGreens, amount);
}
Result<Image> lightcraftPresetDesertWarmth(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::DesertWarmth, amount);
}
Result<Image> lightcraftPresetMistyMorning(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::MistyMorning, amount);
}
Result<Image> lightcraftPresetGrittyStreet(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::GrittyStreet, amount);
}
Result<Image> lightcraftPresetNeonNight(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::NeonNight, amount);
}
Result<Image> lightcraftPresetConcreteCool(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::ConcreteCool, amount);
}
Result<Image> lightcraftPresetFadedUrban(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::FadedUrban, amount);
}
Result<Image> lightcraftPresetFreshBright(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::FreshBright, amount);
}
Result<Image> lightcraftPresetWarmTable(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::WarmTable, amount);
}
Result<Image> lightcraftPresetDarkMoodyFood(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::DarkMoodyFood, amount);
}
Result<Image> lightcraftPresetAutumnGold(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::AutumnGold, amount);
}
Result<Image> lightcraftPresetWinterBlue(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::WinterBlue, amount);
}
Result<Image> lightcraftPresetSpringFresh(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::SpringFresh, amount);
}
Result<Image> lightcraftPresetSummerHaze(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::SummerHaze, amount);
}
Result<Image> lightcraftPresetInstant70s(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::Instant70s, amount);
}
Result<Image> lightcraftPresetCrossProcess(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::CrossProcess, amount);
}
Result<Image> lightcraftPresetBleachBypass(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::BleachBypass, amount);
}
Result<Image> lightcraftPresetBwSepia(ImageView image, double amount) {
    return applyLightcraftPreset(image, LightcraftPreset::BwSepia, amount);
}

namespace {
enum class ResizeKernel { Box, Bilinear, Mitchell, Lanczos3 };
constexpr std::size_t maxResizeAxis = 65536;
constexpr std::size_t maxResizePixels = 64 * 1024 * 1024;
constexpr std::size_t maxAxisTaps = 1024 * 1024;
using LinearRgba = std::array<double, 4>;
struct Contributions {
    std::size_t first;
    std::vector<double> weights;
};

double support(ResizeKernel kernel) {
    switch (kernel) {
    case ResizeKernel::Box:
        return .5;
    case ResizeKernel::Bilinear:
        return 1;
    case ResizeKernel::Mitchell:
        return 2;
    case ResizeKernel::Lanczos3:
        return 3;
    }
    return 0;
}

double kernelValue(ResizeKernel kernel, double x) {
    x = std::abs(x);
    switch (kernel) {
    case ResizeKernel::Box:
        return x <= .5 ? 1 : 0;
    case ResizeKernel::Bilinear:
        return std::max(0., 1 - x);
    case ResizeKernel::Mitchell:
        // Mitchell-Netravali, B=C=1/3.
        if (x < 1)
            return (7 * x * x * x - 12 * x * x + 16. / 3) / 6;
        if (x < 2)
            return (-7. / 3 * x * x * x + 12 * x * x - 20 * x + 32. / 3) / 6;
        return 0;
    case ResizeKernel::Lanczos3:
        if (x < 1e-6)
            return 1;
        if (x >= 3)
            return 0;
        const auto px = std::numbers::pi * x;
        return 3 * std::sin(px) * std::sin(px / 3) / (px * px);
    }
    return 0;
}

Result<std::vector<Contributions>> contributions(std::size_t source, std::size_t destination,
                                                 ResizeKernel kernel) {
    const auto scale = double(source) / double(destination);
    const auto filterScale = std::max(1., scale);
    const auto radius = support(kernel) * filterScale;
    std::vector<Contributions> table;
    table.reserve(destination);
    std::size_t total = 0;
    for (std::size_t i = 0; i < destination; ++i) {
        const auto center = (double(i) + .5) * scale;
        const auto lo = static_cast<std::size_t>(std::max(0., std::floor(center - radius)));
        const auto hi =
            static_cast<std::size_t>(std::min(double(source), std::ceil(center + radius)));
        if (lo >= hi || hi - lo > maxAxisTaps - total)
            return std::unexpected(invalid("Resampling contributions exceed bounded storage"));
        total += hi - lo;
        Contributions row{lo, std::vector<double>(hi - lo)};
        double sum = 0;
        for (std::size_t j = lo; j < hi; ++j) {
            const auto weight = kernelValue(kernel, (double(j) + .5 - center) / filterScale);
            row.weights[j - lo] = weight;
            sum += weight;
        }
        if (std::abs(sum) > 1e-8) {
            for (auto &weight : row.weights)
                weight /= sum;
        } else {
            for (auto &weight : row.weights)
                weight = 1. / double(row.weights.size());
        }
        table.push_back(std::move(row));
    }
    return table;
}

Result<Image> resize(ImageView image, std::size_t width, std::size_t height, ResizeKernel kernel) {
    const auto inputCount = detail::pixelCount(image.width, image.height);
    if (!inputCount)
        return std::unexpected(inputCount.error());
    const auto outputCount = detail::pixelCount(width, height);
    if (!outputCount)
        return std::unexpected(outputCount.error());
    const auto temporaryCount = detail::pixelCount(width, image.height);
    if (!temporaryCount)
        return std::unexpected(temporaryCount.error());
    if (image.width > maxResizeAxis || image.height > maxResizeAxis || width > maxResizeAxis ||
        height > maxResizeAxis || *inputCount > maxResizePixels || *outputCount > maxResizePixels ||
        *temporaryCount > maxResizePixels)
        return std::unexpected(Error{ErrorCode::InvalidDimensions,
                                     "Resampling axis or pixel count exceeds bounded storage"});
    if (auto checked = detail::validate(image); !checked)
        return std::unexpected(checked.error());
    auto allocated = detail::allocated([&]() -> Result<Image> {
        if (image.width == width && image.height == height)
            return detail::copy(image);
        auto horizontal = contributions(image.width, width, kernel);
        if (!horizontal)
            return std::unexpected(horizontal.error());
        auto vertical = contributions(image.height, height, kernel);
        if (!vertical)
            return std::unexpected(vertical.error());
        std::vector<LinearRgba> temporary(*temporaryCount);
        std::vector<LinearRgba> sourceRow(image.width);
        for (std::size_t y = 0; y < image.height; ++y) {
            for (std::size_t x = 0; x < image.width; ++x) {
                const auto p = image.pixels[y * image.width + x];
                sourceRow[x] = {detail::linear(p.r) * p.a, detail::linear(p.g) * p.a,
                                detail::linear(p.b) * p.a, p.a};
            }
            for (std::size_t x = 0; x < width; ++x) {
                const auto &taps = (*horizontal)[x];
                LinearRgba value{};
                for (std::size_t k = 0; k < taps.weights.size(); ++k)
                    for (std::size_t c = 0; c < 4; ++c)
                        value[c] += sourceRow[taps.first + k][c] * taps.weights[k];
                temporary[y * width + x] = value;
            }
        }
        Image output{width, height, std::vector<Rgba>(*outputCount)};
        for (std::size_t y = 0; y < height; ++y) {
            const auto &taps = (*vertical)[y];
            for (std::size_t x = 0; x < width; ++x) {
                LinearRgba value{};
                for (std::size_t k = 0; k < taps.weights.size(); ++k)
                    for (std::size_t c = 0; c < 4; ++c)
                        value[c] += temporary[(taps.first + k) * width + x][c] * taps.weights[k];
                // Divide by the unbounded filtered alpha before clipping negative-lobe overshoot.
                // Invisible output has no meaningful straight RGB; use black to isolate hidden
                // color.
                const auto alpha = static_cast<float>(detail::clamp(value[3]));
                if (alpha > 0)
                    output.pixels[y * width + x] = {detail::encoded(value[0] / value[3]),
                                                    detail::encoded(value[1] / value[3]),
                                                    detail::encoded(value[2] / value[3]), alpha};
                else
                    output.pixels[y * width + x] = {0, 0, 0, 0};
            }
        }
        return output;
    });
    if (!allocated)
        return std::unexpected(allocated.error());
    return std::move(*allocated);
}
} // namespace

Result<Image> resizeBox(ImageView image, std::size_t width, std::size_t height) {
    return resize(image, width, height, ResizeKernel::Box);
}
Result<Image> resizeBilinear(ImageView image, std::size_t width, std::size_t height) {
    return resize(image, width, height, ResizeKernel::Bilinear);
}
Result<Image> resizeMitchell(ImageView image, std::size_t width, std::size_t height) {
    return resize(image, width, height, ResizeKernel::Mitchell);
}
Result<Image> resizeLanczos3(ImageView image, std::size_t width, std::size_t height) {
    return resize(image, width, height, ResizeKernel::Lanczos3);
}
} // namespace iiGraphicAdjust
