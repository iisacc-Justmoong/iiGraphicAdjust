#include <iiGraphicAdjust/Objects.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string_view>
#include <type_traits>
using namespace iiGraphicAdjust;
namespace o=iiGraphicAdjust::objects;
namespace {
int failures=0, calls=0;
std::string_view selected="all";
Image source() {
    Image image{8,8,std::vector<Rgba>(64)};
    for(std::size_t i=0;i<64;++i) image.pixels[i]={float(i%8)/8,float(i/8)/8,float((i*7)%13)/13,1};
    image.pixels[0].a=0; return image;
}
Mask mask() { return {8,8,std::vector<float>(64,.5F)}; }
MeshQuad quad() { return {MeshPoint{0,0},MeshPoint{8,0},MeshPoint{8,8},MeshPoint{0,8}}; }
void setup(o::MatchColor &v) { v.reference=source(); }
void setup(o::Displace &v) { v.map=source(); }
void setup(o::CrossGuidedBlur &v) { v.guide=source(); }
void setup(o::Blend &v) { v.source=source(); }
void setup(o::ColorLookup &v) { v.lut=*identityLut(2); }
void setup(o::Crop &v) { v.rectangle={0,0,4,4}; }
void setup(o::FlatFieldCorrection &v) { v.reference={8,8,std::vector<Rgba>(64,{.5F,.5F,.5F,1})}; }
void setup(o::PatternFill &v) { v.pattern=source(); }
void setup(o::ApplyMaskedCorrection &v) { v.corrected=source();v.mask=mask(); }
void setup(o::MaskColorOverlay &v) { v.mask=mask(); }
void setup(o::MaskColorOnBwOverlay &v) { v.mask=mask(); }
void setup(o::MaskImageOnBlack &v) { v.mask=mask(); }
void setup(o::MaskImageOnWhite &v) { v.mask=mask(); }
void setup(o::MaskWhiteOnBlack &v) { v.mask=mask(); }
void setup(o::VanishingPoint &v) { v.options.planes.push_back({quad()}); }
void setup(o::VanishingPointPlanes &v) { setup(v.operation); }
void setup(o::PerspectiveWarp &v) { v.options.planes.push_back({quad(),quad()}); }
void setup(o::PerspectiveWarpPoint &v) { v.options.planes.push_back({quad(),quad()}); }
void setup(o::PuppetWarp &v) { v.options.pins={{{1,1},{1,1}},{ {6,6},{6,6}}}; }
void setup(o::PuppetWarpGeometry &v) { v.options.pins={{{1,1},{1,1}},{ {6,6},{6,6}}}; }
void setup(o::ColorRangeMask &v) { v.options.samples={{.2F,.4F,.6F,1}}; }
void setup(o::RefineMask &v) { v.mask=mask(); }
void setup(o::CombineMasks &v) { v.operand=mask(); }
void setup(o::CubeLut &v) { v.text="LUT_1D_SIZE 2\n0 0 0\n1 1 1\n"; }
template<class T> void setup(T &) {}
void failure(OperationInfo info,Error error) { std::cerr<<info.id<<": "<<error.message<<'\n';++failures; }
template<class T> void execute() {
    if(selected!="all" && selected!=T::info.family) return;
    ++calls;
    T object; setup(object); const auto image=source();
    if constexpr(ImageOperationObject<T>) {
        auto result=object.apply(image.view());
        if(!result) { failure(T::info,result.error());return; }
        if(!result->width || !result->height || result->pixels.size()!=result->width*result->height ||
           result->pixels.data()==image.pixels.data()) {failure(T::info,{ErrorCode::InvalidBuffer,"detached geometry"});return;}
        for(auto p:result->pixels) for(auto x:{p.r,p.g,p.b,p.a})
            if(!std::isfinite(x)||x<0||x>1) {failure(T::info,{ErrorCode::InvalidPixel,"normalized output"});return;}
    } else if constexpr(requires { object.generate(image.view()); }) {
        auto result=object.generate(image.view());
        if(!result) { failure(T::info,result.error());return; }
        for(auto v:result->coverage) if(!std::isfinite(v)||v<0||v>1) {failure(T::info,{ErrorCode::InvalidPixel,"mask range"});return;}
    } else if constexpr(requires { object.analyze(image.view()); }) {
        auto result=object.analyze(image.view());if(!result) failure(T::info,result.error());
    } else if constexpr(requires { object.apply(mask().view()); }) {
        const auto m=mask();auto result=object.apply(m.view());if(!result) failure(T::info,result.error());
    } else if constexpr(requires { object.build(); }) {
        auto result=object.build();if(!result) failure(T::info,result.error());
    } else if constexpr(requires { object.resolve(); }) {
        auto result=object.resolve();if(!result) failure(T::info,result.error());
    } else if constexpr(requires { object.sample(); }) {
        auto result=object.sample();if(!result) failure(T::info,result.error());
    } else if constexpr(requires { object.map(MeshPoint{}); }) {
        auto result=object.map({4,4});if(!result) failure(T::info,result.error());
    } else if constexpr(std::same_as<T,o::RawDevelopment> || std::same_as<T,o::RawRender>) {
        const std::array<std::uint16_t,4> data{16000,32000,32000,48000};
        auto result=object.apply({2,2,2,data});if(!result) failure(T::info,result.error());
    } else if constexpr(requires { typename T::sample_type; }) {
        using S=typename T::sample_type;
        constexpr S middle=[] { if constexpr(std::same_as<S,float>) return .5F;
            else if constexpr(std::same_as<S,std::uint8_t>) return S{128}; else return S{16384}; }();
        const std::array<S,3> samples{S{0},middle,static_cast<S>(std::same_as<S,float>?1:
                                      std::same_as<S,std::uint8_t>?255:32768)};
        const SamplePlaneView<S> plane{samples,3,1,3};
        std::array<S,3> output{};
        const MutableSamplePlaneView<S> destination{output,3,1,3};
        if constexpr(requires { object.analyze(plane); }) {
            auto r=object.analyze(plane);
            if(!r || r->value!=middle || r->selectedSamples!=3)
                failure(T::info,{ErrorCode::InvalidParameter,"sample average oracle"});
        } else if constexpr(requires { object.apply(plane,destination); }) {
            auto r=object.apply(plane,destination);
            if(!r || output!=std::array<S,3>{middle,middle,middle})
                failure(T::info,{ErrorCode::InvalidParameter,"sample fill oracle"});
        } else {
            auto row=samples;auto r=object.apply({row,3,1,3});
            constexpr auto expectedMiddle=std::same_as<S,std::uint8_t>?127:16384;
            if(!r || row!=std::array<S,3>{0,static_cast<S>(expectedMiddle),0})
                failure(T::info,{ErrorCode::InvalidParameter,"sample Solarize oracle"});
        }
    } else static_assert(sizeof(T)==0,"Every catalog object must have an executable contract");
}
}
int main(int argc,char **argv) {
    if(argc>1) selected=argv[1];
    execute<o::Adjust>();
    execute<o::AutoWhiteBalance>();
    execute<o::SampledWhiteBalance>();
    execute<o::PresetWhiteBalance>();
    execute<o::AutoDevelopLight>();
    execute<o::AutoBlackAndWhiteMix>();
    execute<o::Histogram>();
    execute<o::ChromaticAberrationEstimate>();
    execute<o::RankSpotSources>();
    execute<o::PupilDetector>();
    execute<o::DustDetector>();
    execute<o::AutoLensScale>();
    execute<o::AdaptiveWideAngleGeometry>();
    execute<o::PuppetWarpGeometry>();
    execute<o::PerspectiveWarpPoint>();
    execute<o::Blend>();
    execute<o::FilterGaussianBlur>();
    execute<o::BoxBlur>();
    execute<o::Blur>();
    execute<o::BlurMore>();
    execute<o::MotionBlur>();
    execute<o::RadialBlur>();
    execute<o::SurfaceBlur>();
    execute<o::UnsharpMask>();
    execute<o::FilterSharpen>();
    execute<o::SharpenMore>();
    execute<o::SharpenEdges>();
    execute<o::SmartSharpen>();
    execute<o::HighPass>();
    execute<o::Despeckle>();
    execute<o::SmartBlur>();
    execute<o::ShapeBlur>();
    execute<o::TiltShift>();
    execute<o::IrisBlur>();
    execute<o::FieldBlur>();
    execute<o::SpinBlur>();
    execute<o::PathBlur>();
    execute<o::CameraRawFilter>();
    execute<o::HueSaturation>();
    execute<o::Vibrance>();
    execute<o::ColorBalance>();
    execute<o::BlackAndWhite>();
    execute<o::PhotoFilter>();
    execute<o::ChannelMixer>();
    execute<o::SelectiveColor>();
    execute<o::Desaturate>();
    execute<o::ReplaceColor>();
    execute<o::MatchColor>();
    execute<o::ColorToAlpha>();
    execute<o::FlatFieldCorrection>();
    execute<o::SolidFill>();
    execute<o::GradientFill>();
    execute<o::PatternFill>();
    execute<o::DevelopColor>();
    execute<o::Calibrate>();
    execute<o::DevelopEffects>();
    execute<o::PostCropVignette>();
    execute<o::FilmGrain>();
    execute<o::DevelopDetail>();
    execute<o::Texture>();
    execute<o::Clarity>();
    execute<o::Dehaze>();
    execute<o::ColorMixer>();
    execute<o::PointColor>();
    execute<o::ColorGrading>();
    execute<o::DevelopBlackAndWhite>();
    execute<o::DevelopVibrance>();
    execute<o::Sharpening>();
    execute<o::LuminanceNoiseReduction>();
    execute<o::ColorNoiseReduction>();
    execute<o::Defringe>();
    execute<o::Develop>();
    execute<o::WhiteBalance>();
    execute<o::DevelopLight>();
    execute<o::DevelopCurve>();
    execute<o::PointColorRangePreview>();
    execute<o::MaskColorOverlay>();
    execute<o::MaskColorOnBwOverlay>();
    execute<o::MaskImageOnBlack>();
    execute<o::MaskImageOnWhite>();
    execute<o::MaskWhiteOnBlack>();
    execute<o::Offset>();
    execute<o::Twirl>();
    execute<o::Pinch>();
    execute<o::Spherize>();
    execute<o::Wave>();
    execute<o::Ripple>();
    execute<o::PolarCoordinates>();
    execute<o::Displace>();
    execute<o::Shear>();
    execute<o::ZigZag>();
    execute<o::GaussianBlur>();
    execute<o::Sharpen>();
    execute<o::AddNoise>();
    execute<o::ColoredPencil>();
    execute<o::Cutout>();
    execute<o::DryBrush>();
    execute<o::GalleryFilmGrain>();
    execute<o::Fresco>();
    execute<o::NeonGlow>();
    execute<o::PaintDaubs>();
    execute<o::PaletteKnife>();
    execute<o::PlasticWrap>();
    execute<o::PosterEdges>();
    execute<o::RoughPastels>();
    execute<o::SmudgeStick>();
    execute<o::Sponge>();
    execute<o::Underpainting>();
    execute<o::Watercolor>();
    execute<o::AccentedEdges>();
    execute<o::AngledStrokes>();
    execute<o::Crosshatch>();
    execute<o::DarkStrokes>();
    execute<o::InkOutlines>();
    execute<o::Spatter>();
    execute<o::SprayedStrokes>();
    execute<o::SumiE>();
    execute<o::DiffuseGlow>();
    execute<o::Glass>();
    execute<o::OceanRipple>();
    execute<o::BasRelief>();
    execute<o::ChalkCharcoal>();
    execute<o::Charcoal>();
    execute<o::Chrome>();
    execute<o::ConteCrayon>();
    execute<o::GraphicPen>();
    execute<o::HalftonePattern>();
    execute<o::NotePaper>();
    execute<o::Photocopy>();
    execute<o::Plaster>();
    execute<o::Reticulation>();
    execute<o::Stamp>();
    execute<o::TornEdges>();
    execute<o::WaterPaper>();
    execute<o::GlowingEdges>();
    execute<o::Craquelure>();
    execute<o::Grain>();
    execute<o::MosaicTiles>();
    execute<o::Patchwork>();
    execute<o::StainedGlass>();
    execute<o::Texturizer>();
    execute<o::FilterGallery>();
    execute<o::HdrToning>();
    execute<o::LensCorrection>();
    execute<o::ApplyLightcraftLook>();
    execute<o::ApplyLightcraftPreset>();
    execute<o::LightcraftLookColor>();
    execute<o::LightcraftLookNeutral>();
    execute<o::LightcraftLookVivid>();
    execute<o::LightcraftLookLandscape>();
    execute<o::LightcraftLookPortrait>();
    execute<o::LightcraftLookMono>();
    execute<o::LightcraftLookFilmWarmPrint>();
    execute<o::LightcraftLookFilmCoolFade>();
    execute<o::LightcraftLookFilmGoldenHour>();
    execute<o::LightcraftLookFilmFadedSlide>();
    execute<o::LightcraftLookCineTealAmber>();
    execute<o::LightcraftLookCineNightBlue>();
    execute<o::LightcraftLookCineDesertHeat>();
    execute<o::LightcraftLookCineNeonDusk>();
    execute<o::LightcraftLookMutedMatteSoft>();
    execute<o::LightcraftLookMutedBleached>();
    execute<o::LightcraftLookMutedPastelHaze>();
    execute<o::LightcraftLookMutedQuietGreen>();
    execute<o::LightcraftLookBwMonoRich>();
    execute<o::LightcraftLookBwRedFilter>();
    execute<o::LightcraftLookBwSoft>();
    execute<o::LightcraftLookBwSepia>();
    execute<o::LightcraftPresetWarmGlow>();
    execute<o::LightcraftPresetCoolMorning>();
    execute<o::LightcraftPresetTealOrange>();
    execute<o::LightcraftPresetVividPop>();
    execute<o::LightcraftPresetSoftPastel>();
    execute<o::LightcraftPresetFadedMatte>();
    execute<o::LightcraftPresetWarmFilm>();
    execute<o::LightcraftPresetMutedFilm>();
    execute<o::LightcraftPresetBwHighContrast>();
    execute<o::LightcraftPresetBwSoft>();
    execute<o::LightcraftPresetBwSelenium>();
    execute<o::LightcraftPresetCrispLandscape>();
    execute<o::LightcraftPresetGoldenHour>();
    execute<o::LightcraftPresetBlueHour>();
    execute<o::LightcraftPresetSoftSkin>();
    execute<o::LightcraftPresetBrightAiry>();
    execute<o::LightcraftPresetMoody>();
    execute<o::LightcraftPresetCinematic>();
    execute<o::LightcraftPresetMoodyPortrait>();
    execute<o::LightcraftPresetGoldenGlow>();
    execute<o::LightcraftPresetCleanStudio>();
    execute<o::LightcraftPresetCrispVista>();
    execute<o::LightcraftPresetDeepSky>();
    execute<o::LightcraftPresetLushGreens>();
    execute<o::LightcraftPresetDesertWarmth>();
    execute<o::LightcraftPresetMistyMorning>();
    execute<o::LightcraftPresetGrittyStreet>();
    execute<o::LightcraftPresetNeonNight>();
    execute<o::LightcraftPresetConcreteCool>();
    execute<o::LightcraftPresetFadedUrban>();
    execute<o::LightcraftPresetFreshBright>();
    execute<o::LightcraftPresetWarmTable>();
    execute<o::LightcraftPresetDarkMoodyFood>();
    execute<o::LightcraftPresetAutumnGold>();
    execute<o::LightcraftPresetWinterBlue>();
    execute<o::LightcraftPresetSpringFresh>();
    execute<o::LightcraftPresetSummerHaze>();
    execute<o::LightcraftPresetInstant70s>();
    execute<o::LightcraftPresetCrossProcess>();
    execute<o::LightcraftPresetBleachBypass>();
    execute<o::LightcraftPresetBwSepia>();
    execute<o::ResizeBox>();
    execute<o::ResizeBilinear>();
    execute<o::ResizeMitchell>();
    execute<o::ResizeLanczos3>();
    execute<o::ApplyMaskedCorrection>();
    execute<o::GradientMap>();
    execute<o::ColorLookup>();
    execute<o::LinearMask>();
    execute<o::RadialMask>();
    execute<o::BrushMask>();
    execute<o::LuminanceRangeMask>();
    execute<o::ColorRangeMask>();
    execute<o::CombineMasks>();
    execute<o::InvertMask>();
    execute<o::RefineMask>();
    execute<o::Liquify>();
    execute<o::AdaptiveWideAngle>();
    execute<o::PuppetWarp>();
    execute<o::PerspectiveWarp>();
    execute<o::CorrectOptics>();
    execute<o::LensBlur>();
    execute<o::DestinationGamutWarning>();
    execute<o::DisplayGamutWarning>();
    execute<o::VanishingPoint>();
    execute<o::ApplyLocalAdjustments>();
    execute<o::Orientation>();
    execute<o::Crop>();
    execute<o::ColorHalftone>();
    execute<o::Crystallize>();
    execute<o::Facet>();
    execute<o::Fragment>();
    execute<o::Mezzotint>();
    execute<o::Pointillize>();
    execute<o::Mosaic>();
    execute<o::Emboss>();
    execute<o::FindEdges>();
    execute<o::Solarize>();
    execute<o::Diffuse>();
    execute<o::Extrude>();
    execute<o::OilPaint>();
    execute<o::Tiles>();
    execute<o::TraceContour>();
    execute<o::Wind>();
    execute<o::RawDevelopment>();
    execute<o::RawRender>();
    execute<o::Clouds>();
    execute<o::DifferenceClouds>();
    execute<o::Fibers>();
    execute<o::LensFlare>();
    execute<o::LightingEffects>();
    execute<o::Relight>();
    execute<o::Flame>();
    execute<o::Tree>();
    execute<o::PictureFrame>();
    execute<o::IdentityLut>();
    execute<o::CubeLut>();
    execute<o::PointColorSample>();
    execute<o::LookRecipe>();
    execute<o::PresetRecipe>();
    execute<o::GenericLensProfile>();
    execute<o::VanishingPointPlanes>();
    execute<o::CloneSpot>();
    execute<o::HealSpot>();
    execute<o::RemoveSpot>();
    execute<o::RedEye>();
    execute<o::PetEye>();
    execute<o::VisualizeSpots>();
    execute<o::BrightnessContrast>();
    execute<o::Levels>();
    execute<o::Curves>();
    execute<o::Exposure>();
    execute<o::ShadowsHighlights>();
    execute<o::Invert>();
    execute<o::Posterize>();
    execute<o::Threshold>();
    execute<o::Equalize>();
    execute<o::AutoLevels>();
    execute<o::AutoContrast>();
    execute<o::AutoColor>();
    execute<o::Average>();
    execute<o::FilterAddNoise>();
    execute<o::Median>();
    execute<o::DustAndScratches>();
    execute<o::Minimum>();
    execute<o::Maximum>();
    execute<o::ReduceNoise>();
    execute<o::CustomFilter>();
    execute<o::HsbHsl>();
    execute<o::DeInterlace>();
    execute<o::NtscColors>();
    execute<o::GuidedBlur>();
    execute<o::CrossGuidedBlur>();
    execute<o::SampleAverage8>();
    execute<o::SampleAverage16>();
    execute<o::SampleAverageFloat32>();
    execute<o::FillSampleAverage8>();
    execute<o::FillSampleAverage16>();
    execute<o::FillSampleAverageFloat32>();
    execute<o::SolarizeSamples8>();
    execute<o::SolarizeSamples16>();
    if(selected=="all" && static_cast<std::size_t>(calls)!=operationCatalog().size()) { std::cerr<<"Catalog execution count mismatch\n";return 1;}
    if(!calls) { std::cerr<<"No object selected\n";return 1;}
    std::cout<<"Executed "<<calls<<" objects\n";
    return failures?1:0;
}
