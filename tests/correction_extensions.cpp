#include <iiGraphicAdjust/Objects.h>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

using namespace iiGraphicAdjust;
namespace o=iiGraphicAdjust::objects;
namespace {
int failures=0;
void check(bool ok,const char *message) { if(!ok) { std::cerr<<message<<'\n'; ++failures; } }
double linear(double v) { return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4); }
float encoded(double v) { return static_cast<float>(v<=.0031308?12.92*v:1.055*std::pow(v,1/2.4)-.055); }
void normalized(const Image &image) {
    check(image.pixels.size()==image.width*image.height,"geometry and buffer agree");
    for(auto p:image.pixels) for(auto v:{p.r,p.g,p.b,p.a})
        check(std::isfinite(v)&&v>=0&&v<=1,"output is finite and normalized");
}
void alpha() {
    Image source{3,1,{{.7F,.7F,.7F,.5F},{1,1,1,1},{.2F,.3F,.4F,0}}};
    auto out=o::ColorToAlpha{}.apply(source.view());
    check(out && std::abs(out->pixels[0].a-.15F)<1e-7 && out->pixels[0].r==0,
          "matte removal unmixed edge and multiplies preexisting alpha");
    check(out && out->pixels[1].a==0 && out->pixels[2]==source.pixels[2],
          "key vanishes and preexisting hidden pixels remain unchanged");
    std::mt19937 random(19263);
    for(int n=0;n<120;++n) {
        ColorToAlphaOptions options;
        for(auto &v:options.color) v=double(random()%10001)/10000;
        Rgba p{float(random()%10001)/10000,float(random()%10001)/10000,
               float(random()%10001)/10000,float(1+random()%10000)/10000};
        Image one{1,1,{p}};
        auto result=colorToAlpha(one.view(),options); check(bool(result),"random unmix succeeds");
        if(!result) continue;
        normalized(*result); const auto q=result->pixels[0];
        const std::array<double,3> old{p.r,p.g,p.b},now{q.r,q.g,q.b};
        for(int k=0;k<3;++k)
            check(std::abs(now[k]*q.a+(1-q.a)*options.color[k]-
                           (old[k]*p.a+(1-p.a)*options.color[k]))<2e-7,
                  "independent recomposition over the removed key recovers the source");
    }
    auto clipped=colorToAlpha(source.view(),{{1,1,1},.4,.8});
    check(clipped && clipped->pixels[0].a==0,"transparency threshold removes near-key colors");
    check(bool(colorToAlpha(source.view(),{{1,1,1},.8,.2})),"crossed thresholds remain finite");
}
void flat() {
    Image reference{3,1,{{encoded(.1),encoded(.2),encoded(.3),1},
                         {encoded(.2),encoded(.4),encoded(.6),1},
                         {encoded(.3),encoded(.6),encoded(.9),1}}};
    Image source=reference;
    for(auto &p:source.pixels) { p.r=encoded(linear(p.r)*.4);p.g=encoded(linear(p.g)*.4);p.b=encoded(linear(p.b)*.4); }
    o::FlatFieldCorrection correct; correct.reference=reference;
    auto result=correct.apply(source.view());
    check(bool(result),"flat field correction succeeds");
    if(result) for(auto p:result->pixels)
        check(std::abs(linear(p.r)-.08)<2e-7 && std::abs(linear(p.g)-.16)<2e-7 &&
              std::abs(linear(p.b)-.24)<2e-7,"known uniform scene is recovered from vignetting");
    correct.options.amount=0;
    auto neutral=correct.apply(source.view());
    check(neutral && neutral->pixels==source.pixels,"zero strength is an exact copy");
    correct.options.amount=.5;
    auto half=correct.apply(source.view());
    check(half && std::abs(linear(half->pixels[0].r)-.06)<2e-7,"flat strength mixes linear gains");
    reference.pixels[0].a=0;
    check(!flatFieldCorrection(source.view(),reference.view()),"holes in reference coverage are rejected");
    check(!flatFieldCorrection(source.view(),{1,1,{source.pixels.data(),1}}),"reference geometry must match");
    source.pixels[0]={.9F,.8F,.7F,0};
    auto hidden=correct.apply(source.view());
    check(hidden && hidden->pixels[0]==source.pixels[0],"flat correction preserves hidden RGB");
}
void fills() {
    Image source{3,1,{{.1F,.2F,.3F,.5F},{0,0,0,1},{1,0,0,0}}};
    auto solid=solidFill(source.view(),{{.6F,.2F,.4F,.4F},true});
    check(solid && solid->pixels[0]==Rgba{.6F,.2F,.4F,.2F},"solid fill multiplies source coverage");
    GradientFillOptions gradient;
    auto ramp=gradientFill(source.view(),gradient);
    check(ramp && std::abs(ramp->pixels[1].r-.735356983F)<2e-7 && ramp->pixels[1].a==1,
          "linear gradient's midpoint is independently transfer encoded");
    gradient.space=CorrectionSpace::Srgb;
    auto encodedRamp=gradientFill(source.view(),gradient);
    check(encodedRamp && encodedRamp->pixels[1].r==.5F,"encoded gradient is explicit");
    gradient.reverse=true;
    auto reverse=gradientFill(source.view(),gradient);
    check(reverse && encodedRamp && std::abs(reverse->pixels[0].r+encodedRamp->pixels[0].r-1)<1e-7,
          "gradient reverse complements position");
    for(auto style:{GradientFillStyle::Radial,GradientFillStyle::Angle,GradientFillStyle::Reflected,GradientFillStyle::Diamond}) {
        gradient.style=style; auto r=gradientFill(source.view(),gradient);check(bool(r),"all gradient geometries execute");
        if(r) normalized(*r);
    }
    Image tile{2,1,{{1,0,0,1},{0,1,0,1}}};
    o::PatternFill pattern; pattern.pattern=tile;
    auto repeated=pattern.apply(source.view());
    check(repeated && repeated->pixels[0].r==1 && repeated->pixels[1].g==1 &&
          repeated->pixels[2].r==1 && repeated->pixels[2].a==0,"pattern wraps at pixel centers");
    pattern.options.offsetX=1e9;pattern.options.scale=.01;
    auto huge=pattern.apply(source.view());check(bool(huge),"large bounded pattern offsets are safe");
    if(huge) normalized(*huge);
}
Image detailFixture() {
    Image source{19,11,std::vector<Rgba>(19*11)};
    for(std::size_t y=0;y<11;++y) for(std::size_t x=0;x<19;++x) {
        float v=static_cast<float>(.22+.018*x+.04*std::sin(x*1.8+y*.7));
        source.pixels[y*19+x]={v,v,v,1};
    }
    source.pixels[0]={.1F,.7F,.9F,0};return source;
}
double difference(const Image&a,const Image&b) {
    double sum=0;for(std::size_t i=0;i<a.pixels.size();++i)sum+=std::abs(a.pixels[i].r-b.pixels[i].r);return sum;
}
void detail() {
    auto source=detailFixture();
    o::Sharpening sharpen;sharpen.amount=80;
    auto a=sharpen.apply(source.view());sharpen.radius=2;
    auto b=sharpen.apply(source.view());sharpen.detail=90;
    auto c=sharpen.apply(source.view());
    check(a&&b&&c,"all sharpening controls render");
    check(a&&b&&difference(*a,*b)>1e-4,"radius has an actual spatial effect");
    check(b&&c&&difference(*b,*c)>1e-4,"fine detail has an actual spatial effect");
    o::LuminanceNoiseReduction noise;noise.amount=80;
    auto n=noise.apply(source.view());noise.contrast=100;
    auto restored=noise.apply(source.view());
    check(n&&restored&&difference(*n,*restored)>1e-5,"NR contrast is an active correction");
    for(const auto *r:{&a,&b,&c,&n,&restored}) if(*r) {
        normalized(**r);check((*r)->pixels[0]==source.pixels[0],"new detail controls preserve hidden color");
    }
}
void masksAndAnalysis() {
    auto source=detailFixture();
    auto linearMaskResult=o::LinearMask{}.generate(source.view());
    auto radial=o::RadialMask{}.generate(source.view());
    check(linearMaskResult&&radial,"mask objects generate owned coverage");
    if(radial) {
        o::InvertMask invert;auto complement=invert.apply(radial->view());
        check(complement && std::abs(complement->coverage[100]+radial->coverage[100]-1)<1e-7,"mask complement is correct");
        o::ApplyLocalAdjustments local;
        local.adjustments.push_back({*radial,{}});local.adjustments[0].options.exposure=.5;
        Operation frozen{local};local.adjustments.clear();
        auto out=frozen.apply(source.view());check(out&&difference(*out,source)>1e-5,"local correction owns its mask");
    }
    auto histogram=o::Histogram{}.analyze(source.view());
    check(histogram && histogram->contributingPixels==source.pixels.size()-1,"analysis object excludes hidden samples");
    auto automatic=o::AutoDevelopLight{}.apply(source.view());check(bool(automatic),"auto adjustment object estimates and applies");
    auto bw=o::AutoBlackAndWhiteMix{}.apply(source.view());check(bool(bw),"automatic monochrome object renders");
    const std::array<std::uint16_t,4> samples{16384,32768,32768,49152};
    auto raw=o::RawDevelopment{}.apply({2,2,2,samples});
    check(raw && raw->pixels.size()==4,"RAW development has a correctly typed sensor input");
    auto rendered=o::RawRender{}.apply({2,2,2,samples});check(bool(rendered),"RAW rendering object returns encoded output");
    if(raw) check(bool(o::HdrToning{}.apply(raw->view())),"HDR object retains the high-headroom input overload");
}
void validation() {
    auto source=detailFixture();
    const auto nan=std::numeric_limits<double>::quiet_NaN();
    check(!colorToAlpha(source.view(),{{nan,1,1},0,1}),"invalid key color rejected");
    GradientFillOptions gradient;gradient.stops[1].position=0;
    check(!gradientFill(source.view(),gradient),"duplicate gradient knots rejected");
    gradient={};gradient.style=static_cast<GradientFillStyle>(99);
    check(!gradientFill(source.view(),gradient),"unknown gradient type rejected");
    PatternFillOptions pattern;pattern.scale=0;
    check(!patternFill(source.view(),source.view(),pattern),"invalid pattern scale rejected");
    for(int control=0;control<3;++control) {
        DevelopDetailOptions d;
        if(control==0)d.sharpenRadius=nan;
        if(control==1)d.sharpenDetail=101;
        if(control==2)d.luminanceContrast=-1;
        check(!developDetail(source.view(),d),"inactive new detail controls are validated");
    }
    check(!o::ColorLookup{}.apply(source.view()),"missing required LUT is an explicit error");
    o::PointColor point;point.samples.resize(9);
    check(!point.apply(source.view()),"Point Color sample count is bounded before allocation");
}
}
int main(int argc,char **argv) {
    const std::string_view which=argc>1?argv[1]:"all";
    if(which=="all"||which=="alpha")alpha();
    if(which=="all"||which=="flat")flat();
    if(which=="all"||which=="fills")fills();
    if(which=="all"||which=="detail")detail();
    if(which=="all"||which=="analysis")masksAndAnalysis();
    if(which=="all"||which=="validation")validation();
    return failures?1:0;
}
