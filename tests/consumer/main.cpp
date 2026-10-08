#include <iiGraphicAdjust/iiGraphicAdjust.h>
#include <algorithm>
#include <array>

int main() {
    using namespace iiGraphicAdjust;
    const std::array<std::uint32_t,2> pixels{0xff808080U,0x804080c0U};
    const auto image=fromArgb32(2,1,pixels);
    if (!image) return 1;
    const auto neutral=toArgb32(image->view());
    if (!neutral || !std::equal(neutral->begin(),neutral->end(),pixels.begin(),pixels.end())) return 2;
    ColorAdjustment options; options.exposure=1;
    const auto adjusted=adjust(image->view(),options);
    if (!adjusted || adjusted->pixels[0].r < .68F || adjusted->pixels[0].r > .70F) return 3;
    const auto noise=addNoise(adjusted->view(),{.05,42});
    if (!noise) return 4;
    const auto blurred=gaussianBlur(noise->view(),.8);
    if (!blurred) return 4;
    const auto sharp=sharpen(blurred->view());
    if (!sharp) return 5;
    const auto mixed=blend(image->view(),sharp->view(),{BlendMode::SoftLight,.5});
    if (!mixed) return 6;
    const auto bytes=toRgba8(mixed->view());
    if (!bytes || bytes->size()!=8) return 7;

    const auto lut=identityLut(2);
    if (!lut) return 8;
    // Resolve every new public operation against the installed library, not
    // the SDK source tree. Unit suites provide each operation's numeric oracle.
    const std::array<Result<Image>,25> operations{
        brightnessContrast(image->view(),{.1,.2}), levels(image->view()), curves(image->view()),
        exposure(image->view(),{.5,0,1}), vibrance(image->view(),{.4,0}),
        hueSaturation(image->view(),{30,.1,0}), colorBalance(image->view()),
        blackAndWhite(image->view()), photoFilter(image->view()), channelMixer(image->view()),
        invert(image->view()), posterize(image->view(),4), threshold(image->view()),
        gradientMap(image->view()), selectiveColor(image->view()), colorLookup(image->view(),*lut),
        desaturate(image->view()), equalize(image->view()), shadowsHighlights(image->view(),{.4,.2}),
        replaceColor(image->view()), matchColor(image->view(),image->view()), hdrToning(image->view()),
        autoLevels(image->view()), autoContrast(image->view()), autoColor(image->view())};
    for (const auto& result:operations) {
        if (!result || result->width!=2 || result->height!=1 || result->pixels.size()!=2) return 9;
        for (std::size_t i=0;i<2;++i) if (result->pixels[i].a!=image->pixels[i].a) return 10;
    }
    const auto parsed=parseCubeLut("LUT_1D_SIZE 2\n0 0 0\n1 1 1\n");
    if (!parsed || !colorLookup(image->view(),*parsed)) return 11;
    const std::array<HdrRgba,1> radiance{{{4,4,4,.5F}}};
    const auto tone=hdrToning(HdrImageView{1,1,radiance},{HdrToningMethod::HighlightCompression});
    return !tone || tone->pixels[0].r<.90F || tone->pixels[0].r>.91F ? 12 : 0;
}
