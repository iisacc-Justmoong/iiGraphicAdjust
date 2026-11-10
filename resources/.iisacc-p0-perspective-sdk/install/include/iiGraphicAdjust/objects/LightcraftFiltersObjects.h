#pragma once
#include <iiGraphicAdjust/LightcraftFilters.h>
#include <iiGraphicAdjust/Operation.h>

namespace iiGraphicAdjust::objects {

struct ApplyLightcraftLook final {
    inline static constexpr OperationInfo info{"look.applyLightcraftLook", "ApplyLightcraftLook", "look"};
    iiGraphicAdjust::LightcraftLook look = iiGraphicAdjust::LightcraftLook::Color;
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::applyLightcraftLook(image, look, amount);
    }
};

struct ApplyLightcraftPreset final {
    inline static constexpr OperationInfo info{"look.applyLightcraftPreset", "ApplyLightcraftPreset", "look"};
    iiGraphicAdjust::LightcraftPreset preset = iiGraphicAdjust::LightcraftPreset::WarmGlow;
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::applyLightcraftPreset(image, preset, amount);
    }
};

struct LightcraftLookColor final {
    inline static constexpr OperationInfo info{"look.lightcraftLookColor", "LightcraftLookColor", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookColor(image, amount);
    }
};

struct LightcraftLookNeutral final {
    inline static constexpr OperationInfo info{"look.lightcraftLookNeutral", "LightcraftLookNeutral", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookNeutral(image, amount);
    }
};

struct LightcraftLookVivid final {
    inline static constexpr OperationInfo info{"look.lightcraftLookVivid", "LightcraftLookVivid", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookVivid(image, amount);
    }
};

struct LightcraftLookLandscape final {
    inline static constexpr OperationInfo info{"look.lightcraftLookLandscape", "LightcraftLookLandscape", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookLandscape(image, amount);
    }
};

struct LightcraftLookPortrait final {
    inline static constexpr OperationInfo info{"look.lightcraftLookPortrait", "LightcraftLookPortrait", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookPortrait(image, amount);
    }
};

struct LightcraftLookMono final {
    inline static constexpr OperationInfo info{"look.lightcraftLookMono", "LightcraftLookMono", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookMono(image, amount);
    }
};

struct LightcraftLookFilmWarmPrint final {
    inline static constexpr OperationInfo info{"look.lightcraftLookFilmWarmPrint", "LightcraftLookFilmWarmPrint", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookFilmWarmPrint(image, amount);
    }
};

struct LightcraftLookFilmCoolFade final {
    inline static constexpr OperationInfo info{"look.lightcraftLookFilmCoolFade", "LightcraftLookFilmCoolFade", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookFilmCoolFade(image, amount);
    }
};

struct LightcraftLookFilmGoldenHour final {
    inline static constexpr OperationInfo info{"look.lightcraftLookFilmGoldenHour", "LightcraftLookFilmGoldenHour", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookFilmGoldenHour(image, amount);
    }
};

struct LightcraftLookFilmFadedSlide final {
    inline static constexpr OperationInfo info{"look.lightcraftLookFilmFadedSlide", "LightcraftLookFilmFadedSlide", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookFilmFadedSlide(image, amount);
    }
};

struct LightcraftLookCineTealAmber final {
    inline static constexpr OperationInfo info{"look.lightcraftLookCineTealAmber", "LightcraftLookCineTealAmber", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookCineTealAmber(image, amount);
    }
};

struct LightcraftLookCineNightBlue final {
    inline static constexpr OperationInfo info{"look.lightcraftLookCineNightBlue", "LightcraftLookCineNightBlue", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookCineNightBlue(image, amount);
    }
};

struct LightcraftLookCineDesertHeat final {
    inline static constexpr OperationInfo info{"look.lightcraftLookCineDesertHeat", "LightcraftLookCineDesertHeat", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookCineDesertHeat(image, amount);
    }
};

struct LightcraftLookCineNeonDusk final {
    inline static constexpr OperationInfo info{"look.lightcraftLookCineNeonDusk", "LightcraftLookCineNeonDusk", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookCineNeonDusk(image, amount);
    }
};

struct LightcraftLookMutedMatteSoft final {
    inline static constexpr OperationInfo info{"look.lightcraftLookMutedMatteSoft", "LightcraftLookMutedMatteSoft", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookMutedMatteSoft(image, amount);
    }
};

struct LightcraftLookMutedBleached final {
    inline static constexpr OperationInfo info{"look.lightcraftLookMutedBleached", "LightcraftLookMutedBleached", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookMutedBleached(image, amount);
    }
};

struct LightcraftLookMutedPastelHaze final {
    inline static constexpr OperationInfo info{"look.lightcraftLookMutedPastelHaze", "LightcraftLookMutedPastelHaze", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookMutedPastelHaze(image, amount);
    }
};

struct LightcraftLookMutedQuietGreen final {
    inline static constexpr OperationInfo info{"look.lightcraftLookMutedQuietGreen", "LightcraftLookMutedQuietGreen", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookMutedQuietGreen(image, amount);
    }
};

struct LightcraftLookBwMonoRich final {
    inline static constexpr OperationInfo info{"look.lightcraftLookBwMonoRich", "LightcraftLookBwMonoRich", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookBwMonoRich(image, amount);
    }
};

struct LightcraftLookBwRedFilter final {
    inline static constexpr OperationInfo info{"look.lightcraftLookBwRedFilter", "LightcraftLookBwRedFilter", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookBwRedFilter(image, amount);
    }
};

struct LightcraftLookBwSoft final {
    inline static constexpr OperationInfo info{"look.lightcraftLookBwSoft", "LightcraftLookBwSoft", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookBwSoft(image, amount);
    }
};

struct LightcraftLookBwSepia final {
    inline static constexpr OperationInfo info{"look.lightcraftLookBwSepia", "LightcraftLookBwSepia", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftLookBwSepia(image, amount);
    }
};

struct LightcraftPresetWarmGlow final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetWarmGlow", "LightcraftPresetWarmGlow", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetWarmGlow(image, amount);
    }
};

struct LightcraftPresetCoolMorning final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetCoolMorning", "LightcraftPresetCoolMorning", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetCoolMorning(image, amount);
    }
};

struct LightcraftPresetTealOrange final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetTealOrange", "LightcraftPresetTealOrange", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetTealOrange(image, amount);
    }
};

struct LightcraftPresetVividPop final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetVividPop", "LightcraftPresetVividPop", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetVividPop(image, amount);
    }
};

struct LightcraftPresetSoftPastel final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetSoftPastel", "LightcraftPresetSoftPastel", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetSoftPastel(image, amount);
    }
};

struct LightcraftPresetFadedMatte final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetFadedMatte", "LightcraftPresetFadedMatte", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetFadedMatte(image, amount);
    }
};

struct LightcraftPresetWarmFilm final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetWarmFilm", "LightcraftPresetWarmFilm", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetWarmFilm(image, amount);
    }
};

struct LightcraftPresetMutedFilm final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetMutedFilm", "LightcraftPresetMutedFilm", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetMutedFilm(image, amount);
    }
};

struct LightcraftPresetBwHighContrast final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetBwHighContrast", "LightcraftPresetBwHighContrast", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetBwHighContrast(image, amount);
    }
};

struct LightcraftPresetBwSoft final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetBwSoft", "LightcraftPresetBwSoft", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetBwSoft(image, amount);
    }
};

struct LightcraftPresetBwSelenium final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetBwSelenium", "LightcraftPresetBwSelenium", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetBwSelenium(image, amount);
    }
};

struct LightcraftPresetCrispLandscape final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetCrispLandscape", "LightcraftPresetCrispLandscape", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetCrispLandscape(image, amount);
    }
};

struct LightcraftPresetGoldenHour final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetGoldenHour", "LightcraftPresetGoldenHour", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetGoldenHour(image, amount);
    }
};

struct LightcraftPresetBlueHour final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetBlueHour", "LightcraftPresetBlueHour", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetBlueHour(image, amount);
    }
};

struct LightcraftPresetSoftSkin final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetSoftSkin", "LightcraftPresetSoftSkin", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetSoftSkin(image, amount);
    }
};

struct LightcraftPresetBrightAiry final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetBrightAiry", "LightcraftPresetBrightAiry", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetBrightAiry(image, amount);
    }
};

struct LightcraftPresetMoody final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetMoody", "LightcraftPresetMoody", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetMoody(image, amount);
    }
};

struct LightcraftPresetCinematic final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetCinematic", "LightcraftPresetCinematic", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetCinematic(image, amount);
    }
};

struct LightcraftPresetMoodyPortrait final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetMoodyPortrait", "LightcraftPresetMoodyPortrait", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetMoodyPortrait(image, amount);
    }
};

struct LightcraftPresetGoldenGlow final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetGoldenGlow", "LightcraftPresetGoldenGlow", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetGoldenGlow(image, amount);
    }
};

struct LightcraftPresetCleanStudio final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetCleanStudio", "LightcraftPresetCleanStudio", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetCleanStudio(image, amount);
    }
};

struct LightcraftPresetCrispVista final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetCrispVista", "LightcraftPresetCrispVista", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetCrispVista(image, amount);
    }
};

struct LightcraftPresetDeepSky final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetDeepSky", "LightcraftPresetDeepSky", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetDeepSky(image, amount);
    }
};

struct LightcraftPresetLushGreens final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetLushGreens", "LightcraftPresetLushGreens", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetLushGreens(image, amount);
    }
};

struct LightcraftPresetDesertWarmth final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetDesertWarmth", "LightcraftPresetDesertWarmth", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetDesertWarmth(image, amount);
    }
};

struct LightcraftPresetMistyMorning final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetMistyMorning", "LightcraftPresetMistyMorning", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetMistyMorning(image, amount);
    }
};

struct LightcraftPresetGrittyStreet final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetGrittyStreet", "LightcraftPresetGrittyStreet", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetGrittyStreet(image, amount);
    }
};

struct LightcraftPresetNeonNight final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetNeonNight", "LightcraftPresetNeonNight", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetNeonNight(image, amount);
    }
};

struct LightcraftPresetConcreteCool final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetConcreteCool", "LightcraftPresetConcreteCool", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetConcreteCool(image, amount);
    }
};

struct LightcraftPresetFadedUrban final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetFadedUrban", "LightcraftPresetFadedUrban", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetFadedUrban(image, amount);
    }
};

struct LightcraftPresetFreshBright final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetFreshBright", "LightcraftPresetFreshBright", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetFreshBright(image, amount);
    }
};

struct LightcraftPresetWarmTable final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetWarmTable", "LightcraftPresetWarmTable", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetWarmTable(image, amount);
    }
};

struct LightcraftPresetDarkMoodyFood final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetDarkMoodyFood", "LightcraftPresetDarkMoodyFood", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetDarkMoodyFood(image, amount);
    }
};

struct LightcraftPresetAutumnGold final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetAutumnGold", "LightcraftPresetAutumnGold", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetAutumnGold(image, amount);
    }
};

struct LightcraftPresetWinterBlue final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetWinterBlue", "LightcraftPresetWinterBlue", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetWinterBlue(image, amount);
    }
};

struct LightcraftPresetSpringFresh final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetSpringFresh", "LightcraftPresetSpringFresh", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetSpringFresh(image, amount);
    }
};

struct LightcraftPresetSummerHaze final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetSummerHaze", "LightcraftPresetSummerHaze", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetSummerHaze(image, amount);
    }
};

struct LightcraftPresetInstant70s final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetInstant70s", "LightcraftPresetInstant70s", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetInstant70s(image, amount);
    }
};

struct LightcraftPresetCrossProcess final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetCrossProcess", "LightcraftPresetCrossProcess", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetCrossProcess(image, amount);
    }
};

struct LightcraftPresetBleachBypass final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetBleachBypass", "LightcraftPresetBleachBypass", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetBleachBypass(image, amount);
    }
};

struct LightcraftPresetBwSepia final {
    inline static constexpr OperationInfo info{"look.lightcraftPresetBwSepia", "LightcraftPresetBwSepia", "look"};
    double amount = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::lightcraftPresetBwSepia(image, amount);
    }
};

struct ResizeBox final {
    inline static constexpr OperationInfo info{"look.resizeBox", "ResizeBox", "look"};
    std::size_t width = 1, height = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::resizeBox(image, width, height);
    }
};

struct ResizeBilinear final {
    inline static constexpr OperationInfo info{"look.resizeBilinear", "ResizeBilinear", "look"};
    std::size_t width = 1, height = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::resizeBilinear(image, width, height);
    }
};

struct ResizeMitchell final {
    inline static constexpr OperationInfo info{"look.resizeMitchell", "ResizeMitchell", "look"};
    std::size_t width = 1, height = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::resizeMitchell(image, width, height);
    }
};

struct ResizeLanczos3 final {
    inline static constexpr OperationInfo info{"look.resizeLanczos3", "ResizeLanczos3", "look"};
    std::size_t width = 1, height = 1;
    [[nodiscard]] Result<Image> apply(ImageView image) const {
        return iiGraphicAdjust::resizeLanczos3(image, width, height);
    }
};

} // namespace iiGraphicAdjust::objects
