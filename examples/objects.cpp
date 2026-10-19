#include <iiFileProviderStorage.h>
#include <iiGraphicAdjust/Objects.h>
#include <iiGraphicAdjust/Image.h>
#include <cmath>
#include <iostream>

int main(int argc,char **argv) {
    using namespace iiGraphicAdjust;
    namespace fx=iiGraphicAdjust::objects;
    Image source{240,160,std::vector<Rgba>(240*160)};
    for(std::size_t y=0;y<source.height;++y) for(std::size_t x=0;x<source.width;++x) {
        const auto u=float(x)/239,v=float(y)/159;
        const auto ring=std::hypot(double(u)-.6,double(v)-.45)<.22;
        source.pixels[y*source.width+x]=ring?Rgba{.75F,.24F+.3F*v,.12F,1}:
            Rgba{.06F+.32F*u,.18F+.38F*v,.3F+.22F*u,1};
    }
    fx::Exposure exposure; exposure.options.stops=.55;
    fx::ColorGrading grading; grading.options.shadows={215,18,0};
    grading.options.highlights={35,12,0};
    fx::Sharpening detail; detail.amount=60;detail.radius=1.5;detail.detail=45;
    OperationStack global{{{Operation{exposure}},{Operation{grading}},{Operation{detail}}}};
    auto color=global.apply(source.view()); if(!color) return 1;
    fx::RadialMask radial;radial.options={{.6,.45},.27,.2,0,70};
    auto mask=radial.generate(source.view());if(!mask)return 2;
    fx::DevelopBlackAndWhite monochrome;monochrome.bands[0]=25;
    OperationStack local{{{Operation{monochrome},true,1,std::move(*mask)}}};
    auto selected=local.apply(color->view());if(!selected)return 3;
    fx::Watercolor watercolor;
    auto art=watercolor.apply(color->view());if(!art)return 4;
    Image gallery{960,160,std::vector<Rgba>(960*160)};
    const Image *panels[]{&source,&*color,&*selected,&*art};
    for(std::size_t y=0;y<160;++y) for(std::size_t panel=0;panel<4;++panel)
        for(std::size_t x=0;x<240;++x)
            gallery.pixels[y*960+panel*240+x]=panels[panel]->pixels[y*240+x];
    auto rgba=toRgba8(gallery.view());if(!rgba)return 5;
    const std::string path=argc>1?argv[1]:"build/operation-objects.ppm";
    iiFileProvider::native::OutputFile output(path,std::ios::binary);
    output<<"P6\n960 160\n255\n";
    for(std::size_t i=0;i<rgba->size();i+=4)
        output.write(reinterpret_cast<const char *>(rgba->data()+i),3);
    if(!output)return 6;
    std::cout<<"Wrote original, global correction, masked monochrome and watercolor panels to "<<path<<'\n';
    return 0;
}
