#include <array>
#include <cmath>
#include <iiGraphicAdjust/iiGraphicAdjust.h>
#include <iostream>
using namespace iiGraphicAdjust;
int runFilterConsumer() {
    Image image{8, 8, {}};
    for (std::size_t i = 0; i < 64; ++i)
        image.pixels.push_back({float(i % 8) / 7, float(i / 8) / 7, float((i * 7) % 19) / 18, 1});
    const auto original = image.pixels;
    int failures = 0, operations = 0;
    auto check = [&](const Result<Image> &result, const char *name, std::size_t width = 8,
                     std::size_t height = 8) {
        ++operations;
        if (!result || result->width != width || result->height != height ||
            result->pixels.size() != width * height) {
            std::cerr << "Installed filter failed: " << name << '\n';
            ++failures;
            return;
        }
        for (auto p : result->pixels)
            for (auto v : {p.r, p.g, p.b, p.a})
                if (!std::isfinite(v) || v < 0 || v > 1) {
                    std::cerr << "Invalid installed output: " << name << '\n';
                    ++failures;
                    return;
                }
    };
    Image map{8, 8, std::vector<Rgba>(64, {.5F, .5F, .5F, 1})};
    std::array iris{IrisBlurPin{}};
    std::array field{FieldBlurPin{}};
    std::array spin{SpinBlurPin{}};
    std::array paths{BlurPath{}};
    std::array shearPoints{ShearPoint{0, 0}, ShearPoint{1, .1}};
    PuppetWarpOptions puppet;
    puppet.pins = {PuppetPin{{2.5, 2.5}, {3.5, 2.5}}};
    PerspectiveWarpOptions perspective;
    perspective.planes = {PerspectivePlane{MeshQuad{{{.5, .5}, {7.5, .5}, {7.5, 7.5}, {.5, 7.5}}},
                                           MeshQuad{{{.5, .5}, {7.5, .5}, {7.5, 7.5}, {.5, 7.5}}}}};
    VanishingPointOptions vanishing;
    vanishing.planes = {VanishingPlane{MeshQuad{{{.5, .5}, {7.5, .5}, {7.5, 7.5}, {.5, 7.5}}}}};
    check(perspectiveWarp(image.view(), perspective), "perspectiveWarp");
    check(puppetWarp(image.view(), puppet), "puppetWarp");
    check(adaptiveWideAngle(image.view()), "adaptiveWideAngle");
    check(average(image.view()), "average");
    check(blur(image.view()), "blur");
    check(blurMore(image.view()), "blurMore");
    check(boxBlur(image.view()), "boxBlur");
    check(filterGaussianBlur(image.view()), "filterGaussianBlur");
    check(lensBlur(image.view()), "lensBlur");
    check(motionBlur(image.view()), "motionBlur");
    check(radialBlur(image.view()), "radialBlur");
    check(shapeBlur(image.view()), "shapeBlur");
    check(smartBlur(image.view()), "smartBlur");
    check(surfaceBlur(image.view()), "surfaceBlur");
    check(fieldBlur(image.view(), field), "fieldBlur");
    check(irisBlur(image.view(), iris), "irisBlur");
    check(pathBlur(image.view(), paths), "pathBlur");
    check(spinBlur(image.view(), spin), "spinBlur");
    check(tiltShift(image.view()), "tiltShift");
    check(cameraRawFilter(image.view()), "cameraRawFilter");
    check(displace(image.view(), map.view()), "displace");
    check(pinch(image.view()), "pinch");
    check(polarCoordinates(image.view()), "polarCoordinates");
    check(ripple(image.view()), "ripple");
    check(shear(image.view(), shearPoints), "shear");
    check(spherize(image.view()), "spherize");
    check(twirl(image.view()), "twirl");
    check(wave(image.view()), "wave");
    check(zigZag(image.view()), "zigZag");
    check(accentedEdges(image.view()), "accentedEdges");
    check(angledStrokes(image.view()), "angledStrokes");
    check(basRelief(image.view()), "basRelief");
    check(chalkCharcoal(image.view()), "chalkCharcoal");
    check(charcoal(image.view()), "charcoal");
    check(chrome(image.view()), "chrome");
    check(coloredPencil(image.view()), "coloredPencil");
    check(conteCrayon(image.view()), "conteCrayon");
    check(craquelure(image.view()), "craquelure");
    check(crosshatch(image.view()), "crosshatch");
    check(cutout(image.view()), "cutout");
    check(darkStrokes(image.view()), "darkStrokes");
    check(diffuseGlow(image.view()), "diffuseGlow");
    check(dryBrush(image.view()), "dryBrush");
    check(galleryFilmGrain(image.view()), "galleryFilmGrain");
    check(fresco(image.view()), "fresco");
    check(glass(image.view()), "glass");
    check(glowingEdges(image.view()), "glowingEdges");
    check(grain(image.view()), "grain");
    check(graphicPen(image.view()), "graphicPen");
    check(halftonePattern(image.view()), "halftonePattern");
    check(inkOutlines(image.view()), "inkOutlines");
    check(mosaicTiles(image.view()), "mosaicTiles");
    check(neonGlow(image.view()), "neonGlow");
    check(notePaper(image.view()), "notePaper");
    check(oceanRipple(image.view()), "oceanRipple");
    check(paintDaubs(image.view()), "paintDaubs");
    check(paletteKnife(image.view()), "paletteKnife");
    check(patchwork(image.view()), "patchwork");
    check(photocopy(image.view()), "photocopy");
    check(plaster(image.view()), "plaster");
    check(plasticWrap(image.view()), "plasticWrap");
    check(posterEdges(image.view()), "posterEdges");
    check(reticulation(image.view()), "reticulation");
    check(roughPastels(image.view()), "roughPastels");
    check(smudgeStick(image.view()), "smudgeStick");
    check(spatter(image.view()), "spatter");
    check(sponge(image.view()), "sponge");
    check(sprayedStrokes(image.view()), "sprayedStrokes");
    check(stainedGlass(image.view()), "stainedGlass");
    check(stamp(image.view()), "stamp");
    check(sumiE(image.view()), "sumiE");
    check(texturizer(image.view()), "texturizer");
    check(tornEdges(image.view()), "tornEdges");
    check(underpainting(image.view()), "underpainting");
    check(waterPaper(image.view()), "waterPaper");
    check(watercolor(image.view()), "watercolor");
    check(lensCorrection(image.view()), "lensCorrection");
    check(liquify(image.view()), "liquify");
    check(filterAddNoise(image.view()), "filterAddNoise");
    check(despeckle(image.view()), "despeckle");
    check(dustAndScratches(image.view()), "dustAndScratches");
    check(median(image.view()), "median");
    check(reduceNoise(image.view()), "reduceNoise");
    check(customFilter(image.view()), "customFilter");
    check(highPass(image.view()), "highPass");
    check(hsbHsl(image.view(), FilterColorModel::Rgb, FilterColorModel::Hsl), "hsbHsl");
    check(maximum(image.view()), "maximum");
    check(minimum(image.view()), "minimum");
    check(offset(image.view(), 1, -1), "offset");
    check(colorHalftone(image.view()), "colorHalftone");
    check(crystallize(image.view()), "crystallize");
    check(facet(image.view()), "facet");
    check(fragment(image.view()), "fragment");
    check(mezzotint(image.view()), "mezzotint");
    check(mosaic(image.view()), "mosaic");
    check(pointillize(image.view()), "pointillize");
    check(clouds(image.view()), "clouds");
    check(differenceClouds(image.view()), "differenceClouds");
    check(fibers(image.view()), "fibers");
    check(flame(image.view()), "flame");
    check(lensFlare(image.view()), "lensFlare");
    check(lightingEffects(image.view()), "lightingEffects");
    check(pictureFrame(image.view()), "pictureFrame");
    check(relight(image.view()), "relight");
    check(tree(image.view()), "tree");
    check(filterSharpen(image.view()), "filterSharpen");
    check(sharpenEdges(image.view()), "sharpenEdges");
    check(sharpenMore(image.view()), "sharpenMore");
    check(smartSharpen(image.view()), "smartSharpen");
    check(unsharpMask(image.view()), "unsharpMask");
    check(diffuse(image.view()), "diffuse");
    check(emboss(image.view()), "emboss");
    check(extrude(image.view()), "extrude");
    check(findEdges(image.view()), "findEdges");
    check(oilPaint(image.view()), "oilPaint");
    check(solarize(image.view()), "solarize");
    check(tiles(image.view()), "tiles");
    check(traceContour(image.view()), "traceContour");
    check(wind(image.view()), "wind");
    check(vanishingPoint(image.view(), vanishing), "vanishingPoint");
    check(deInterlace(image.view()), "deInterlace");
    check(ntscColors(image.view()), "ntscColors");
    check(guidedBlur(image.view()), "guidedBlur");
    check(crossGuidedBlur(image.view(), image.view()), "crossGuidedBlur");
    const std::array<GalleryEffect, 2> stack{GalleryCutoutOptions{}, GalleryDryBrushOptions{}};
    check(filterGallery(image.view(), stack), "filterGallery");
    check(lightcraftLookColor(image.view(), .5), "lightcraftLookColor");
    check(lightcraftLookNeutral(image.view(), .5), "lightcraftLookNeutral");
    check(lightcraftLookVivid(image.view(), .5), "lightcraftLookVivid");
    check(lightcraftLookLandscape(image.view(), .5), "lightcraftLookLandscape");
    check(lightcraftLookPortrait(image.view(), .5), "lightcraftLookPortrait");
    check(lightcraftLookMono(image.view(), .5), "lightcraftLookMono");
    check(lightcraftLookFilmWarmPrint(image.view(), .5), "lightcraftLookFilmWarmPrint");
    check(lightcraftLookFilmCoolFade(image.view(), .5), "lightcraftLookFilmCoolFade");
    check(lightcraftLookFilmGoldenHour(image.view(), .5), "lightcraftLookFilmGoldenHour");
    check(lightcraftLookFilmFadedSlide(image.view(), .5), "lightcraftLookFilmFadedSlide");
    check(lightcraftLookCineTealAmber(image.view(), .5), "lightcraftLookCineTealAmber");
    check(lightcraftLookCineNightBlue(image.view(), .5), "lightcraftLookCineNightBlue");
    check(lightcraftLookCineDesertHeat(image.view(), .5), "lightcraftLookCineDesertHeat");
    check(lightcraftLookCineNeonDusk(image.view(), .5), "lightcraftLookCineNeonDusk");
    check(lightcraftLookMutedMatteSoft(image.view(), .5), "lightcraftLookMutedMatteSoft");
    check(lightcraftLookMutedBleached(image.view(), .5), "lightcraftLookMutedBleached");
    check(lightcraftLookMutedPastelHaze(image.view(), .5), "lightcraftLookMutedPastelHaze");
    check(lightcraftLookMutedQuietGreen(image.view(), .5), "lightcraftLookMutedQuietGreen");
    check(lightcraftLookBwMonoRich(image.view(), .5), "lightcraftLookBwMonoRich");
    check(lightcraftLookBwRedFilter(image.view(), .5), "lightcraftLookBwRedFilter");
    check(lightcraftLookBwSoft(image.view(), .5), "lightcraftLookBwSoft");
    check(lightcraftLookBwSepia(image.view(), .5), "lightcraftLookBwSepia");
    check(lightcraftPresetWarmGlow(image.view(), .5), "lightcraftPresetWarmGlow");
    check(lightcraftPresetCoolMorning(image.view(), .5), "lightcraftPresetCoolMorning");
    check(lightcraftPresetTealOrange(image.view(), .5), "lightcraftPresetTealOrange");
    check(lightcraftPresetVividPop(image.view(), .5), "lightcraftPresetVividPop");
    check(lightcraftPresetSoftPastel(image.view(), .5), "lightcraftPresetSoftPastel");
    check(lightcraftPresetFadedMatte(image.view(), .5), "lightcraftPresetFadedMatte");
    check(lightcraftPresetWarmFilm(image.view(), .5), "lightcraftPresetWarmFilm");
    check(lightcraftPresetMutedFilm(image.view(), .5), "lightcraftPresetMutedFilm");
    check(lightcraftPresetBwHighContrast(image.view(), .5), "lightcraftPresetBwHighContrast");
    check(lightcraftPresetBwSoft(image.view(), .5), "lightcraftPresetBwSoft");
    check(lightcraftPresetBwSelenium(image.view(), .5), "lightcraftPresetBwSelenium");
    check(lightcraftPresetCrispLandscape(image.view(), .5), "lightcraftPresetCrispLandscape");
    check(lightcraftPresetGoldenHour(image.view(), .5), "lightcraftPresetGoldenHour");
    check(lightcraftPresetBlueHour(image.view(), .5), "lightcraftPresetBlueHour");
    check(lightcraftPresetSoftSkin(image.view(), .5), "lightcraftPresetSoftSkin");
    check(lightcraftPresetBrightAiry(image.view(), .5), "lightcraftPresetBrightAiry");
    check(lightcraftPresetMoody(image.view(), .5), "lightcraftPresetMoody");
    check(lightcraftPresetCinematic(image.view(), .5), "lightcraftPresetCinematic");
    check(lightcraftPresetMoodyPortrait(image.view(), .5), "lightcraftPresetMoodyPortrait");
    check(lightcraftPresetGoldenGlow(image.view(), .5), "lightcraftPresetGoldenGlow");
    check(lightcraftPresetCleanStudio(image.view(), .5), "lightcraftPresetCleanStudio");
    check(lightcraftPresetCrispVista(image.view(), .5), "lightcraftPresetCrispVista");
    check(lightcraftPresetDeepSky(image.view(), .5), "lightcraftPresetDeepSky");
    check(lightcraftPresetLushGreens(image.view(), .5), "lightcraftPresetLushGreens");
    check(lightcraftPresetDesertWarmth(image.view(), .5), "lightcraftPresetDesertWarmth");
    check(lightcraftPresetMistyMorning(image.view(), .5), "lightcraftPresetMistyMorning");
    check(lightcraftPresetGrittyStreet(image.view(), .5), "lightcraftPresetGrittyStreet");
    check(lightcraftPresetNeonNight(image.view(), .5), "lightcraftPresetNeonNight");
    check(lightcraftPresetConcreteCool(image.view(), .5), "lightcraftPresetConcreteCool");
    check(lightcraftPresetFadedUrban(image.view(), .5), "lightcraftPresetFadedUrban");
    check(lightcraftPresetFreshBright(image.view(), .5), "lightcraftPresetFreshBright");
    check(lightcraftPresetWarmTable(image.view(), .5), "lightcraftPresetWarmTable");
    check(lightcraftPresetDarkMoodyFood(image.view(), .5), "lightcraftPresetDarkMoodyFood");
    check(lightcraftPresetAutumnGold(image.view(), .5), "lightcraftPresetAutumnGold");
    check(lightcraftPresetWinterBlue(image.view(), .5), "lightcraftPresetWinterBlue");
    check(lightcraftPresetSpringFresh(image.view(), .5), "lightcraftPresetSpringFresh");
    check(lightcraftPresetSummerHaze(image.view(), .5), "lightcraftPresetSummerHaze");
    check(lightcraftPresetInstant70s(image.view(), .5), "lightcraftPresetInstant70s");
    check(lightcraftPresetCrossProcess(image.view(), .5), "lightcraftPresetCrossProcess");
    check(lightcraftPresetBleachBypass(image.view(), .5), "lightcraftPresetBleachBypass");
    check(lightcraftPresetBwSepia(image.view(), .5), "lightcraftPresetBwSepia");
    check(resizeBox(image.view(), 7, 9), "resizeBox", 7, 9);
    check(resizeBilinear(image.view(), 7, 9), "resizeBilinear", 7, 9);
    check(resizeMitchell(image.view(), 7, 9), "resizeMitchell", 7, 9);
    check(resizeLanczos3(image.view(), 7, 9), "resizeLanczos3", 7, 9);
    check(applyLightcraftLook(image.view(), LightcraftLook::Vivid, .5), "applyLightcraftLook");
    check(applyLightcraftPreset(image.view(), LightcraftPreset::BwSepia, .5),
          "applyLightcraftPreset");
    if (!lightcraftLookOptions(LightcraftLook::Landscape) ||
        !lightcraftPresetOptions(LightcraftPreset::WarmGlow) || !genericLensProfile(35) ||
        !autoLensScale(image.view()) || !solveAdaptiveWideAngle(image.view()) ||
        !solvePuppetWarp(image.view(), puppet) || !mapPerspectiveWarpPoint({2, 2}, perspective) ||
        !vanishingPointPlanes(image.view(), vanishing)) {
        std::cerr << "Installed filter helper failed\n";
        ++failures;
    }
    SpotOptions spot;
    spot.size = .25;
    spot.sourceOffset = std::array{.25, 0.};
    check(cloneSpot(image.view(), spot), "cloneSpot");
    check(healSpot(image.view(), spot), "healSpot");
    check(removeSpot(image.view(), spot), "removeSpot");
    EyeOptions eye;
    eye.radiusX = eye.radiusY = .25;
    check(redEye(image.view(), eye), "redEye");
    check(petEye(image.view(), eye), "petEye");
    check(visualizeSpots(image.view()), "visualizeSpots");
    if (!rankSpotSources(image.view()) || !detectPupil(image.view(), eye) ||
        !detectDustSpots(image.view())) {
        std::cerr << "Installed retouch analysis failed\n";
        ++failures;
    }
    const auto previewSample = samplePointColor(image.pixels[27]);
    const auto diagnosticMask = radialMask(8, 8);
    if (!previewSample || !diagnosticMask)
        return 18;
    check(pointColorRangePreview(image.view(), *previewSample), "pointColorRangePreview");
    check(maskColorOverlay(image.view(), diagnosticMask->view()), "maskColorOverlay");
    check(maskColorOnBwOverlay(image.view(), diagnosticMask->view()), "maskColorOnBwOverlay");
    check(maskImageOnBlack(image.view(), diagnosticMask->view()), "maskImageOnBlack");
    check(maskImageOnWhite(image.view(), diagnosticMask->view()), "maskImageOnWhite");
    check(maskWhiteOnBlack(image.view(), diagnosticMask->view()), "maskWhiteOnBlack");
    const std::vector<std::array<double, 3>> extended(64, {1.5, -.1, .2});
    GamutWarningOptions gamut;
    gamut.linearRec2020 = extended;
    check(destinationGamutWarning(image.view(), gamut), "destinationGamutWarning");
    check(displayGamutWarning(image.view(), gamut), "displayGamutWarning");
    if (image.pixels != original) {
        std::cerr << "Installed filters modified source\n";
        ++failures;
    }
    std::cout << "Installed filter image calls: " << operations << "\n";
    return failures ? 18 : 0;
}
