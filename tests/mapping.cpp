#include <iiGraphicAdjust/Mapping.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

using namespace iiGraphicAdjust;
namespace {
int failures=0;
void check(bool ok,std::string_view message) { if(!ok) { ++failures; std::cerr<<"FAIL: "<<message<<'\n'; } }
bool near(double a,double b) { return std::abs(a-b)<2e-6; }
Image single(Rgba p) { return {1,1,{p}}; }
void gradient() {
    const Image image{4,1,{{0,0,0,.2F},{.5F,.5F,.5F,.7F},{1,1,1,1},{.3F,.7F,.9F,0}}};
    const auto original=image.pixels;
    GradientMapOptions options;
    options.stops={{0,{1,0,0}},{.5,{0,1,0}},{1,{0,0,1}}};
    const auto result=gradientMap(image.view(),options);
    check(result && result->pixels[0]==Rgba{1,0,0,.2F} && result->pixels[1]==Rgba{0,1,0,.7F} &&
          result->pixels[2]==Rgba{0,0,1,1},"Gradient interpolates specified RGB stops and preserves alpha");
    check(result && result->pixels[3]==original[3] && image.pixels==original,"Hidden RGB and original storage are preserved");
    options.reverse=true;
    const auto reversed=gradientMap(image.view(),options);
    check(reversed && reversed->pixels[0].b==1 && reversed->pixels[2].r==1,"Reverse flips the luminance coordinate");
    options.reverse=false; options.amount=.5;
    const auto mixed=gradientMap(single({.25F,.25F,.25F,.4F}).view(),options);
    check(mixed && near(mixed->pixels[0].r,.375) && near(mixed->pixels[0].g,.375) && near(mixed->pixels[0].b,.125),
          "Gradient amount interpolates colors without changing alpha");
    options.amount=0;
    check(gradientMap(image.view(),options)->pixels==original,"Zero mapping amount is exact identity");
    options.stops[1].position=0;
    check(!gradientMap(image.view(),options),"Duplicate stop positions fail even with zero amount");
    options={}; options.stops[0].color[0]=std::numeric_limits<double>::quiet_NaN();
    check(!gradientMap(image.view(),options),"Nonfinite gradient colors are rejected");
    options={}; options.stops.clear(); check(!gradientMap(image.view(),options),"Empty gradients are rejected");
    options={}; options.dither=true;
    Image gray{4,4,std::vector<Rgba>(16,{.5F,.5F,.5F,.25F})};
    const auto dithered=gradientMap(gray.view(),options);
    double sum=0;
    if(dithered) for(auto p:dithered->pixels) { sum+=p.r-.5; check(p.a==.25F,"Dither preserves alpha"); }
    check(dithered && std::abs(sum)<1e-6 && dithered->pixels[0].r!=dithered->pixels[1].r,
          "Ordered 4x4 dither is spatially varied with zero mean");
}
void lookup() {
    auto identity=identityLut(3);
    check(identity.has_value(),"Identity LUT is generated"); if(!identity) return;
    const auto source=single({.23F,.67F,.91F,.4F});
    for(auto interpolation:{LutInterpolation::Trilinear,LutInterpolation::Tetrahedral}) {
        const auto result=colorLookup(source.view(),*identity,{interpolation});
        check(result && near(result->pixels[0].r,.23) && near(result->pixels[0].g,.67) && near(result->pixels[0].b,.91) &&
              result->pixels[0].a==.4F,"Identity lookup preserves arbitrary colors and alpha");
    }
    auto cross=identityLut(2);
    for(auto& row:cross->values) row={row[0]*row[1],row[0]*row[2],row[1]*row[2]};
    std::array<float,3> fractions{.2F,.4F,.8F};
    do {
        const auto image=single({fractions[0],fractions[1],fractions[2],1});
        const auto tri=colorLookup(image.view(),*cross,{LutInterpolation::Trilinear});
        const auto tetra=colorLookup(image.view(),*cross,{LutInterpolation::Tetrahedral});
        check(tri && near(tri->pixels[0].r,fractions[0]*fractions[1]) && near(tri->pixels[0].g,fractions[0]*fractions[2]) &&
              near(tri->pixels[0].b,fractions[1]*fractions[2]),"Trilinear lookup matches multilinear cross-products");
        check(tetra && near(tetra->pixels[0].r,std::min(fractions[0],fractions[1])) &&
              near(tetra->pixels[0].g,std::min(fractions[0],fractions[2])) &&
              near(tetra->pixels[0].b,std::min(fractions[1],fractions[2])),"All six tetrahedra interpolate analytic corner data");
    } while(std::next_permutation(fractions.begin(),fractions.end()));
    auto domain=identityLut(2); domain->domainMin={-1,-1,-1}; domain->domainMax={1,1,1};
    const auto shifted=colorLookup(single({0,0,0,1}).view(),*domain);
    check(shifted && near(shifted->pixels[0].r,.5),"LUT domains normalize input coordinates");
    const auto invisible=single({.2F,.6F,.7F,0});
    const auto hidden=colorLookup(invisible.view(),*cross);
    check(hidden && hidden->pixels==invisible.pixels,"Lookup preserves zero-alpha RGB");
    domain->values[0][0]=std::numeric_limits<float>::infinity();
    check(!colorLookup(source.view(),*domain),"Nonfinite LUT output data is rejected");
    domain=identityLut(2); domain->values.pop_back(); check(!colorLookup(source.view(),*domain),"Truncated LUT rows are rejected");
    check(!identityLut(0) && !identityLut(130),"LUT size limits are enforced");
    check(!colorLookup(source.view(),*identity,{static_cast<LutInterpolation>(99)}),"Invalid interpolation enum is rejected");
    check(!colorLookup({},*identity),"Invalid image dimensions are rejected");
}
void parser() {
    constexpr auto cube=R"(TITLE "Identity # literal" # comment
LUT_3D_SIZE 2
DOMAIN_MIN 0 0 0
DOMAIN_MAX 1 1 1
0 0 0
1 0 0
0 1 0
1 1 0
0 0 1
1 0 1
0 1 1
1 1 1
)";
    const auto parsed=parseCubeLut(cube);
    check(parsed && parsed->size==2 && parsed->kind==LutKind::ThreeDimensional,"Cube parser accepts comments, title and domain");
    if(parsed) {
        const auto result=colorLookup(single({.3F,.6F,.9F,1}).view(),*parsed);
        check(result && near(result->pixels[0].r,.3) && near(result->pixels[0].g,.6),"Parsed cube uses red-fastest numeric ordering");
    }
    const auto one=parseCubeLut("LUT_1D_SIZE 3\n0 0 0\n.25 .5 .75\n1 1 1\n");
    check(one && one->kind==LutKind::OneDimensional,"One-dimensional Cube LUTs are represented directly");
    if(one) {
        const auto result=colorLookup(single({.5F,.5F,.5F,.4F}).view(),*one);
        check(result && near(result->pixels[0].r,.25) && near(result->pixels[0].g,.5) && near(result->pixels[0].b,.75),
              "One-dimensional LUT channels interpolate independently");
    }
    for(auto bad:{"", "LUT_3D_SIZE 1\n", "LUT_3D_SIZE 999999999\n", "LUT_1D_SIZE 2\n0 0 0\n",
                 "LUT_1D_SIZE 2\n+-1 0 0\n1 1 1\n", "LUT_1D_SIZE 2\n++1 0 0\n1 1 1\n",
                 "LUT_1D_SIZE 2\n0 0 0\n1 1 1\n0 0 0\n", "LUT_1D_SIZE 2\n0 0 0\nNaN 1 1\n",
                 "LUT_1D_SIZE 2\n0 0 0 junk\n1 1 1\n", "LUT_1D_SIZE 2\nLUT_3D_SIZE 2\n",
                 "LUT_1D_SIZE 2\nDOMAIN_MIN 1 0 0\nDOMAIN_MAX 1 1 1\n0 0 0\n1 1 1\n",
                 "LUT_1D_SIZE 2\n0 0 0\nDOMAIN_MIN 0 0 0\n1 1 1\n", "TITLE unquoted\nLUT_1D_SIZE 2\n0 0 0\n1 1 1\n",
                 "LUT_1D_SIZE 2\nDOMAIN_MAX 1 1 1\nDOMAIN_MAX 1 1 1\n0 0 0\n1 1 1\n"})
        check(!parseCubeLut(bad),"Malformed, truncated, unsupported and duplicate Cube data is rejected");
}
} // namespace
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    const std::string_view name=argv[1];
    if(name=="gradient") gradient(); else if(name=="lookup") lookup(); else if(name=="parser") parser(); else return 2;
    return failures?1:0;
}
