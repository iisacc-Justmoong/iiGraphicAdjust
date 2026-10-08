#include <iiGraphicAdjust/Color.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

using namespace iiGraphicAdjust;
namespace {
int failures=0;
void check(bool condition,std::string_view message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
bool near(double a,double b,double tolerance=2e-6) { return std::abs(a-b)<=tolerance; }
bool nearPixel(Rgba a,Rgba b) { return near(a.r,b.r)&&near(a.g,b.g)&&near(a.b,b.b)&&a.a==b.a; }
Image single(Rgba p) { return {1,1,{p}}; }
double luma(Rgba p) { return .2126*p.r+.7152*p.g+.0722*p.b; }
template<class T> void error(const Result<T>& r,ErrorCode code,std::string_view message) {
    check(!r && r.error().code==code && !r.error().message.empty(),message);
}
void hue() {
    const Image input{3,1,{{1,0,0,.4F},{0,0,1,1},{.5F,.5F,.5F,1}}};
    auto options=HueSaturationOptions{}; options.hue=120;
    const auto rotated=hueSaturation(input.view(),options);
    check(rotated && nearPixel(rotated->pixels[0],{0,1,0,.4F}) &&
          nearPixel(rotated->pixels[1],{1,0,0,1}),"120 degrees rotates primaries through HSL");
    options.range=HueRange::Reds;
    const auto selected=hueSaturation(input.view(),options);
    check(selected && nearPixel(selected->pixels[0],{0,1,0,.4F}) && selected->pixels[1]==input.pixels[1] &&
          selected->pixels[2]==input.pixels[2],"Hue selection excludes blue and achromatic pixels");
    options={}; options.saturation=-1;
    const auto gray=hueSaturation(input.view(),options);
    check(gray && nearPixel(gray->pixels[0],{.5F,.5F,.5F,.4F}),"Full HSL desaturation retains HSL lightness");
    options={}; options.colorize=true; options.hue=-120; options.saturation=1;
    const auto colorized=hueSaturation(single({.5F,.5F,.5F,.7F}).view(),options);
    check(colorized && nearPixel(colorized->pixels[0],{0,0,1,.7F}),"Colorize sets hue and absolute saturation");
    options={}; options.lightness=.5;
    const auto light=hueSaturation(single({0,0,0,1}).view(),options);
    check(light && nearPixel(light->pixels[0],{.5F,.5F,.5F,1}),"Positive lightness interpolates toward white");
    options={}; options.hue=120; options.range=HueRange::Reds;
    const auto shoulder=hueSaturation(single({1,.75F,0,1}).view(),options);
    check(shoulder && nearPixel(shoulder->pixels[0],{.5F,.875F,.375F,1}),
          "45-degree input receives the analytic half-weight hue falloff");
    options={}; options.saturation=1;
    const auto keepGray=hueSaturation(single({.25F,.25F,.25F,1}).view(),options);
    check(keepGray && keepGray->pixels[0]==Rgba{.25F,.25F,.25F,1},"Saturation leaves undefined gray hue achromatic");
    options={}; options.hue=120;
    for (const float tiny:{1e-20F,std::numeric_limits<float>::denorm_min()}) {
        const auto darkRotation=hueSaturation(single({tiny,0,0,.5F}).view(),options);
        check(darkRotation && darkRotation->pixels[0]==Rgba{0,tiny,0,.5F},
              "Hue rotation preserves actual chroma for tiny and subnormal RGB");
    }
}
void vibranceCase() {
    const Image input{3,1,{{.625F,.375F,.375F,1},{1,0,0,1},{.5F,.5F,.5F,.5F}}};
    const auto output=vibrance(input.view(),{1,0});
    check(output && nearPixel(output->pixels[0],{.71875F,.28125F,.28125F,1}),
          "Muted HSL saturation .25 gains v*s*(1-s)=.1875");
    check(output && output->pixels[1]==input.pixels[1] && output->pixels[2]==input.pixels[2],
          "Vibrance preserves fully saturated and achromatic colors");
    const auto negative=vibrance(input.view(),{-1,0});
    check(negative && nearPixel(negative->pixels[0],{.53125F,.46875F,.46875F,1}),
          "Negative vibrance follows its documented signed delta");
    const auto gray=vibrance(input.view(),{0,-1});
    check(gray && nearPixel(gray->pixels[0],{.5F,.5F,.5F,1}),"Global saturation can fully remove chroma");
}
void balance() {
    ColorBalanceOptions options; options.preserveLuminosity=false; options.midtones.cyanRed=1;
    const auto result=colorBalance(single({.5F,.5F,.5F,.4F}).view(),options);
    check(result && nearPixel(result->pixels[0],{.75F,.5F,.5F,.4F}),
          "Midtone weight 2*y*(1-y)=.5 gives a .25 red offset");
    options.preserveLuminosity=true;
    const auto preserved=colorBalance(single({.5F,.5F,.5F,.4F}).view(),options);
    check(preserved && nearPixel(preserved->pixels[0],{.69685F,.44685F,.44685F,.4F}),
          "Luminosity preservation subtracts the encoded-luma change");
    options={}; options.shadows.cyanRed=1; options.highlights.yellowBlue=-1;
    for (const auto p:std::array<Rgba,4>{{{.1F,.2F,.3F,1},{.8F,.9F,.7F,.5F},{0,0,0,1},{1,1,1,1}}}) {
        const auto output=colorBalance(single(p).view(),options);
        check(output && near(luma(output->pixels[0]),luma(p),2e-7),
              "Gamut compression preserves original luminosity even at channel boundaries");
    }
}
void blackWhite() {
    const Image primaries{6,1,{{1,0,0,1},{1,1,0,1},{0,1,0,1},{0,1,1,1},{0,0,1,1},{1,0,1,1}}};
    const auto output=blackAndWhite(primaries.view());
    const std::array<double,6> expected{.4,.6,.4,.6,.2,.8};
    if (output) for (std::size_t i=0;i<6;++i)
        check(near(output->pixels[i].r,expected[i]) && output->pixels[i].r==output->pixels[i].g &&
              output->pixels[i].g==output->pixels[i].b,"Six hue controls map primary/secondary reference colors");
    else check(false,"Black and white accepts normalized primaries");
    const auto interpolated=blackAndWhite(single({1,.5F,0,.7F}).view());
    check(interpolated && nearPixel(interpolated->pixels[0],{.5F,.5F,.5F,.7F}),
          "Orange interpolates red and yellow grayscale coefficients");
    BlackWhiteOptions options; options.tint=true; options.tintHue=0; options.tintSaturation=1;
    const auto tinted=blackAndWhite(single({.5F,.5F,.5F,1}).view(),options);
    check(tinted && nearPixel(tinted->pixels[0],{1,0,0,1}),"Tint colorizes grayscale through HSL lightness");
    options={}; options.weights[0]=-2;
    const auto clipped=blackAndWhite(single({1,0,0,1}).view(),options);
    check(clipped && nearPixel(clipped->pixels[0],{0,0,0,1}),"Negative grayscale coefficients clip to gamut");
}
void filter() {
    PhotoFilterOptions options; options.color={1,0,0}; options.density=.5; options.preserveLuminosity=false;
    const auto output=photoFilter(single({.5F,.5F,.5F,.3F}).view(),options);
    check(output && nearPixel(output->pixels[0],{.5F,.25F,.25F,.3F}),
          "Filter density blends identity and specified channel transmittance");
    options.preserveLuminosity=true;
    const auto preserved=photoFilter(single({.5F,.5F,.5F,.3F}).view(),options);
    check(preserved && nearPixel(preserved->pixels[0],{.69685F,.44685F,.44685F,.3F}) &&
          near(luma(preserved->pixels[0]),.5),"Preserved filter luma matches its independent reference");
}
void mixer() {
    ChannelMixerOptions options; options.red={{0,0,1}}; options.blue={{1,0,0}};
    const auto swapped=channelMixer(single({.2F,.4F,.8F,.5F}).view(),options);
    check(swapped && nearPixel(swapped->pixels[0],{.8F,.4F,.2F,.5F}),"Channel coefficients can swap red and blue");
    options={}; options.monochrome=true; options.gray={{.25,.5,.25},.1};
    const auto mono=channelMixer(single({.2F,.4F,.8F,.5F}).view(),options);
    check(mono && nearPixel(mono->pixels[0],{.55F,.55F,.55F,.5F}),
          "Monochrome mixer includes all RGB coefficients and its constant");
    options={}; options.red={{-2,2,2},1};
    const auto clipped=channelMixer(single({0,1,1,1}).view(),options);
    check(clipped && clipped->pixels[0].r==1,"Mixer output clips finite extreme coefficients");
}
void selective() {
    const auto input=single({.75F,.25F,.25F,.4F});
    SelectiveColorOptions options; options.ranges[static_cast<std::size_t>(ColorRange::Reds)].cyan=.4;
    const auto relative=selectiveColor(input.view(),options);
    check(relative && nearPixel(relative->pixels[0],{.7F,.25F,.25F,.4F}),
          "Relative cyan uses red-range weight .5 and existing complement .25");
    options.method=SelectiveColorMethod::Absolute;
    const auto absolute=selectiveColor(input.view(),options);
    check(absolute && nearPixel(absolute->pixels[0],{.55F,.25F,.25F,.4F}),
          "Absolute cyan uses its weighted normalized-channel amount");
    options={}; options.ranges[static_cast<std::size_t>(ColorRange::Neutrals)].black=.2;
    const auto neutral=selectiveColor(single({.5F,.5F,.5F,1}).view(),options);
    check(neutral && nearPixel(neutral->pixels[0],{.4F,.4F,.4F,1}),
          "Neutral black adjustment is weighted by source intensity in relative mode");
    options={}; options.method=SelectiveColorMethod::Absolute;
    options.ranges[static_cast<std::size_t>(ColorRange::Whites)].yellow=.2;
    const auto white=selectiveColor(single({1,1,1,1}).view(),options);
    check(white && nearPixel(white->pixels[0],{1,1,.8F,1}),"Whites selection reaches the white endpoint");
    options={}; options.method=SelectiveColorMethod::Absolute;
    options.ranges[static_cast<std::size_t>(ColorRange::Blacks)].cyan=-.3;
    const auto black=selectiveColor(single({0,0,0,1}).view(),options);
    check(black && nearPixel(black->pixels[0],{.3F,0,0,1}),"Blacks selection reaches the black endpoint");
}
void replace() {
    const Image input{2,1,{{1,0,0,.4F},{0,0,1,.7F}}};
    ReplaceColorOptions options; options.target={1,0,0}; options.fuzziness=0; options.hue=120;
    const auto output=replaceColor(input.view(),options);
    check(output && nearPixel(output->pixels[0],{0,1,0,.4F}) && output->pixels[1]==input.pixels[1],
          "Zero fuzziness replaces an exact target and preserves distant colors");
    options={}; options.target={.5,.5,.5}; options.fuzziness=.5; options.lightness=1;
    const auto soft=replaceColor(single({.75F,.75F,.75F,1}).view(),options);
    check(soft && nearPixel(soft->pixels[0],{.875F,.875F,.875F,1}),
          "Half-radius selection receives the smoothstep half-weight replacement");
    options.softness=0;
    const auto hard=replaceColor(single({.75F,.75F,.75F,1}).view(),options);
    check(hard && nearPixel(hard->pixels[0],{1,1,1,1}),"Zero softness uses a hard distance selection");
    options={}; options.target={.5,.5,.5}; options.saturation=1; options.hue=120;
    const auto achromatic=replaceColor(single({.5F,.5F,.5F,1}).view(),options);
    check(achromatic && achromatic->pixels[0]==Rgba{.5F,.5F,.5F,1},
          "Replacement saturation preserves undefined achromatic hue");
}
void match() {
    const auto source=single({.2F,.3F,.4F,.7F});
    const Image reference{2,1,{{.8F,.6F,.4F,.5F},{0,1,0,0}}};
    const auto constant=matchColor(source.view(),reference.view());
    check(constant && nearPixel(constant->pixels[0],{.8F,.6F,.4F,.7F}),
          "Zero source variance transfers visible reference means and ignores hidden reference RGB");
    const Image varying{2,1,{{.2F,.3F,.4F,1},{.6F,.7F,.8F,1}}};
    const Image target{2,1,{{.1F,.2F,.3F,1},{.5F,.6F,.7F,1}}};
    const auto mapped=matchColor(varying.view(),target.view());
    check(mapped && nearPixel(mapped->pixels[0],target.pixels[0]) && nearPixel(mapped->pixels[1],target.pixels[1]),
          "Two-sample equal weights transfer linear per-channel mean and population deviation");
    MatchColorOptions options; options.fade=1;
    const auto faded=matchColor(source.view(),reference.view(),options);
    check(faded && faded->pixels==source.pixels,"Full fade preserves exact source channels");
    options={}; options.neutralize=true;
    const auto neutral=matchColor(source.view(),reference.view(),options);
    check(neutral && near(neutral->pixels[0].r,neutral->pixels[0].g) &&
          near(neutral->pixels[0].g,neutral->pixels[0].b),"Neutralize targets equal linear reference-mean luminance");
    options={}; options.colorIntensity=0;
    const auto gray=matchColor(source.view(),reference.view(),options);
    check(gray && near(gray->pixels[0].r,gray->pixels[0].g) && near(gray->pixels[0].g,gray->pixels[0].b),
          "Zero color intensity removes matched linear chroma");
    const Image alphaTarget{2,1,{{0,0,0,1},{1,1,1,.5F}}};
    const auto weighted=matchColor(source.view(),alphaTarget.view());
    // An alpha-weighted linear mean of black and white is 1/3, encoded to 0.612501.
    check(weighted && near(weighted->pixels[0].r,.6125010249),"Reference means are weighted by alpha in linear RGB");
}
void validation() {
    const auto input=single({.2F,.5F,.8F,.7F});
    const double nan=std::numeric_limits<double>::quiet_NaN();
    HueSaturationOptions h; h.hue=nan;
    error(hueSaturation(input.view(),h),ErrorCode::InvalidParameter,"Hue rejects NaN");
    h={}; h.range=static_cast<HueRange>(-1);
    error(hueSaturation(input.view(),h),ErrorCode::InvalidParameter,"Hue rejects unknown enum");
    h={}; h.colorize=true; h.saturation=-.1;
    error(hueSaturation(input.view(),h),ErrorCode::InvalidParameter,"Colorize requires absolute nonnegative saturation");
    error(vibrance(input.view(),{1.1,0}),ErrorCode::InvalidParameter,"Vibrance bounds are enforced");
    ColorBalanceOptions b; b.shadows.yellowBlue=nan;
    error(colorBalance(input.view(),b),ErrorCode::InvalidParameter,"All tone balances are validated");
    BlackWhiteOptions bw; bw.weights[5]=3.1;
    error(blackAndWhite(input.view(),bw),ErrorCode::InvalidParameter,"Six black-white controls are bounded");
    bw={}; bw.tintHue=361;
    error(blackAndWhite(input.view(),bw),ErrorCode::InvalidParameter,"Inactive tint parameters still validate");
    PhotoFilterOptions f; f.color[1]=-1; f.density=0;
    error(photoFilter(input.view(),f),ErrorCode::InvalidParameter,"Zero density still validates filter color");
    ChannelMixerOptions m; m.gray.constant=nan;
    error(channelMixer(input.view(),m),ErrorCode::InvalidParameter,"Inactive mixer parameters still validate");
    SelectiveColorOptions s; s.method=static_cast<SelectiveColorMethod>(-1);
    error(selectiveColor(input.view(),s),ErrorCode::InvalidParameter,"Selective method rejects unknown enum");
    s={}; s.ranges[8].black=2;
    error(selectiveColor(input.view(),s),ErrorCode::InvalidParameter,"Every selective range is validated");
    ReplaceColorOptions r; r.softness=-1;
    error(replaceColor(input.view(),r),ErrorCode::InvalidParameter,"Replacement softness is bounded");
    MatchColorOptions mc; mc.fade=1; mc.luminance=nan;
    error(matchColor(input.view(),input.view(),mc),ErrorCode::InvalidParameter,"Full fade still validates matching parameters");
    mc={}; mc.fade=1;
    error(matchColor(single({1,0,0,0}).view(),input.view(),mc),ErrorCode::InvalidParameter,
          "Full fade still rejects source statistics without visible samples");
    error(matchColor(input.view(),single({1,0,0,0}).view()),ErrorCode::InvalidParameter,
          "Matching rejects reference statistics without visible samples");
    error(matchColor(input.view(),{}),ErrorCode::InvalidDimensions,"Reference dimensions are validated");
    auto bad=input; bad.pixels[0].g=std::numeric_limits<float>::infinity();
    error(photoFilter(bad.view()),ErrorCode::InvalidPixel,"Invalid normalized pixels are rejected");
    error(desaturate({}),ErrorCode::InvalidDimensions,"Desaturate validates images");
    error(vibrance({2,1,input.pixels}),ErrorCode::InvalidBuffer,"Color operations reject truncated storage");
}
void invariants() {
    Image input{5,2,{{.1F,.3F,.8F,0},{1,0,0,.25F},{.5F,.5F,.5F,1},{0,1,0,.5F},{0,0,1,.75F},
                     {0,0,0,1},{1,1,1,1},{std::numeric_limits<float>::denorm_min(),0,0,1},
                     {.2F,.6F,.9F,.4F},{.8F,.4F,.1F,0}}};
    const auto original=input.pixels;
    const std::array<Result<Image>,7> neutral{
        hueSaturation(input.view()),vibrance(input.view()),colorBalance(input.view()),
        photoFilter(input.view(),{{1,0,0},0}),channelMixer(input.view()),selectiveColor(input.view()),
        replaceColor(input.view())};
    for (const auto& output:neutral) {
        check(output && output->pixels==original,"Neutral settings are exact copies including hidden colors");
        check(output && output->pixels.data()!=input.pixels.data(),"Neutral output owns detached storage");
    }
    ColorBalanceOptions cb; cb.shadows={1,-1,1}; cb.midtones={-1,1,-1}; cb.highlights={1,1,-1};
    ChannelMixerOptions cm; cm.red={{2,-2,2},1}; cm.green={{-2,2,-2},-1};
    SelectiveColorOptions sc; for (auto& range:sc.ranges) range={1,-1,1,-1};
    const std::array<Result<Image>,10> outputs{
        hueSaturation(input.view(),{180,1,-1}),vibrance(input.view(),{-1,1}),colorBalance(input.view(),cb),
        blackAndWhite(input.view(),{{-2,3,-2,3,-2,3},true,300,1}),photoFilter(input.view(),{{0,1,0},1}),
        channelMixer(input.view(),cm),selectiveColor(input.view(),sc),desaturate(input.view()),
        replaceColor(input.view(),{{0,0,0},1,1,180,1,1}),matchColor(input.view(),input.view(),{2,2,0,true})};
    for (const auto& output:outputs) {
        check(output && output->width==5 && output->height==2 && output->pixels.size()==10,
              "Color operations preserve dimensions and storage count");
        if (!output) continue;
        for (std::size_t i=0;i<10;++i) {
            const auto p=output->pixels[i];
            check(p.a==original[i].a,"Every color operation preserves alpha exactly");
            if (p.a==0) check(p==original[i],"Hidden zero-alpha RGB is copied exactly");
            for (const auto channel:{p.r,p.g,p.b,p.a})
                check(std::isfinite(channel) && channel>=0 && channel<=1,"Boundary settings retain normalized finite channels");
        }
    }
    check(input.pixels==original,"All color operations preserve input buffers");
}
} // namespace
int main(int argc,char** argv) {
    if (argc!=2) return 2;
    const std::string_view name=argv[1];
    if (name=="hue") hue(); else if (name=="vibrance") vibranceCase(); else if (name=="balance") balance();
    else if (name=="black-white") blackWhite(); else if (name=="filter") filter(); else if (name=="mixer") mixer();
    else if (name=="selective") selective(); else if (name=="replace") replace(); else if (name=="match") match();
    else if (name=="validation") validation(); else if (name=="invariants") invariants(); else return 2;
    return failures ? 1 : 0;
}
