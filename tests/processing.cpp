#include <iiGraphicAdjust/iiGraphicAdjust.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

using namespace iiGraphicAdjust;
namespace {
int failures = 0;
void check(bool condition, std::string_view message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
bool near(double a, double b, double tolerance = 2e-6) { return std::abs(a-b) <= tolerance; }
bool nearPixel(Rgba a, Rgba b) {
    return near(a.r,b.r) && near(a.g,b.g) && near(a.b,b.b) && near(a.a,b.a);
}
Image single(Rgba pixel) { return {1,1,{pixel}}; }
template<class T> void error(const Result<T>& result, ErrorCode code, std::string_view message) {
    check(!result && result.error().code == code && !result.error().message.empty(), message);
}
void validation() {
    const auto valid = single({.2F,.5F,.8F,.7F});
    error(adjust({}), ErrorCode::InvalidDimensions, "Zero dimensions are rejected");
    error(adjust({2,1,valid.pixels}), ErrorCode::InvalidBuffer, "Truncated buffers are rejected");
    error(adjust({std::numeric_limits<std::size_t>::max(),2,valid.pixels}),
          ErrorCode::InvalidDimensions, "Dimension multiplication must not overflow");
    for (const float value : {-0.1F,1.1F,std::numeric_limits<float>::infinity(),
                             std::numeric_limits<float>::quiet_NaN()}) {
        auto bad = valid; bad.pixels[0].r = value;
        error(adjust(bad.view()), ErrorCode::InvalidPixel, "Non-normalized/NaN RGB is rejected");
        bad = valid; bad.pixels[0].a = value;
        error(gaussianBlur(bad.view(),1), ErrorCode::InvalidPixel, "Invalid alpha is rejected");
    }
    auto options = ColorAdjustment{}; options.gamma = 0;
    error(adjust(valid.view(),options), ErrorCode::InvalidParameter, "Nonpositive gamma is rejected");
    options = {}; options.exposure = std::numeric_limits<double>::quiet_NaN();
    error(adjust(valid.view(),options), ErrorCode::InvalidParameter, "NaN settings are rejected");
    error(gaussianBlur(valid.view(),-1), ErrorCode::InvalidParameter, "Negative sigma is rejected");
    error(gaussianBlur(valid.view(),33), ErrorCode::InvalidParameter, "Unbounded blur work is rejected");
    error(sharpen(valid.view(),{1,-1,0}), ErrorCode::InvalidParameter, "Negative sharpen gain is rejected");
    error(addNoise(valid.view(),{1.1}), ErrorCode::InvalidParameter, "Noise amount range is enforced");
    auto noise = NoiseOptions{}; noise.distribution = static_cast<NoiseDistribution>(99);
    error(addNoise(valid.view(),noise), ErrorCode::InvalidParameter, "Unknown noise modes are rejected");
    auto blendOptions = BlendOptions{}; blendOptions.mode = static_cast<BlendMode>(99);
    error(blend(valid.view(),valid.view(),blendOptions), ErrorCode::InvalidParameter,
          "Unknown blend modes are rejected");
    blendOptions = {}; blendOptions.space = static_cast<BlendSpace>(99);
    error(blend(valid.view(),valid.view(),blendOptions), ErrorCode::InvalidParameter,
          "Unknown blend spaces are rejected");
    error(blend(valid.view(),valid.view(),{BlendMode::Normal,-.1}), ErrorCode::InvalidParameter,
          "Invalid composition opacity is rejected");
    Image other{2,1,{valid.pixels[0],valid.pixels[0]}};
    error(blend(valid.view(),other.view()), ErrorCode::SizeMismatch, "Mismatched extents are rejected");
    error(fromRgba8(1,1,std::array<std::uint8_t,3>{}), ErrorCode::InvalidBuffer,
          "Incomplete RGBA byte groups are rejected");
    error(fromArgb32(2,1,std::array<std::uint32_t,1>{}), ErrorCode::InvalidBuffer,
          "Incomplete ARGB buffers are rejected");
}
void conversion() {
    std::vector<std::uint32_t> input{0x12345678U,0x00abcdefU,0xff000000U,0xffffffffU};
    for (std::uint32_t i=0; i<256; ++i) input.push_back((i<<24)|((255-i)<<16)|(i<<8)|i);
    const auto image = fromArgb32(input.size(),1,input);
    check(image.has_value(), "Packed numeric ARGB is decoded");
    if (!image) return;
    const auto packed = toArgb32(image->view());
    check(packed && *packed == input, "ARGB conversion round-trips every 8-bit channel value");
    const auto bytes = toRgba8(image->view());
    check(bytes && (*bytes)[0] == 0x34 && (*bytes)[1] == 0x56 && (*bytes)[2] == 0x78 &&
          (*bytes)[3] == 0x12, "RGBA byte order is independent of numeric ARGB order");
    if (bytes) {
        const auto decoded = fromRgba8(input.size(),1,*bytes);
        check(decoded && decoded->pixels == image->pixels, "RGBA8 and ARGB32 encode identical pixels");
    }
    const auto half = toRgba8(single({.5F,0,1,.5F}).view());
    check(half && *half == std::vector<std::uint8_t>({128,0,255,128}), "8-bit encoding rounds to nearest");
}
void adjustment() {
    const auto input = single({.2F,.6F,.9F,.4F});
    const auto identity = adjust(input.view());
    check(identity && identity->pixels == input.pixels, "Default adjustment is an exact detached copy");
    if (identity) check(identity->pixels.data() != input.pixels.data(), "The output never aliases caller storage");
    ColorAdjustment setting; setting.exposure=1;
    const auto exposed = adjust(single({.5F,.5F,.5F,.4F}).view(),setting);
    check(exposed && nearPixel(exposed->pixels[0],{.6858361F,.6858361F,.6858361F,.4F}),
          "One stop doubles linear-light energy, preserving alpha");
    setting = {}; setting.saturation=-1;
    const auto gray = adjust(input.view(),setting);
    check(gray && nearPixel(gray->pixels[0],{.53662F,.53662F,.53662F,.4F}),
          "Full desaturation uses the documented encoded-RGB luma weights");
    setting = {}; setting.contrast=-1;
    const auto flat = adjust(input.view(),setting);
    check(flat && nearPixel(flat->pixels[0],{.5F,.5F,.5F,.4F}), "Minimum contrast collapses to midpoint");
    setting = {}; setting.gamma=2; setting.opacity=.5;
    const auto gamma = adjust(single({.25F,.25F,.25F,.4F}).view(),setting);
    check(gamma && nearPixel(gamma->pixels[0],{.5F,.5F,.5F,.2F}), "Gamma and alpha opacity have separate semantics");
    setting = {}; setting.temperature=1; setting.tint=1;
    const auto warm = adjust(single({.5F,.5F,.5F,1}).view(),setting);
    check(warm && warm->pixels[0].r > .5F && warm->pixels[0].g > .5F && warm->pixels[0].b < .5F,
          "Temperature/tint use the stated channel directions");
    setting = {}; setting.brightness=1;
    const auto clipped = adjust(input.view(),setting);
    check(clipped && nearPixel(clipped->pixels[0],{1,1,1,.4F}), "Adjustment clips to the normalized gamut");
    check(input.pixels[0] == Rgba{.2F,.6F,.9F,.4F}, "Input data is immutable");
}
void blending() {
    const auto back = single({.25F,.25F,.25F,1});
    const auto source = single({.8F,.8F,.8F,1});
    const std::array cases{
        std::pair{BlendMode::Normal,.8}, std::pair{BlendMode::Multiply,.2},
        std::pair{BlendMode::Screen,.85}, std::pair{BlendMode::Overlay,.4},
        std::pair{BlendMode::Darken,.25}, std::pair{BlendMode::Lighten,.8},
        std::pair{BlendMode::SoftLight,.4}, std::pair{BlendMode::Difference,.55}};
    for (const auto& [mode,value] : cases) {
        const auto result = blend(back.view(),source.view(),{mode});
        check(result && near(result->pixels[0].r,value), "Opaque blend matches its reference vector");
    }
    const auto low = blend(single({.1F,.1F,.1F,1}).view(),source.view(),{BlendMode::SoftLight});
    check(low && near(low->pixels[0].r,.2176), "Soft light evaluates its low-backdrop cubic branch");
    const auto dark = blend(back.view(),single({.2F,.2F,.2F,1}).view(),{BlendMode::SoftLight});
    check(dark && near(dark->pixels[0].r,.1375), "Soft light evaluates its dark-source branch");
    const auto high = blend(single({.64F,.64F,.64F,1}).view(),single({.75F,.75F,.75F,1}).view(),
                            {BlendMode::SoftLight});
    check(high && near(high->pixels[0].r,.72), "Soft light evaluates its square-root branch");
    const auto red = single({1,0,0,.5F}), blue = single({0,0,1,.5F});
    const auto over = blend(red.view(),blue.view());
    check(over && nearPixel(over->pixels[0],{1.F/3,0,2.F/3,.75F}),
          "Source-over accounts for both partial alphas before unpremultiplying");
    const auto multiply = blend(red.view(),blue.view(),{BlendMode::Multiply});
    check(multiply && nearPixel(multiply->pixels[0],{1.F/3,0,1.F/3,.75F}),
          "Blend mode affects only the overlapping portion");
    const auto clear = blend(single({1,0,0,0}).view(),blue.view(),{BlendMode::Multiply});
    check(clear && nearPixel(clear->pixels[0],blue.pixels[0]), "Hidden backdrop RGB cannot contaminate the source");
    const auto linear = blend(single({0,0,0,1}).view(),single({1,1,1,.5F}).view(),
                              {BlendMode::Normal,1,BlendSpace::LinearSrgb});
    check(linear && near(linear->pixels[0].r,.73535698), "Linear-light composition encodes the final color once");
    const auto zero = blend(back.view(),source.view(),{BlendMode::Normal,0});
    check(zero && zero->pixels == back.pixels, "Zero opacity is an exact backdrop copy");
}
void blur() {
    const Image edge{2,1,{{1,0,0,0},{1,1,1,1}}};
    const auto softened = gaussianBlur(edge.view(),1);
    check(softened && softened->pixels[0].a > 0 && softened->pixels[1].a < 1,
          "Blur spreads alpha across the boundary");
    if (softened) for (auto p : softened->pixels)
        check(near(p.r,1) && near(p.g,1) && near(p.b,1), "Premultiplied blur eliminates hidden-color fringes");
    const Image impulse{7,1,{{0,0,0,1},{0,0,0,1},{0,0,0,1},{1,1,1,1},
                           {0,0,0,1},{0,0,0,1},{0,0,0,1}}};
    const auto result = gaussianBlur(impulse.view(),1);
    if (result) {
        const double center = 1/(1+2*(std::exp(-.5)+std::exp(-2.)+std::exp(-4.5)));
        const double encoded = 1.055*std::pow(center,1/2.4)-.055;
        check(near(result->pixels[3].r,encoded), "Gaussian impulse response has the normalized analytic center");
        check(near(result->pixels[2].r,result->pixels[4].r) && result->pixels[3].r > result->pixels[2].r,
              "Gaussian kernel is symmetric and localized");
    } else check(false,"Gaussian impulse image is accepted");
    const auto constant = single({.2F,.6F,.9F,.4F});
    const auto blurred = gaussianBlur(constant.view(),32);
    check(blurred && nearPixel(blurred->pixels[0],constant.pixels[0]), "Clamped boundaries preserve a constant 1x1 image");
    const auto zero = gaussianBlur(edge.view(),0);
    check(zero && zero->pixels == edge.pixels, "Zero blur preserves even invisible RGB exactly");
}
void sharpening() {
    const Image edge{3,1,{{.2F,.2F,.2F,.4F},{.8F,.8F,.8F,.7F},{.8F,.8F,.8F,1}}};
    const auto result = sharpen(edge.view());
    check(result && result->pixels[0].r < .2F && result->pixels[1].r > .8F,
          "Unsharp masking increases local edge contrast");
    if (result) for (std::size_t i=0;i<edge.pixels.size();++i)
        check(result->pixels[i].a == edge.pixels[i].a, "Sharpening preserves each original alpha");
    const auto skipped = sharpen(edge.view(),{1,1,1});
    check(skipped && skipped->pixels == edge.pixels, "A high threshold suppresses low-contrast sharpening");
    const auto zero = sharpen(edge.view(),{1,0,0});
    check(zero && zero->pixels == edge.pixels, "Zero sharpen amount is exactly neutral");
}
void noise() {
    Image base{128,64,std::vector<Rgba>(8192,{.5F,.5F,.5F,.4F})};
    const NoiseOptions uniform{.1,42,NoiseDistribution::Uniform,true};
    const auto a = addNoise(base.view(),uniform), b = addNoise(base.view(),uniform);
    check(a && b && a->pixels == b->pixels, "Fixed seeds reproduce the same detached result");
    if (!a) return;
    double sum=0, variance=0;
    for (auto p : a->pixels) {
        check(p.r >= .4F && p.r <= .6F && p.r == p.g && p.g == p.b && p.a == .4F,
              "Uniform monochrome noise respects amplitude and alpha");
        const double delta=p.r-.5; sum+=delta; variance+=delta*delta;
    }
    check(std::abs(sum/8192)<.003 && std::abs(variance/8192-.01/3)<.0003,
          "Uniform samples have the expected mean and variance");
    auto changed=uniform; changed.seed=43;
    const auto c=addNoise(base.view(),changed);
    check(c && c->pixels != a->pixels, "Changing the seed changes the actual output");
    changed=uniform; changed.monochromatic=false;
    const auto rgb=addNoise(base.view(),changed);
    check(rgb && rgb->pixels[0].r != rgb->pixels[0].g, "Color noise samples channels independently");
    check(rgb && rgb->pixels[0].g != rgb->pixels[1].r,
          "Adjacent pixels must not reuse shifted channel sequences");
    changed=uniform; changed.distribution=NoiseDistribution::Gaussian;
    const auto gaussian=addNoise(base.view(),changed);
    if (gaussian) {
        double mean=0,squares=0; bool outsideUniform=false;
        for (auto p : gaussian->pixels) {
            const double d=p.r-.5; mean+=d; squares+=d*d;
            outsideUniform |= std::abs(d)>.1;
        }
        check(std::abs(mean/8192)<.005 && std::abs(squares/8192-.01)<.001 && outsideUniform,
              "Gaussian noise has the requested standard deviation and nonuniform tails");
    } else check(false,"Gaussian noise image is accepted");
    const auto zero=addNoise(base.view());
    check(zero && zero->pixels == base.pixels, "Zero noise is exactly neutral");
}
void invariants() {
    Image input{13,11,{}};
    for (std::size_t i=0;i<input.width*input.height;++i)
        input.pixels.push_back({float(i%11)/10,float(i%7)/6,float(i%5)/4,float(i%3)/2});
    const auto original=input.pixels;
    const std::array<Result<Image>,5> results{
        adjust(input.view(),{3,.2,1,1,.1,1,-1,.3}), gaussianBlur(input.view(),2.5),
        sharpen(input.view(),{2.5,8,0}), addNoise(input.view(),{1,25,NoiseDistribution::Gaussian,false}),
        blend(input.view(),input.view(),{BlendMode::SoftLight,.65,BlendSpace::LinearSrgb})};
    for (const auto& result : results) {
        check(result && result->width==13 && result->height==11 && result->pixels.size()==143,
              "Operations preserve dimensions and valid storage");
        if (result) for (auto p : result->pixels)
            for (float channel : {p.r,p.g,p.b,p.a})
                check(std::isfinite(channel) && channel>=0 && channel<=1,
                      "Extreme valid settings produce finite normalized channels");
    }
    check(input.pixels==original,"All public operations leave the original buffer unchanged");
}
} // namespace
int main(int argc,char** argv) {
    if (argc!=2) return 2;
    const std::string_view name=argv[1];
    if (name=="validation") validation(); else if(name=="conversion") conversion();
    else if(name=="adjustment") adjustment(); else if(name=="blend") blending();
    else if(name=="blur") blur(); else if(name=="sharpen") sharpening();
    else if(name=="noise") noise(); else if(name=="invariants") invariants(); else return 2;
    return failures ? 1 : 0;
}
