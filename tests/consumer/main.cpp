#include <algorithm>
#include <array>
#include <iiGraphicAdjust/iiGraphicAdjust.h>

int runFilterConsumer();

int main() {
    using namespace iiGraphicAdjust;
    const std::array<std::uint32_t, 2> pixels{0xff808080U, 0x804080c0U};
    const auto image = fromArgb32(2, 1, pixels);
    if (!image)
        return 1;
    const auto neutral = toArgb32(image->view());
    if (!neutral || !std::equal(neutral->begin(), neutral->end(), pixels.begin(), pixels.end()))
        return 2;
    ColorAdjustment options;
    options.exposure = 1;
    const auto adjusted = adjust(image->view(), options);
    if (!adjusted || adjusted->pixels[0].r < .68F || adjusted->pixels[0].r > .70F)
        return 3;
    const auto noise = addNoise(adjusted->view(), {.05, 42});
    if (!noise)
        return 4;
    const auto blurred = gaussianBlur(noise->view(), .8);
    if (!blurred)
        return 4;
    const auto sharp = sharpen(blurred->view());
    if (!sharp)
        return 5;
    const auto mixed = blend(image->view(), sharp->view(), {BlendMode::SoftLight, .5});
    if (!mixed)
        return 6;
    const auto bytes = toRgba8(mixed->view());
    if (!bytes || bytes->size() != 8)
        return 7;

    const auto lut = identityLut(2);
    if (!lut)
        return 8;
    // Resolve every new public operation against the installed library, not
    // the SDK source tree. Unit suites provide each operation's numeric oracle.
    const std::array<Result<Image>, 25> operations{brightnessContrast(image->view(), {.1, .2}),
                                                   levels(image->view()),
                                                   curves(image->view()),
                                                   exposure(image->view(), {.5, 0, 1}),
                                                   vibrance(image->view(), {.4, 0}),
                                                   hueSaturation(image->view(), {30, .1, 0}),
                                                   colorBalance(image->view()),
                                                   blackAndWhite(image->view()),
                                                   photoFilter(image->view()),
                                                   channelMixer(image->view()),
                                                   invert(image->view()),
                                                   posterize(image->view(), 4),
                                                   threshold(image->view()),
                                                   gradientMap(image->view()),
                                                   selectiveColor(image->view()),
                                                   colorLookup(image->view(), *lut),
                                                   desaturate(image->view()),
                                                   equalize(image->view()),
                                                   shadowsHighlights(image->view(), {.4, .2}),
                                                   replaceColor(image->view()),
                                                   matchColor(image->view(), image->view()),
                                                   hdrToning(image->view()),
                                                   autoLevels(image->view()),
                                                   autoContrast(image->view()),
                                                   autoColor(image->view())};
    for (const auto &result : operations) {
        if (!result || result->width != 2 || result->height != 1 || result->pixels.size() != 2)
            return 9;
        for (std::size_t i = 0; i < 2; ++i)
            if (result->pixels[i].a != image->pixels[i].a)
                return 10;
    }
    const auto parsed = parseCubeLut("LUT_1D_SIZE 2\n0 0 0\n1 1 1\n");
    if (!parsed || !colorLookup(image->view(), *parsed))
        return 11;
    const std::array<HdrRgba, 1> radiance{{{4, 4, 4, .5F}}};
    const auto tone =
        hdrToning(HdrImageView{1, 1, radiance}, {HdrToningMethod::HighlightCompression});
    if (!tone || tone->pixels[0].r < .90F || tone->pixels[0].r > .91F)
        return 12;
    const std::array<Result<Image>, 11> developOperations{
        whiteBalance(image->view(), {7500, 5}),
        developLight(image->view(), {.5}),
        developCurve(image->view()),
        developColor(image->view()),
        calibrate(image->view()),
        developEffects(image->view(), {20, 10, -10}),
        developDetail(image->view()),
        postCropVignette(image->view(), {-20}),
        filmGrain(image->view(), {20, 25, 50, 7}),
        correctOptics(image->view()),
        develop(image->view())};
    for (const auto &result : developOperations)
        if (!result || result->width != 2 || result->height != 1 ||
            result->pixels[1].a != image->pixels[1].a)
            return 13;
    if (!whiteBalancePreset(WhiteBalancePreset::Daylight) || !autoWhiteBalance(image->view()) ||
        !sampledWhiteBalance(image->pixels[0]) || !autoDevelopLight(image->view()) ||
        !samplePointColor(image->pixels[1]) || !autoBlackAndWhiteMix(image->view()) ||
        !estimateChromaticAberration(image->view()))
        return 14;
    const auto linear = linearMask(2, 1), radial = radialMask(2, 1),
               luma = luminanceRangeMask(image->view());
    BrushStroke stroke;
    stroke.points = {{.5, .5}};
    const std::array<BrushStroke, 1> strokes{stroke};
    const auto brush = brushMask(image->view(), strokes),
               color = colorRangeMask(image->view(), {{image->pixels[0]}});
    if (!linear || !radial || !luma || !brush || !color)
        return 15;
    if (!combineMasks(linear->view(), radial->view(), MaskCombine::Intersect) ||
        !invertMask(linear->view()) || !refineMask(image->view(), linear->view(), 20))
        return 16;
    LocalAdjustment local{linear->view(), {}};
    local.options.exposure = .5;
    const std::array<LocalAdjustment, 1> locals{local};
    if (!applyLocalAdjustments(image->view(), locals))
        return 17;
    const std::array<std::uint16_t,4> mosaic{65535,32768,32768,0};
    const auto raw=developRaw({2,2,2,mosaic});
    if(!raw || raw->pixels[0].r!=1 || raw->pixels[0].b!=0) return 18;
    const auto rendered=renderRaw({2,2,2,mosaic});
    if(!rendered) return 19;
    const auto oriented=applyOrientation(rendered->view(),ExifOrientation::RightTop);
    if(!oriented || oriented->width!=2 || oriented->height!=2) return 20;
    const auto cropped=crop(oriented->view(),{0,0,1,2});
    if(!cropped || !imageHistogram(cropped->view())) return 21;
    return runFilterConsumer();
}
