#include <iiGraphicAdjust/Tone.h>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

using namespace iiGraphicAdjust;
namespace {
int failures=0;
void check(bool condition,std::string_view message) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool near(double a,double b,double tolerance=2e-6) { return std::abs(a-b) <= tolerance; }
bool nearPixel(Rgba actual,Rgba expected) {
    return near(actual.r,expected.r) && near(actual.g,expected.g) &&
           near(actual.b,expected.b) && actual.a == expected.a;
}
Image single(Rgba pixel) { return {1,1,{pixel}}; }
template<class T> void error(const Result<T>& value,ErrorCode code,std::string_view message) {
    check(!value && value.error().code == code && !value.error().message.empty(),message);
}
void brightness() {
    const auto image=single({.2F,.5F,.8F,.6F});
    const auto result=brightnessContrast(image.view(),{.1,.5});
    check(result && nearPixel(result->pixels[0],{.2F,.65F,1.F,.6F}),
          "Brightness offset precedes midpoint contrast and clipping");
    const auto flat=brightnessContrast(image.view(),{0,-1});
    check(flat && nearPixel(flat->pixels[0],{.5F,.5F,.5F,.6F}),"Minimum contrast collapses to midgray");
    const auto identity=brightnessContrast(image.view());
    check(identity && identity->pixels == image.pixels,"Default brightness and contrast are exact identity");
}
void leveling() {
    const auto image=single({.35F,.5F,.8F,.6F});
    LevelsOptions setting;
    setting.master={.2,.8,2,.1,.9};
    const auto result=levels(image.view(),setting);
    check(result && nearPixel(result->pixels[0],{.5F,.6656854F,.9F,.6F}),
          "Levels stretches input, applies reciprocal gamma, then maps output endpoints");
    setting={}; setting.master.outputBlack=.25; setting.master.outputWhite=.75;
    setting.red={.25,.75,1,1,0};
    const auto composed=levels(single({.25F,.5F,.75F,.2F}).view(),setting);
    check(composed && nearPixel(composed->pixels[0],{.75F,.5F,.625F,.2F}),
          "Master levels precede channel levels, with reversible output endpoints");
    check(levels(image.view())->pixels == image.pixels,"Neutral levels are exact identity");
}
void curving() {
    CurvesOptions setting;
    setting.master.points={{0,0},{.5,.75},{1,1}};
    setting.red.points={{0,1},{1,0}};
    const auto result=curves(single({.25F,.5F,.75F,.4F}).view(),setting);
    check(result && nearPixel(result->pixels[0],{.625F,.75F,.875F,.4F}),
          "Piecewise linear master and individual curves compose deterministically");
    setting={}; setting.master.points={{.25,.1},{.75,.9}};
    const auto endpoints=curves(single({0,.5F,1,.6F}).view(),setting);
    check(endpoints && nearPixel(endpoints->pixels[0],{.1F,.5F,.9F,.6F}),
          "Values outside the supplied curve domain use the nearest endpoint output");
    const auto image=single({.12345F,.6789F,.98765F,.3F});
    setting={}; setting.red.points={{0,0},{.3,.3},{.8,.8},{1,1}};
    const auto identity=curves(image.view(),setting);
    check(identity && identity->pixels == image.pixels,"Diagonal curves of any valid point count are exact identity");
}
void exposing() {
    const auto image=single({.5F,.5F,.5F,.4F});
    const auto oneStop=exposure(image.view(),{1,0,1});
    check(oneStop && nearPixel(oneStop->pixels[0],{.6858361F,.6858361F,.6858361F,.4F}),
          "A one-stop exposure doubles linear-light energy");
    const auto offset=exposure(single({0,0,0,.2F}).view(),{0,.25,1});
    check(offset && nearPixel(offset->pixels[0],{.5370987F,.5370987F,.5370987F,.2F}),
          "Exposure offset is additive in linear light");
    const auto gamma=exposure(single({0,0,0,1}).view(),{0,.0625,2});
    check(gamma && nearPixel(gamma->pixels[0],{.5370987F,.5370987F,.5370987F,1}),
          "Exposure gamma operates in linear light after offset");
    check(exposure(image.view())->pixels == image.pixels,"Neutral exposure bypasses transfer conversion exactly");
}
void shadows() {
    Image image{3,1,{{.25F,.25F,.25F,.3F},{.5F,.5F,.5F,.7F},{.75F,.75F,.75F,1}}};
    const auto result=shadowsHighlights(image.view(),{.5,.5,.5,.5});
    check(result && nearPixel(result->pixels[0],{.4375F,.4375F,.4375F,.3F}) &&
          result->pixels[1] == image.pixels[1] &&
          nearPixel(result->pixels[2],{.5625F,.5625F,.5625F,1}),
          "Smooth tonal masks lift shadows and darken highlights with a neutral middle");
    const auto overlap=shadowsHighlights(single({.5F,.5F,.5F,1}).view(),{1,1,1,1});
    check(overlap && nearPixel(overlap->pixels[0],{.375F,.375F,.375F,1}),
          "Overlapping masks use original luminance and apply lift before darkening");
    const double tiny=std::numeric_limits<double>::denorm_min();
    const auto narrow=shadowsHighlights(single({1,1,1,.4F}).view(),{0,.5,.5,tiny});
    check(narrow && nearPixel(narrow->pixels[0],{.5F,.5F,.5F,.4F}),
          "An arbitrarily narrow positive highlight width still includes pure white");
    check(shadowsHighlights(image.view())->pixels == image.pixels,"Neutral shadow/highlight amounts are exact identity");
}
void discrete() {
    const auto image=single({.2F,.5F,.8F,.4F});
    const auto reversed=invert(image.view());
    check(reversed && nearPixel(reversed->pixels[0],{.8F,.5F,.2F,.4F}),"Invert complements encoded RGB");
    const auto poster=posterize(image.view(),3);
    check(poster && nearPixel(poster->pixels[0],{0,.5F,1,.4F}),"Posterize rounds to evenly spaced levels");
    const auto boundaries=posterize(single({.25F,.75F,1,1}).view(),3);
    check(boundaries && nearPixel(boundaries->pixels[0],{.5F,1,1,1}),"Posterize halfway ties round upward");
    const auto white=threshold(single({.5F,.5F,.5F,.8F}).view());
    const auto red=threshold(single({1,0,0,.3F}).view());
    check(white && nearPixel(white->pixels[0],{1,1,1,.8F}),"Threshold includes values equal to its level");
    check(red && nearPixel(red->pixels[0],{0,0,0,.3F}),"Threshold uses encoded Rec.709 weighted luminance");
}
void histogram() {
    Image image{4,1,{{.2F,.2F,.2F,.4F},{.3F,.3F,.3F,.6F},{.3F,.3F,.3F,1},{.8F,.8F,.8F,1}}};
    const auto result=equalize(image.view());
    check(result && nearPixel(result->pixels[0],{0,0,0,.4F}) &&
          nearPixel(result->pixels[1],{2.F/3,2.F/3,2.F/3,.6F}) &&
          nearPixel(result->pixels[3],{1,1,1,1}),"Equalize subtracts first occupied CDF count");
    const auto constant=single({.27123F,.27123F,.27123F,.5F});
    check(equalize(constant.view())->pixels == constant.pixels,"One-bin equalization retains the original float channels");
    const auto color=equalize(single({.2F,.5F,.8F,1}).view());
    check(color && nearPixel(color->pixels[0],{0,.5F,1,1}),"Equalize shares a pooled RGB transfer table");
}
void automatic() {
    Image image{5,1,{{.2F,.1F,.4F,.2F},{.3F,.15F,.45F,.4F},{.4F,.2F,.5F,.6F},
                     {.5F,.25F,.65F,.8F},{.6F,.3F,.8F,1}}};
    const auto channels=autoLevels(image.view(),{0,0});
    const auto common=autoContrast(image.view(),{0,0});
    const auto color=autoColor(image.view(),{0,0});
    check(channels && nearPixel(channels->pixels[2],{.5F,.5F,.25F,.6F}),
          "Auto Levels stretches independent RGB ranges");
    check(common && nearPixel(common->pixels[2],{3.F/7,1.F/7,4.F/7,.6F}),
          "Auto Contrast uses a single pooled range");
    check(color && nearPixel(color->pixels[2],{.5F,.5F,.5F,.6F}) &&
          near(color->pixels[1].b,std::sqrt(.125)),
          "Auto Color independently neutralizes channel medians with gamma after stretching");
    Image clipped{5,1,{{0,0,0,1},{.2F,.2F,.2F,1},{.4F,.4F,.4F,1},{.6F,.6F,.6F,1},{1,1,1,1}}};
    const auto outlier=autoLevels(clipped.view(),{.25,.25});
    check(outlier && nearPixel(outlier->pixels[2],{.5F,.5F,.5F,1}) && outlier->pixels[0].r == 0 &&
          outlier->pixels[4].r == 1,"Auto quantiles exclude selected extrema and clip outside endpoints");
    Image tied{4,1,{{.1F,.1F,.1F,1},{.1F,.1F,.1F,.5F},{.1F,.1F,.1F,1},{.9F,.9F,.9F,1}}};
    const auto tiedColor=autoColor(tied.view(),{0,0});
    check(tiedColor && tiedColor->pixels[1].r == 0 && tiedColor->pixels[3].r == 1,
          "An endpoint median keeps the finite stretch instead of an undefined gamma");
    const auto tiedClip=autoLevels(tied.view(),{.4,.4});
    check(tiedClip && tiedClip->pixels == tied.pixels,"Collapsed quantile endpoints preserve that channel");
    const auto constant=single({.28F,.28F,.28F,.5F});
    check(autoLevels(constant.view())->pixels == constant.pixels &&
          autoContrast(constant.view())->pixels == constant.pixels &&
          autoColor(constant.view())->pixels == constant.pixels,
          "Neutral constant images remain exactly unchanged for all auto modes");
    const auto constantColor=single({.2F,.5F,.8F,.4F});
    check(autoColor(constantColor.view())->pixels == constantColor.pixels,
          "Auto Color preserves channels without a measurable range");
}
void validation() {
    const auto image=single({.2F,.5F,.8F,.6F});
    const double nan=std::numeric_limits<double>::quiet_NaN();
    error(brightnessContrast(image.view(),{nan,0}),ErrorCode::InvalidParameter,"NaN brightness is rejected");
    error(brightnessContrast(image.view(),{0,1.01}),ErrorCode::InvalidParameter,"Contrast bounds are enforced");
    LevelsOptions lv; lv.blue.inputWhite=0;
    error(levels(image.view(),lv),ErrorCode::InvalidParameter,"Equal input endpoints are rejected");
    lv={}; lv.master.gamma=0;
    error(levels(image.view(),lv),ErrorCode::InvalidParameter,"Nonpositive level gamma is rejected");
    lv={}; lv.red.outputWhite=nan;
    error(levels(image.view(),lv),ErrorCode::InvalidParameter,"NaN output endpoints are rejected");
    for (const auto& points : std::array<std::vector<CurvePoint>,5>{{
            {},{{.5,.5}},{{0,0},{0,1}},{{1,1},{0,0}},{{0,0},{nan,1}}}}) {
        CurvesOptions c; c.red.points=points;
        error(curves(image.view(),c),ErrorCode::InvalidParameter,"Curve point count, ordering, duplicates, and NaN are rejected");
    }
    CurvesOptions excessive; excessive.master.points.resize(65537);
    error(curves(image.view(),excessive),ErrorCode::InvalidParameter,"Unbounded curve point storage is rejected");
    error(exposure(image.view(),{17,0,1}),ErrorCode::InvalidParameter,"Exposure stops are bounded");
    error(exposure(image.view(),{0,nan,1}),ErrorCode::InvalidParameter,"NaN exposure offset is rejected");
    error(exposure(image.view(),{0,0,0}),ErrorCode::InvalidParameter,"Exposure gamma must be positive");
    error(shadowsHighlights(image.view(),{0,0,0,.5}),ErrorCode::InvalidParameter,"Zero shadow width is rejected");
    error(shadowsHighlights(image.view(),{nan,0,.5,.5}),ErrorCode::InvalidParameter,"NaN shadow amount is rejected");
    error(posterize(image.view(),1),ErrorCode::InvalidParameter,"Posterize needs two or more levels");
    error(posterize(image.view(),65537),ErrorCode::InvalidParameter,"Posterize level count is bounded");
    error(threshold(image.view(),nan),ErrorCode::InvalidParameter,"NaN threshold is rejected");
    error(threshold(image.view(),-1),ErrorCode::InvalidParameter,"Threshold is normalized");
    error(autoLevels(image.view(),{.5,.5}),ErrorCode::InvalidParameter,"Clipped fractions cannot consume the whole range");
    error(autoContrast(image.view(),{nan,0}),ErrorCode::InvalidParameter,"NaN auto clipping is rejected");
    error(autoColor(image.view(),{0,-1}),ErrorCode::InvalidParameter,"Negative auto clipping is rejected");
    const std::array<std::pair<std::string_view,Result<Image>(*)(ImageView)>,2> simple{{{"invert",invert},{"equalize",equalize}}};
    for (const auto& [name,operation] : simple) {
        error(operation({}),ErrorCode::InvalidDimensions,name);
        auto invalid=image; invalid.pixels[0].g=std::numeric_limits<float>::infinity();
        error(operation(invalid.view()),ErrorCode::InvalidPixel,name);
    }
    error(levels({2,1,image.pixels}),ErrorCode::InvalidBuffer,"Tonal operations reject truncated buffers");
    auto bad=image; bad.pixels[0].a=std::numeric_limits<float>::quiet_NaN();
    const std::array<Result<Image>,12> invalidPixels{{brightnessContrast(bad.view()),levels(bad.view()),curves(bad.view()),
        exposure(bad.view()),shadowsHighlights(bad.view()),invert(bad.view()),posterize(bad.view(),3),threshold(bad.view()),
        equalize(bad.view()),autoLevels(bad.view()),autoContrast(bad.view()),autoColor(bad.view())}};
    for (const auto& result : invalidPixels) error(result,ErrorCode::InvalidPixel,"Every tone operation rejects nonfinite alpha");
}
void invariants() {
    const Image image{3,1,{{.8F,.1F,.4F,0},{.2F,.6F,.9F,.35F},{.7F,.3F,.4F,1}}};
    const auto before=image.pixels;
    CurvesOptions curve; curve.master.points={{0,1},{1,0}};
    const std::array<Result<Image>,12> outputs{{brightnessContrast(image.view(),{.3,.7}),levels(image.view()),
        curves(image.view(),curve),exposure(image.view(),{1,.1,1.2}),shadowsHighlights(image.view(),{.7,.3}),
        invert(image.view()),posterize(image.view(),5),threshold(image.view()),equalize(image.view()),
        autoLevels(image.view()),autoContrast(image.view()),autoColor(image.view())}};
    for (const auto& output : outputs) {
        check(output.has_value(),"Every tone function accepts normalized RGBA");
        if (!output) continue;
        check(output->width == image.width && output->height == image.height,"Dimensions are retained");
        check(output->pixels.data() != image.pixels.data(),"Result storage is detached");
        check(output->pixels[0] == image.pixels[0],"Hidden zero-alpha RGB remains byte-for-byte unchanged");
        for (std::size_t i=0;i<image.pixels.size();++i) {
            const auto p=output->pixels[i];
            check(p.a == image.pixels[i].a,"Straight alpha is preserved exactly");
            check(std::isfinite(p.r) && p.r >= 0 && p.r <= 1 && std::isfinite(p.g) && p.g >= 0 && p.g <= 1 &&
                  std::isfinite(p.b) && p.b >= 0 && p.b <= 1,"Results remain finite and normalized");
        }
    }
    check(image.pixels == before,"All input pixels remain immutable");
    const Image visible{2,1,{image.pixels[1],image.pixels[2]}};
    const auto isolated=autoContrast(visible.view(),{0,0});
    const auto hidden=autoContrast(image.view(),{0,0});
    check(isolated && hidden && isolated->pixels[0] == hidden->pixels[1] && isolated->pixels[1] == hidden->pixels[2],
          "Zero-alpha colors never influence automatic statistics");
    const Image transparent{1,1,{{.24F,.75F,.4F,0}}};
    check(equalize(transparent.view())->pixels == transparent.pixels && autoColor(transparent.view())->pixels == transparent.pixels,
          "Statistics-based operations copy all-transparent images exactly");
    const std::array<Result<Image>,12> invalid{{brightnessContrast({}),levels({}),curves({}),exposure({}),
        shadowsHighlights({}),invert({}),posterize({},3),threshold({}),equalize({}),autoLevels({}),autoContrast({}),autoColor({})}};
    for (const auto& output : invalid) error(output,ErrorCode::InvalidDimensions,"Every tone operation validates image dimensions");
}
} // namespace
int main(int argc,char** argv) {
    const std::string_view selected=argc > 1 ? argv[1] : "all";
    const std::array<std::pair<std::string_view,void(*)()>,10> cases{{{"brightness",brightness},
        {"levels",leveling},{"curves",curving},{"exposure",exposing},
        {"shadows",shadows},{"discrete",discrete},{"histogram",histogram},{"automatic",automatic},
        {"validation",validation},{"invariants",invariants}}};
    bool found=false;
    for (const auto& [name,operation] : cases) if (selected == "all" || selected == name) {
        found=true; operation();
    }
    if (!found) { std::cerr << "Unknown test case\n"; return 2; }
    if (failures) return 1;
    std::cout << "Tone " << selected << ": passed\n";
}
