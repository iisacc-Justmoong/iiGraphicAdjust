#include <fstream>
#include <iiGraphicAdjust/iiGraphicAdjust.h>
#include <iostream>

int main(int argc, char **argv) {
    using namespace iiGraphicAdjust;
    if (argc != 2) {
        std::cerr << "Usage: iiGraphicAdjustDevelopExample output.ppm\n";
        return 1;
    }
    Image source{256, 128, {}};
    for (int y = 0; y < 128; ++y)
        for (int x = 0; x < 256; ++x)
            source.pixels.push_back({x / 255.F, y / 127.F, .25F + .5F * (x % 32) / 31, 1});
    DevelopOptions options;
    options.light.exposure = .25;
    options.light.shadows = 20;
    options.color.vibrance = 25;
    options.color.mixer[static_cast<int>(MixerBand::Blue)].luminance = 15;
    options.effects.texture = 20;
    options.vignette.amount = -25;
    options.grain = {10, 25, 50, 42};
    auto adjusted = develop(source.view(), options);
    if (!adjusted) {
        std::cerr << adjusted.error().message << '\n';
        return 2;
    }
    auto selection = radialMask(source.width, source.height);
    if (!selection)
        return 3;
    LocalAdjustment local{selection->view(), {}};
    local.options.exposure = .3;
    const std::array<LocalAdjustment, 1> locals{local};
    auto result = applyLocalAdjustments(adjusted->view(), locals);
    if (!result)
        return 4;
    auto bytes = toRgba8(result->view());
    if (!bytes)
        return 5;
    std::ofstream file(argv[1], std::ios::binary);
    file << "P6\n256 128\n255\n";
    for (std::size_t i = 0; i < bytes->size(); i += 4)
        file.write(reinterpret_cast<const char *>(bytes->data() + i), 3);
    if (!file)
        return 6;
    std::cout << "Developed 256x128 sRGB image with a radial local adjustment\n";
    return 0;
}
