#include <iiFileProviderStorage.h>
#include <iiGraphicAdjust/iiGraphicAdjust.h>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: iiGraphicAdjustExample output.ppm\n"; return 2; }
    iiGraphicAdjust::Image image{256,128,{}};
    image.pixels.reserve(image.width*image.height);
    for (std::size_t y=0;y<image.height;++y)
        for (std::size_t x=0;x<image.width;++x)
            image.pixels.push_back({float(x)/255,float(y)/127,.35F,1});
    iiGraphicAdjust::ColorAdjustment settings;
    settings.exposure=.5; settings.contrast=.15; settings.saturation=.2;
    const auto adjusted=iiGraphicAdjust::adjust(image.view(),settings);
    if (!adjusted) { std::cerr << adjusted.error().message << '\n'; return 1; }
    const auto bytes=iiGraphicAdjust::toRgba8(adjusted->view());
    if (!bytes) { std::cerr << bytes.error().message << '\n'; return 1; }
    iiFileProvider::native::OutputFile output(argv[1],std::ios::binary);
    output << "P6\n256 128\n255\n";
    for (std::size_t i=0;i<bytes->size();i+=4)
        output.write(reinterpret_cast<const char*>(bytes->data()+i),3);
    if (!output) { std::cerr << "Could not write image\n"; return 1; }
}
