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
    return !bytes || bytes->size()!=8 ? 7 : 0;
}
