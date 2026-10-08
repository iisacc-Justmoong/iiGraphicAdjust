#include <iiGraphicAdjust/Hdr.h>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

using namespace iiGraphicAdjust;
int main() {
    int failures=0;
    const auto check=[&](bool valid,const char* reason) { if(!valid) { std::cerr<<"FAIL: "<<reason<<'\n'; ++failures; } };
    const auto near=[](double a,double b) { return std::abs(a-b)<2e-6; };
    const std::array<HdrRgba,4> source{{{0,0,0,.2F},{1,1,1,.4F},{4,4,4,.7F},{20,20,20,0}}};
    const HdrImageView view{4,1,source};
    const auto compressed=hdrToning(view,{HdrToningMethod::HighlightCompression});
    check(compressed && near(compressed->pixels[1].r,.73535698) && near(compressed->pixels[2].r,.90633175),
          "Highlight compression maps linear gray 1 and 4 to encoded 0.5 and 0.8");
    check(compressed && compressed->pixels[0].a==.2F && compressed->pixels[2].a==.7F &&
          compressed->pixels[3]==Rgba{0,0,0,0},"HDR alpha is preserved and invisible HDR colors canonicalized");
    const auto exposed=hdrToning(view,{HdrToningMethod::ExposureGamma,-2,1});
    check(exposed && near(exposed->pixels[1].r,.53709873) && exposed->pixels[2].r==1,
          "HDR exposure scales values above one before clipping");
    const std::array<HdrRgba,1> huge{{{std::numeric_limits<float>::max(),0,0,1}}};
    const auto stable=hdrToning(HdrImageView{1,1,huge},{HdrToningMethod::ExposureGamma,16,.01});
    check(stable && stable->pixels[0].r==1 && std::isfinite(stable->pixels[0].g),"Extreme HDR and gamma do not overflow the result");
    const Image sdr{2,1,{{.2F,.6F,.9F,.4F},{.7F,.2F,.1F,0}}};
    const auto neutral=hdrToning(sdr.view());
    check(neutral && neutral->pixels==sdr.pixels && neutral->pixels.data()!=sdr.pixels.data(),"Neutral SDR toning is a detached exact copy");
    auto invalid=huge; invalid[0].r=-1;
    check(!hdrToning(HdrImageView{1,1,invalid}),"Negative HDR RGB is rejected");
    invalid=huge; invalid[0].r=std::numeric_limits<float>::infinity();
    check(!hdrToning(HdrImageView{1,1,invalid}),"Infinite HDR RGB is rejected");
    check(!hdrToning(HdrImageView{2,1,huge}),"HDR dimensions must match buffer size");
    check(!hdrToning(view,{static_cast<HdrToningMethod>(99)}),"Unknown HDR methods are rejected");
    check(!hdrToning(sdr.view(),{HdrToningMethod::ExposureGamma,0,0}),"Invalid gamma is rejected on SDR input");
    return failures?1:0;
}
