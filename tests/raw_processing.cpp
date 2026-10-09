#include <iiGraphicAdjust/RawProcessing.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

using namespace iiGraphicAdjust;
namespace {
int failures=0;
void check(bool valid,const char* reason) {
    if (!valid) { std::cerr<<"FAIL: "<<reason<<'\n'; ++failures; }
}
bool near(double actual,double expected,double tolerance=2e-6) {
    return std::abs(actual-expected)<=tolerance;
}
bool rgb(const HdrRgba& p,double r,double g,double b) {
    return near(p.r,r)&&near(p.g,g)&&near(p.b,b)&&p.a==1;
}
constexpr std::array<BayerPattern,4> patterns{BayerPattern::RGGB,BayerPattern::BGGR,
    BayerPattern::GRBG,BayerPattern::GBRG};
// Independent test pattern lookup: channel indices are R=0,G=1,B=2.
int channel(BayerPattern pattern,std::size_t x,std::size_t y,unsigned px=0,unsigned py=0) {
    constexpr int tile[4][2][2]={{{0,1},{1,2}},{{2,1},{1,0}},{{1,0},{2,1}},{{1,2},{0,1}}};
    return tile[static_cast<int>(pattern)][(y+py)%2][(x+px)%2];
}
std::vector<std::uint16_t> constant(BayerPattern pattern,std::size_t w,std::size_t h,
    std::size_t stride,const std::array<std::uint16_t,3>& values,unsigned px=0,unsigned py=0) {
    std::vector<std::uint16_t> data((h-1)*stride+w,65535);
    for (std::size_t y=0;y<h;++y) for (std::size_t x=0;x<w;++x)
        data[y*stride+x]=values[std::size_t(channel(pattern,x,y,px,py))];
    return data;
}
void patternTests() {
    RawDevelopmentOptions options; options.calibration.whiteLevel=1000;
    for (const auto pattern : patterns) {
        const auto data=constant(pattern,6,5,6,{100,400,800});
        const auto before=data;
        const auto output=developRaw({6,5,6,data,pattern},options);
        check(output&&output->width==6&&output->height==5&&output->pixels.size()==30,
              "Every Bayer pattern returns a full detached RGB image");
        if(output) for(const auto& p:output->pixels)
            check(rgb(p,.1,.4,.8),"Constant distinct RGB reconstructs correctly at every interior/edge/corner");
        check(data==before,"RAW input remains unchanged");
        check(output&&output->view().pixels.data()==output->pixels.data(),"Linear RGB view uses owned storage");
    }
}
void interpolationTests() {
    constexpr std::size_t w=7,h=7;
    RawDevelopmentOptions options; options.calibration.whiteLevel=10000;
    for(const auto pattern:patterns) {
        std::vector<std::uint16_t> data(w*h);
        const auto value=[](int c,std::size_t x,std::size_t y) {
            return std::uint16_t((c+1)*1000+100*x+200*y);
        };
        for(std::size_t y=0;y<h;++y) for(std::size_t x=0;x<w;++x)
            data[y*w+x]=value(channel(pattern,x,y),x,y);
        const auto output=developRaw({w,h,w,data,pattern},options);
        check(bool(output),"Linear color ramps develop");
        if(!output) continue;
        for(std::size_t y=1;y+1<h;++y) for(std::size_t x=1;x+1<w;++x)
            check(rgb(output->pixels[y*w+x],double(value(0,x,y))/10000,
                      double(value(1,x,y))/10000,double(value(2,x,y))/10000),
                  "Bilinear interpolation exactly reproduces independent affine RGB ramps in the interior");
        for(std::size_t y=0;y<h;++y) for(std::size_t x=0;x<w;++x) {
            const auto p=output->pixels[y*w+x]; const std::array<float,3> actual{p.r,p.g,p.b};
            check(near(actual[std::size_t(channel(pattern,x,y))],double(data[y*w+x])/10000),
                  "Measured color samples are preserved including boundaries");
        }
    }
    // Unequal neighbors make all three interpolation stencils independently observable.
    const std::array<std::uint16_t,9> values{10,20,30,40,100,60,70,80,90};
    options.calibration.whiteLevel=100;
    const auto output=developRaw({3,3,3,values},options);
    check(output&&rgb(output->pixels[4],.5, .5,1),
          "Blue center uses four red diagonal and four green axial neighbors");
    check(output&&rgb(output->pixels[0],.1,.3,1),
          "Red corner averages its two available green neighbors and one blue diagonal");
    check(output&&rgb(output->pixels[1],.2,.2,1),
          "Green boundary interpolates red horizontally and blue vertically");
}
void calibrationTests() {
    RawDevelopmentOptions options;
    options.calibration.blackLevels={100,200,300,400}; options.calibration.whiteLevel=1100;
    const std::array<std::uint16_t,4> values{350,650,700,470};
    const auto output=developRaw({2,2,2,values},options);
    check(output&&rgb(output->pixels[0],.25,.5,.1)&&rgb(output->pixels[3],.25,.5,.1),
          "Each green tile uses its own black level and sample-minus-black over white-minus-black");
    const std::array<std::uint16_t,4> dark{99,200,299,400};
    const auto black=developRaw({2,2,2,dark},options);
    check(black&&std::all_of(black->pixels.begin(),black->pixels.end(),[](auto p){return rgb(p,0,0,0);}),
          "Samples at or below per-tile black clamp to zero");
    const std::array<std::uint16_t,4> whites{1100,1100,1100,1100};
    const auto white=developRaw({2,2,2,whites},options);
    check(white&&std::all_of(white->pixels.begin(),white->pixels.end(),[](auto p){return rgb(p,1,1,1);}),
          "Per-tile white-normalized samples equal one");
    options.calibration.blackLevels={0,0,0,0}; options.calibration.whiteLevel=1000;
    options.calibration.whiteBalanceGains={2,1,.5};
    options.calibration.cameraToLinearSrgb={1,.5,0, .25,1,.5, -.5,0,1};
    options.exposureStops=1;
    const auto data=constant(BayerPattern::RGGB,4,4,4,{100,400,800});
    const auto corrected=developRaw({4,4,4,data},options);
    // WB=(.2,.4,.4), matrix=(.4,.65,.3), exposure multiplies by two.
    check(corrected&&rgb(corrected->pixels[5],.8,1.3,.6),
          "Nonidentity white balance and row-major camera matrix follow an independent arithmetic oracle");
    options.calibration.cameraToLinearSrgb={-1,0,0, 0,1,0, 0,0,1};
    const auto negative=developRaw({4,4,4,data},options);
    check(negative&&rgb(negative->pixels[5],0,.8,.8),"Negative matrix output clamps to zero without clipping positive headroom");
}
void renderingTests() {
    RawDevelopmentOptions options; options.calibration.whiteLevel=1000; options.exposureStops=1;
    const std::array<std::uint16_t,4> data{2000,2000,2000,2000};
    const BayerImageView source{2,2,2,data};
    const auto linear=developRaw(source,options);
    check(linear&&rgb(linear->pixels[0],4,4,4),"Sensor values above white and positive exposure retain linear highlight headroom");
    const auto recovered=renderRaw(source,options,{HdrToningMethod::ExposureGamma,-3});
    check(recovered&&near(recovered->pixels[0].r,.735356983)&&recovered->pixels[0].a==1,
          "SDR rendering can recover headroom by applying negative toning exposure before clipping");
    const auto clipped=renderRaw(source,options);
    check(clipped&&clipped->pixels[0]==Rgba{1,1,1,1},"Neutral SDR rendering explicitly clips highlights");
    const auto compressed=renderRaw(source,options,{HdrToningMethod::HighlightCompression});
    check(compressed&&near(compressed->pixels[0].r,.906331753),"RAW convenience rendering uses existing highlight compression");
    const auto composed=linear?hdrToning(linear->view(),{HdrToningMethod::HighlightCompression}):Result<Image>{std::unexpected(Error{ErrorCode::InvalidPixel,"test"})};
    check(compressed&&composed&&compressed->pixels==composed->pixels,"renderRaw equals explicit development plus hdrToning composition");
    check(!renderRaw(source,options,{static_cast<HdrToningMethod>(99)}),"RAW rendering propagates invalid HDR settings");
}
void layoutTests() {
    RawDevelopmentOptions options; options.calibration.whiteLevel=1000;
    for(const auto pattern:patterns) for(unsigned py=0;py<2;++py) for(unsigned px=0;px<2;++px) {
        const auto data=constant(pattern,5,3,8,{100,400,800},px,py);
        const auto output=developRaw({5,3,8,data,pattern,std::uint8_t(px),std::uint8_t(py)},options);
        check(output&&std::all_of(output->pixels.begin(),output->pixels.end(),[](auto p){return rgb(p,.1,.4,.8);}),
              "Stride padding, minimum final-row length, odd dimensions, and all phase combinations preserve CFA colors");
        const auto smallData=constant(pattern,2,2,2,{100,400,800},px,py);
        const auto small=developRaw({2,2,2,smallData,pattern,std::uint8_t(px),std::uint8_t(py)},options);
        check(small&&std::all_of(small->pixels.begin(),small->pixels.end(),[](auto p){return rgb(p,.1,.4,.8);}),
              "Minimum 2x2 image reconstructs all colors at every phase and Bayer pattern");
    }
    options.calibration.blackLevels={100,200,300,400}; options.calibration.whiteLevel=1100;
    const std::array<std::uint16_t,4> shifted{470,700,650,350};
    const auto shiftedOutput=developRaw({2,2,2,shifted,BayerPattern::RGGB,1,1},options);
    check(shiftedOutput&&rgb(shiftedOutput->pixels[0],.25,.5,.1),"Phase offsets apply equally to CFA pattern and black-level tile");
    const std::array<std::uint16_t,6> extra{100,400,400,800,60000,60000};
    const auto trailing=developRaw({2,2,2,extra},RawDevelopmentOptions{RawCalibration{{0,0,0,0},1000}});
    check(trailing&&rgb(trailing->pixels[0],.1,.4,.8),"Extra trailing buffer samples are allowed and ignored");
}
void validationTests() {
    const std::array<std::uint16_t,4> data{100,400,400,800};
    const BayerImageView valid{2,2,2,data};
    auto badView=valid;
    badView.width=0; check(!developRaw(badView),"Zero width rejected");
    badView=valid; badView.height=0; check(!developRaw(badView),"Zero height rejected");
    badView=valid; badView.width=1; check(!developRaw(badView),"One-column mosaic rejected instead of inventing missing colors");
    badView=valid; badView.height=1; check(!developRaw(badView),"One-row mosaic rejected instead of inventing missing colors");
    badView=valid; badView.strideSamples=1; check(!developRaw(badView),"Stride smaller than width rejected");
    badView=valid; badView.samples=std::span{data}.first<3>(); check(!developRaw(badView),"Truncated sample buffer rejected");
    badView=valid; badView.pattern=static_cast<BayerPattern>(99); check(!developRaw(badView),"Unknown Bayer pattern rejected");
    badView=valid; badView.phaseX=2; check(!developRaw(badView),"Invalid X phase rejected");
    badView=valid; badView.phaseY=2; check(!developRaw(badView),"Invalid Y phase rejected");
    badView=valid; badView.width=std::numeric_limits<std::size_t>::max(); badView.strideSamples=badView.width;
    check(!developRaw(badView),"Pixel count overflow rejected");
    badView=valid; badView.strideSamples=std::numeric_limits<std::size_t>::max();
    check(!developRaw(badView),"Stride plus final row overflow rejected");
    badView=valid; badView.height=3; badView.strideSamples=std::numeric_limits<std::size_t>::max()/2+1;
    check(!developRaw(badView),"Stride times row count overflow rejected");
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    RawDevelopmentOptions options;
    for(const auto value:std::array<double,4>{0,-1,nan,inf}) {
        options={}; options.calibration.whiteLevel=value;
        check(!developRaw(valid,options),"White level must be finite positive");
    }
    options={}; options.calibration.whiteLevel=65536; check(!developRaw(valid,options),"White level cannot exceed unpacked uint16 sensor range");
    for(std::size_t i=0;i<4;++i) for(const auto value:std::array<double,4>{-1,65535,nan,inf}) {
        options={}; options.calibration.blackLevels[i]=value;
        check(!developRaw(valid,options),"Every tile black level must be finite nonnegative below white");
    }
    for(std::size_t i=0;i<3;++i) for(const auto value:std::array<double,4>{0,-1,nan,inf}) {
        options={}; options.calibration.whiteBalanceGains[i]=value;
        check(!developRaw(valid,options),"Every white balance gain must be finite positive");
    }
    for(std::size_t i=0;i<9;++i) for(const auto value:std::array<double,2>{nan,inf}) {
        options={}; options.calibration.cameraToLinearSrgb[i]=value;
        check(!developRaw(valid,options),"Every matrix entry must be finite");
    }
    for(const auto value:std::array<double,4>{-17,17,nan,inf}) {
        options={}; options.exposureStops=value; check(!developRaw(valid,options),"Exposure must be finite in [-16,16]");
    }
    options={}; options.calibration.whiteBalanceGains[0]=std::numeric_limits<double>::max();
    check(!developRaw(valid,options),"Finite calibration producing an unrepresentable output reports an error");
    options={}; options.calibration.cameraToLinearSrgb[0]=std::numeric_limits<double>::max();
    check(!developRaw(valid,options),"Finite matrix producing float overflow reports an error");
    options={}; options.calibration.cameraToLinearSrgb.fill(0);
    const auto zero=developRaw(valid,options);
    check(zero&&rgb(zero->pixels[0],0,0,0),"A finite zero matrix is allowed and deterministically produces black");
}
} // namespace
int main(int argc,char** argv) {
    if(argc!=2) { std::cerr<<"Pass patterns, interpolation, calibration, rendering, layout, or validation\n"; return 2; }
    const std::string_view requested{argv[1]};
    if(requested=="patterns") patternTests();
    else if(requested=="interpolation") interpolationTests();
    else if(requested=="calibration") calibrationTests();
    else if(requested=="rendering") renderingTests();
    else if(requested=="layout") layoutTests();
    else if(requested=="validation") validationTests();
    else { std::cerr<<"Unknown test case\n"; return 2; }
    return failures?1:0;
}
