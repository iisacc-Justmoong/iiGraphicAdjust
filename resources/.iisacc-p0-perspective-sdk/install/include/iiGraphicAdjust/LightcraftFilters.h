#pragma once
#include <iiGraphicAdjust/Develop.h>

namespace iiGraphicAdjust {
// LightCraft recipes, inspected commit 629e39380e296f588c64cd9c0053a8edc3528f36.
// amount is [0,2]; every named operation starts from default rendered-SDR settings.
enum class LightcraftLook {
    Color,
    Neutral,
    Vivid,
    Landscape,
    Portrait,
    Mono,
    FilmWarmPrint,
    FilmCoolFade,
    FilmGoldenHour,
    FilmFadedSlide,
    CineTealAmber,
    CineNightBlue,
    CineDesertHeat,
    CineNeonDusk,
    MutedMatteSoft,
    MutedBleached,
    MutedPastelHaze,
    MutedQuietGreen,
    BwMonoRich,
    BwRedFilter,
    BwSoft,
    BwSepia
};
enum class LightcraftPreset {
    WarmGlow,
    CoolMorning,
    TealOrange,
    VividPop,
    SoftPastel,
    FadedMatte,
    WarmFilm,
    MutedFilm,
    BwHighContrast,
    BwSoft,
    BwSelenium,
    CrispLandscape,
    GoldenHour,
    BlueHour,
    SoftSkin,
    BrightAiry,
    Moody,
    Cinematic,
    MoodyPortrait,
    GoldenGlow,
    CleanStudio,
    CrispVista,
    DeepSky,
    LushGreens,
    DesertWarmth,
    MistyMorning,
    GrittyStreet,
    NeonNight,
    ConcreteCool,
    FadedUrban,
    FreshBright,
    WarmTable,
    DarkMoodyFood,
    AutumnGold,
    WinterBlue,
    SpringFresh,
    SummerHaze,
    Instant70s,
    CrossProcess,
    BleachBypass,
    BwSepia
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<DevelopOptions>
lightcraftLookOptions(LightcraftLook look, double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<DevelopOptions>
lightcraftPresetOptions(LightcraftPreset preset, double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
applyLightcraftLook(ImageView image, LightcraftLook look, double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
applyLightcraftPreset(ImageView image, LightcraftPreset preset, double amount = 1);

[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookColor(ImageView image,
                                                                       double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookNeutral(ImageView image,
                                                                         double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookVivid(ImageView image,
                                                                       double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookLandscape(ImageView image,
                                                                           double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookPortrait(ImageView image,
                                                                          double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookMono(ImageView image,
                                                                      double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookFilmWarmPrint(ImageView image,
                                                                               double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookFilmCoolFade(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookFilmGoldenHour(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookFilmFadedSlide(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookCineTealAmber(ImageView image,
                                                                               double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookCineNightBlue(ImageView image,
                                                                               double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookCineDesertHeat(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookCineNeonDusk(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookMutedMatteSoft(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookMutedBleached(ImageView image,
                                                                               double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookMutedPastelHaze(ImageView image,
                                                                                 double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookMutedQuietGreen(ImageView image,
                                                                                 double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookBwMonoRich(ImageView image,
                                                                            double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookBwRedFilter(ImageView image,
                                                                             double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookBwSoft(ImageView image,
                                                                        double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftLookBwSepia(ImageView image,
                                                                         double amount = 1);

[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetWarmGlow(ImageView image,
                                                                            double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetCoolMorning(ImageView image,
                                                                               double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetTealOrange(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetVividPop(ImageView image,
                                                                            double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetSoftPastel(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetFadedMatte(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetWarmFilm(ImageView image,
                                                                            double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetMutedFilm(ImageView image,
                                                                             double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
lightcraftPresetBwHighContrast(ImageView image, double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetBwSoft(ImageView image,
                                                                          double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetBwSelenium(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
lightcraftPresetCrispLandscape(ImageView image, double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetGoldenHour(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetBlueHour(ImageView image,
                                                                            double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetSoftSkin(ImageView image,
                                                                            double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetBrightAiry(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetMoody(ImageView image,
                                                                         double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetCinematic(ImageView image,
                                                                             double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetMoodyPortrait(ImageView image,
                                                                                 double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetGoldenGlow(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetCleanStudio(ImageView image,
                                                                               double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetCrispVista(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetDeepSky(ImageView image,
                                                                           double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetLushGreens(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetDesertWarmth(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetMistyMorning(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetGrittyStreet(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetNeonNight(ImageView image,
                                                                             double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetConcreteCool(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetFadedUrban(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetFreshBright(ImageView image,
                                                                               double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetWarmTable(ImageView image,
                                                                             double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetDarkMoodyFood(ImageView image,
                                                                                 double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetAutumnGold(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetWinterBlue(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetSpringFresh(ImageView image,
                                                                               double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetSummerHaze(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetInstant70s(ImageView image,
                                                                              double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetCrossProcess(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetBleachBypass(ImageView image,
                                                                                double amount = 1);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> lightcraftPresetBwSepia(ImageView image,
                                                                           double amount = 1);

// Linear-light premultiplied RGBA; newly invisible resized output is black.
// Each axis <= 65536; input/output/intermediate <= 64 Mi pixels.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> resizeBox(ImageView image, std::size_t width,
                                                             std::size_t height);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
resizeBilinear(ImageView image, std::size_t width, std::size_t height);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
resizeMitchell(ImageView image, std::size_t width, std::size_t height);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
resizeLanczos3(ImageView image, std::size_t width, std::size_t height);
} // namespace iiGraphicAdjust
