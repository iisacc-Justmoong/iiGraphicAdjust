#include <iiGraphicAdjust/Objects.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <string_view>

using namespace iiGraphicAdjust;
namespace o = iiGraphicAdjust::objects;
namespace {
int failures = 0;
void check(bool ok, const char *message) {
    if (!ok) { std::cerr << message << '\n'; ++failures; }
}
Image fixture() {
    return {3, 2, {{.1F,.3F,.6F,1}, {.8F,.5F,.2F,.5F}, {.2F,.7F,.9F,0},
                   {.7F,.2F,.4F,1}, {.4F,.6F,.3F,1}, {.9F,.8F,.7F,.7F}}};
}
bool same(const Image &a, const Image &b) {
    return a.width == b.width && a.height == b.height && a.pixels == b.pixels;
}
void ordering() {
    auto source = fixture(); const auto before = source;
    o::Exposure exposure; exposure.options.stops = 1;
    o::Invert invert;
    OperationStack stack{{{Operation{exposure}}, {Operation{invert}}}};
    auto result = stack.evaluate(source.view());
    check(bool(result), "ordered stack succeeds");
    if (result) {
        const auto expected = 1 - float(1.055*std::pow(2*std::pow((.1+.055)/1.055,2.4),1/2.4)-.055);
        check(std::abs(result->pixels[0].r-expected) < 2e-7, "exposure precedes invert");
        check(result->pixels[1].a == .5F && result->pixels[2] == source.pixels[2],
              "point operations preserve alpha and hidden RGB");
        check(result->pixels.data() != source.pixels.data(), "stack output is detached");
    }
    std::reverse(stack.stages.begin(), stack.stages.end());
    auto reverse = stack.apply(source.view());
    check(reverse && result && reverse->pixels[0] != result->pixels[0], "order is observable");
    check(same(source,before), "source remains immutable");
    OperationStack empty;
    auto copy = empty.apply(source.view());
    check(copy && same(*copy,source) && copy->pixels.data()!=source.pixels.data(), "empty stack owns a copy");
}
void ownership() {
    auto source = fixture();
    o::MatchColor match; match.reference = source;
    Operation saved{match};
    const auto baseline = saved.apply(source.view());
    match.reference.pixels.assign(6,{1,0,0,1});
    auto current = saved.apply(source.view());
    check(baseline && current && same(*baseline,*current), "operation captures reference image by value");
    o::Curves curves;
    curves.options.master.points = {{0,.1},{1,.8}};
    Operation frozen{curves};
    curves.options.master.points.clear();
    check(bool(frozen.apply(source.view())), "curve point arrays are owned snapshots");
    Operation deep = [] {
        o::LensBlur lens;
        lens.options.radius = 1;
        lens.options.depth = LensDepthSource::Map;
        lens.options.depthMap = Image{3,2,std::vector<Rgba>(6,{.5F,.5F,.5F,1})};
        return Operation{std::move(lens)};
    }();
    auto copied = deep;
    deep = Operation{o::Invert{}};
    check(bool(copied.apply(source.view())), "copied depth-map operation survives producer destruction");
    o::ColorLookup lookup;
    auto identity = identityLut(2);
    check(bool(identity), "identity LUT is available");
    if (identity) {
        lookup.lut = std::move(*identity);
        Operation lut{lookup}; lookup.lut.values.clear();
        auto result = lut.apply(source.view());
        check(result && std::abs(result->pixels[0].g-.3F)<1e-6, "LUT storage is owned");
    }
}
void masking() {
    Image source{2,1,{{0,0,0,1},{.2F,.4F,.6F,1}}};
    OperationStage stage{Operation{o::Invert{}}};
    stage.mask = Mask{2,1,{.5F,0}};
    OperationStack stack{{stage}};
    stage.mask->coverage[0]=0;
    auto result=stack.apply(source.view());
    check(result && std::abs(result->pixels[0].r-.735356983F)<2e-7,
          "stack owns mask and interpolates in linear light");
    check(result && result->pixels[1]==source.pixels[1], "zero mask preserves exact source");
    stack.stages[0].space=CorrectionSpace::Srgb;
    auto encoded=stack.apply(source.view());
    check(encoded && encoded->pixels[0].r==.5F, "encoded interpolation is explicit");
    stack.stages[0].enabled=false;
    auto disabled=stack.apply(source.view());
    check(disabled && same(*disabled,source), "disabled stage leaves source unchanged");
    stack.stages[0].amount=std::numeric_limits<double>::quiet_NaN();
    check(!stack.apply(source.view()), "disabled stage still rejects corrupt mix settings");
    Image transparent{1,1,{{1,0,0,0}}};
    o::SolidFill fill; fill.options={{0,1,0,1},false};
    OperationStack alpha{{{Operation{fill},true,.5}}};
    auto composed=alpha.apply(transparent.view());
    check(composed && composed->pixels[0]==Rgba{0,1,0,.5F},
          "partial alpha uses premultiplication without leaking invisible red");
}
void failuresAndGeometry() {
    auto source=fixture();
    o::Crop crop; crop.rectangle={0,0,2,1};
    OperationStack stack{{{Operation{crop}},{Operation{o::Invert{}}}}};
    auto resized=stack.evaluate(source.view());
    check(resized && resized->width==2 && resized->height==1, "full geometry stage changes dimensions");
    stack.stages[0].amount=.5;
    auto fail=stack.evaluate(source.view());
    check(!fail && fail.error().stage==0 && fail.error().operation.id==o::Crop::info.id &&
          fail.error().cause.code==ErrorCode::SizeMismatch, "partial geometry fails at exact stage");
    o::Exposure invalid; invalid.options.stops=99;
    stack={{{Operation{o::Invert{}}},{Operation{invalid}}}};
    auto bad=stack.evaluate(source.view());
    check(!bad && bad.error().stage==1 && bad.error().operation.id==o::Exposure::info.id,
          "kernel failure retains stage and identity");
    stack.stages[1].enabled=false;
    check(bool(stack.apply(source.view())), "disabled kernel is not invoked");
    check(!stack.apply({}), "empty input is invalid");
    stack.stages.assign(OperationStack::maxStages+1, {Operation{o::Invert{}}});
    check(!stack.apply(source.view()), "stack stage budget is bounded");
    Operation moved{o::Invert{}}; Operation target{std::move(moved)};
    check(!moved.apply(source.view()) && bool(target.apply(source.view())), "moved-from handle is safely rejected");
}
void catalog() {
    std::set<std::string_view> ids;
    for (const auto &entry : operationCatalog()) {
        check(!entry.id.empty() && !entry.name.empty() && !entry.family.empty(), "catalog metadata is complete");
        check(ids.insert(entry.id).second, "catalog identities are unique");
    }
    check(ids.size()==309, "catalog covers the complete existing processing surface");
    check(ids.contains(o::BrightnessContrast::info.id) && ids.contains(o::CameraRawFilter::info.id) &&
          ids.contains(o::Develop::info.id) && ids.contains(o::Watercolor::info.id), "both product families are represented");
}
}
int main(int argc,char **argv) {
    const std::string_view which=argc>1?argv[1]:"all";
    if(which=="all"||which=="ordering") ordering();
    if(which=="all"||which=="ownership") ownership();
    if(which=="all"||which=="masking") masking();
    if(which=="all"||which=="failures") failuresAndGeometry();
    if(which=="all"||which=="catalog") catalog();
    return failures?1:0;
}
