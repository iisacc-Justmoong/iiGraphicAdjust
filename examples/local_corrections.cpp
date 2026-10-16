#include <iiFileProviderStorage.h>
#include <iiGraphicAdjust/iiGraphicAdjust.h>

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    using namespace iiGraphicAdjust;
    Image base{256, 128, {}};
    base.pixels.reserve(base.width * base.height);
    for (std::size_t y = 0; y < base.height; ++y)
        for (std::size_t x = 0; x < base.width; ++x)
            base.pixels.push_back({.1F + .65F * float(x) / float(base.width - 1),
                                   .15F + .5F * float(y) / float(base.height - 1), .35F, 1});
    const auto corrected = whiteBalance(base.view(), {4000, 8});
    const auto mask = radialMask(base.width, base.height, {{.5, .5}, .3, .2, 0, 75});
    if (!corrected || !mask) return 1;
    const auto localized = applyMaskedCorrection(base.view(), corrected->view(), mask->view(),
                                                  {85, CorrectionSpace::LinearSrgb});
    if (!localized) return 2;
    const auto bytes = toRgba8(localized->view());
    if (!bytes) return 3;
    const std::string path = argc > 1 ? argv[1] : "build/local-correction.ppm";
    iiFileProvider::native::OutputFile output(path, std::ios::binary);
    output << "P6\n" << base.width << ' ' << base.height << "\n255\n";
    for (std::size_t i = 0; i < bytes->size(); i += 4)
        output.write(reinterpret_cast<const char *>(bytes->data() + i), 3);
    if (!output) return 4;
    std::cout << "Wrote a masked Kelvin white-balance correction to " << path << '\n';
    return 0;
}
