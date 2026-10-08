#include <algorithm>
#include <array>
#include <cmath>
#include <iiGraphicAdjust/CameraRawFilter.h>
#include <iostream>
#include <limits>
#include <string>
using namespace iiGraphicAdjust;
namespace {
using Rgb = std::array<double, 3>;
struct Oracle {
    const char *name;
    CameraRawOptions options;
    std::array<Rgb, 5> pixels;
};
// Values produced by the pinned Rust camera_raw::develop, with opaque pixels and float=false.
const std::vector<Oracle> oracles{
    {"temperature",
     [] {
         CameraRawOptions o;
         o.temperature = 35;
         return o;
     }(),
     {{{{0.106494181, 0.332947999, 0.268359244}},
       {{0.220155805, 0.141456276, 0.320598245}},
       {{0.703217566, 0.630824029, 0.216120332}},
       {{0.220155805, 0.269117415, 0.581793129}},
       {{0.419063598, 0.269117415, 0.451195717}}}}},
    {"tint",
     [] {
         CameraRawOptions o;
         o.tint = 40;
         return o;
     }(),
     {{{{0.104867361, 0.330675304, 0.299687773}},
       {{0.217383981, 0.140305370, 0.356987923}},
       {{0.695579588, 0.626806259, 0.242387682}},
       {{0.217383981, 0.267218620, 0.643488586}},
       {{0.414288044, 0.267218620, 0.500238240}}}}},
    {"exposure",
     [] {
         CameraRawOptions o;
         o.exposure = .5;
         return o;
     }(),
     {{{{0.124079667, 0.396412849, 0.342312932}},
       {{0.250118166, 0.173594773, 0.406499207}},
       {{0.785781682, 0.743018687, 0.278126717}},
       {{0.250118166, 0.322140127, 0.727430522}},
       {{0.470685482, 0.322140127, 0.566964865}}}}},
    {"contrast",
     [] {
         CameraRawOptions o;
         o.contrast = 60;
         return o;
     }(),
     {{{{0.080112852, 0.268950313, 0.231437132}},
       {{0.147561565, 0.100818463, 0.243084505}},
       {{0.714067161, 0.674779773, 0.247671962}},
       {{0.167766556, 0.217783928, 0.499247313}},
       {{0.332858413, 0.225868225, 0.402203947}}}}},
    {"highlights",
     [] {
         CameraRawOptions o;
         o.highlights = -70;
         return o;
     }(),
     {{{{0.100000001, 0.335714310, 0.288888901}},
       {{0.209090918, 0.142857149, 0.344444454}},
       {{0.663703978, 0.627187490, 0.230203643}},
       {{0.209090918, 0.271428585, 0.622222185}},
       {{0.400000006, 0.271428585, 0.483333349}}}}},
    {"shadows",
     [] {
         CameraRawOptions o;
         o.shadows = 70;
         return o;
     }(),
     {{{{0.118236661, 0.396937400, 0.341572583}},
       {{0.285628885, 0.195150167, 0.470528722}},
       {{0.672872305, 0.635851383, 0.233383641}},
       {{0.248151377, 0.322134376, 0.738460064}},
       {{0.480161279, 0.325823754, 0.580194890}}}}},
    {"whites",
     [] {
         CameraRawOptions o;
         o.whites = 55;
         return o;
     }(),
     {{{{0.100000001, 0.335714310, 0.288888901}},
       {{0.209090918, 0.142857149, 0.344444454}},
       {{0.716695964, 0.677263916, 0.248583749}},
       {{0.209090918, 0.271428585, 0.622222185}},
       {{0.400000006, 0.271428585, 0.483333349}}}}},
    {"blacks",
     [] {
         CameraRawOptions o;
         o.blacks = 55;
         return o;
     }(),
     {{{{0.109160759, 0.366468281, 0.315353304}},
       {{0.276986688, 0.189245567, 0.456292093}},
       {{0.672727287, 0.635714352, 0.233333349}},
       {{0.227915734, 0.295865774, 0.678241909}},
       {{0.422972530, 0.287017107, 0.511091828}}}}},
    {"texture",
     [] {
         CameraRawOptions o;
         o.texture = 80;
         return o;
     }(),
     {{{{0.074488051, 0.250067055, 0.215187714}},
       {{0.121305756, 0.082879707, 0.199832186}},
       {{0.751545668, 0.710196197, 0.260671258}},
       {{0.155989021, 0.202495053, 0.464199156}},
       {{0.347032994, 0.235486671, 0.419331521}}}}},
    {"clarity",
     [] {
         CameraRawOptions o;
         o.clarity = 80;
         return o;
     }(),
     {{{{0.094880544, 0.318527579, 0.274099350}},
       {{0.187767938, 0.128288656, 0.309318215}},
       {{0.707187295, 0.668278396, 0.245285705}},
       {{0.198557928, 0.257755339, 0.590877652}},
       {{0.388499051, 0.263624370, 0.469436347}}}}},
    {"dehaze",
     [] {
         CameraRawOptions o;
         o.dehaze = 60;
         return o;
     }(),
     {{{{0.062159017, 0.373375118, 0.219513759}},
       {{0.202280015, 0.128586695, 0.295885235}},
       {{0.773848772, 0.736685395, 0.162524909}},
       {{0.202362642, 0.289050281, 0.642780781}},
       {{0.445599943, 0.291333318, 0.467851877}}}}},
    {"dehaze",
     [] {
         CameraRawOptions o;
         o.dehaze = -60;
         return o;
     }(),
     {{{{0.126923084, 0.308919430, 0.338247895}},
       {{0.214384124, 0.153947681, 0.382183105}},
       {{0.589035928, 0.552147508, 0.291936666}},
       {{0.214373797, 0.257592469, 0.606080055}},
       {{0.367670029, 0.257316291, 0.494309574}}}}},
    {"vibrance",
     [] {
         CameraRawOptions o;
         o.vibrance = 70;
         return o;
     }(),
     {{{{0.062005043, 0.346868306, 0.290279299}},
       {{0.220006421, 0.134543508, 0.394656122}},
       {{0.686856806, 0.640857399, 0.140781194}},
       {{0.191587180, 0.268588364, 0.701898217}},
       {{0.433782399, 0.254669100, 0.549874365}}}}},
    {"saturation",
     [] {
         CameraRawOptions o;
         o.saturation = -50;
         return o;
     }(),
     {{{{0.191110328, 0.308967471, 0.285554767}},
       {{0.190291986, 0.157175094, 0.257968754}},
       {{0.643629313, 0.625122905, 0.423932374}},
       {{0.246296912, 0.277465761, 0.452862561}},
       {{0.357031196, 0.292745471, 0.398697853}}}}},
    {"curve_highlights",
     [] {
         CameraRawOptions o;
         o.curveHighlights = -40;
         return o;
     }(),
     {{{{0.099999942, 0.335575104, 0.288849145}},
       {{0.209087431, 0.142856821, 0.344271004}},
       {{0.637250423, 0.609762192, 0.233325750}},
       {{0.209087431, 0.271404475, 0.599493265}},
       {{0.399362087, 0.271404475, 0.480029315}}}}},
    {"curve_lights",
     [] {
         CameraRawOptions o;
         o.curveLights = 35;
         return o;
     }(),
     {{{{0.100068912, 0.347869039, 0.294729680}},
       {{0.210330009, 0.143101811, 0.358189940}},
       {{0.745981336, 0.716560960, 0.235404342}},
       {{0.210330009, 0.275731027, 0.704479158}},
       {{0.427270979, 0.275731027, 0.539287746}}}}},
    {"curve_darks",
     [] {
         CameraRawOptions o;
         o.curveDarks = 35;
         return o;
     }(),
     {{{{0.105867706, 0.411135852, 0.349866718}},
       {{0.240487278, 0.155797109, 0.421852112}},
       {{0.683475673, 0.653611362, 0.273416549}},
       {{0.240487278, 0.325962782, 0.643376470}},
       {{0.482841074, 0.325962782, 0.550671101}}}}},
    {"curve_shadows",
     [] {
         CameraRawOptions o;
         o.curveShadows = 45;
         return o;
     }(),
     {{{{0.139941216, 0.373127818, 0.339781880}},
       {{0.272686660, 0.197569892, 0.379293442}},
       {{0.672853410, 0.636031032, 0.295352817}},
       {{0.272686660, 0.326688260, 0.622657180}},
       {{0.420117348, 0.326688260, 0.489811718}}}}},
    {"point_curve",
     [] {
         CameraRawOptions o;
         o.pointCurve = {{0, 20}, {64, 45}, {192, 210}, {255, 255}};
         return o;
     }(),
     {{{{0.106726840, 0.262817204, 0.211543903}},
       {{0.147047445, 0.118498832, 0.273215950}},
       {{0.731563926, 0.682781756, 0.162726060}},
       {{0.147047445, 0.194607124, 0.664263248}},
       {{0.344410658, 0.194607124, 0.462478608}}}}},
    {"point_curve_red",
     [] {
         CameraRawOptions o;
         o.pointCurveRed = {{0, 5}, {255, 230}};
         return o;
     }(),
     {{{{0.107843146, 0.335714310, 0.288888901}},
       {{0.204099819, 0.142857149, 0.344444454}},
       {{0.613190770, 0.635714352, 0.233333349}},
       {{0.204099819, 0.271428585, 0.622222185}},
       {{0.372549087, 0.271428585, 0.483333349}}}}},
    {"point_curve_green",
     [] {
         CameraRawOptions o;
         o.pointCurveGreen = {{0, 5}, {255, 230}};
         return o;
     }(),
     {{{{0.100000001, 0.315826356, 0.288888901}},
       {{0.209090918, 0.145658270, 0.344444454}},
       {{0.672727287, 0.580532134, 0.233333349}},
       {{0.209090918, 0.259103686, 0.622222185}},
       {{0.400000006, 0.259103686, 0.483333349}}}}},
    {"point_curve_blue",
     [] {
         CameraRawOptions o;
         o.pointCurveBlue = {{0, 5}, {255, 230}};
         return o;
     }(),
     {{{{0.100000001, 0.335714310, 0.274509817}},
       {{0.209090918, 0.142857149, 0.323529392}},
       {{0.672727287, 0.635714352, 0.225490198}},
       {{0.209090918, 0.271428585, 0.568627417}},
       {{0.400000006, 0.271428585, 0.446078420}}}}},
    {"hsl_hue",
     [] {
         CameraRawOptions o;
         o.hslHue[0] = 50;
         o.hslHue[1] = 25;
         o.hslHue[5] = -35;
         return o;
     }(),
     {{{{0.099999994, 0.335714310, 0.288888901}},
       {{0.199993938, 0.145031974, 0.349688202}},
       {{0.665559828, 0.638095975, 0.230847344}},
       {{0.181051895, 0.288186789, 0.538782358}},
       {{0.399999946, 0.271428585, 0.483333379}}}}},
    {"hsl_sat",
     [] {
         CameraRawOptions o;
         o.hslSaturation[0] = 50;
         o.hslSaturation[1] = 25;
         o.hslSaturation[5] = -35;
         return o;
     }(),
     {{{{0.099999994, 0.335714310, 0.288888901}},
       {{0.201006934, 0.149014205, 0.307257801}},
       {{0.675421238, 0.636694968, 0.215687156}},
       {{0.244908839, 0.277240515, 0.459180862}},
       {{0.399999946, 0.271428585, 0.483333379}}}}},
    {"hsl_lum",
     [] {
         CameraRawOptions o;
         o.hslLuminance[0] = 50;
         o.hslLuminance[1] = 25;
         o.hslLuminance[5] = -35;
         return o;
     }(),
     {{{{0.099999994, 0.335714310, 0.288888901}},
       {{0.199155599, 0.136069030, 0.328077585}},
       {{0.685678124, 0.647952616, 0.237825304}},
       {{0.182362482, 0.236731470, 0.542682469}},
       {{0.399999946, 0.271428585, 0.483333379}}}}},
    {"grade_shadows",
     [] {
         CameraRawOptions o;
         o.gradeShadows = {220, 30, 5};
         return o;
     }(),
     {{{{0.080837838, 0.342356473, 0.339223713}},
       {{0.187693760, 0.147372007, 0.407011509}},
       {{0.670452952, 0.638192713, 0.244208887}},
       {{0.191646323, 0.277072370, 0.677486539}},
       {{0.386506617, 0.276670367, 0.532681286}}}}},
    {"grade_midtones",
     [] {
         CameraRawOptions o;
         o.gradeMidtones = {45, 30, 5};
         return o;
     }(),
     {{{{0.105568647, 0.337307066, 0.275022507}},
       {{0.212105393, 0.143199310, 0.337700278}},
       {{0.683345735, 0.639570773, 0.215148434}},
       {{0.215198830, 0.272726268, 0.609859824}},
       {{0.407723129, 0.272858769, 0.468984097}}}}},
    {"grade_highlights",
     [] {
         CameraRawOptions o;
         o.gradeHighlights = {35, 30, 5};
         return o;
     }(),
     {{{{0.101639785, 0.335843831, 0.286541194}},
       {{0.209090918, 0.142857149, 0.344444454}},
       {{0.698956549, 0.640691757, 0.204532549}},
       {{0.210875735, 0.271497756, 0.620148957}},
       {{0.403154612, 0.271539032, 0.479799092}}}}},
    {"grade_global",
     [] {
         CameraRawOptions o;
         o.gradeGlobal = {150, 30, 5};
         return o;
     }(),
     {{{{0.033063430, 0.365621686, 0.271866798}},
       {{0.144446582, 0.168712184, 0.328589708}},
       {{0.617824912, 0.671925306, 0.215143934}},
       {{0.144446582, 0.299985170, 0.612204134}},
       {{0.339367062, 0.299985170, 0.470396936}}}}},
    {"sharpen_amount",
     [] {
         CameraRawOptions o;
         o.sharpenAmount = 100;
         return o;
     }(),
     {{{{0.024465770, 0.260180086, 0.213354677}},
       {{0.130340904, 0.064107150, 0.265694439}},
       {{0.751477301, 0.714464426, 0.312083364}},
       {{0.151416093, 0.213753760, 0.564547360}},
       {{0.387693435, 0.259122014, 0.471026778}}}}},
    {"noise_luminance",
     [] {
         CameraRawOptions o;
         o.noiseLuminance = 80;
         return o;
     }(),
     {{{{0.111050993, 0.346765339, 0.299939901}},
       {{0.227936536, 0.161702767, 0.363290071}},
       {{0.652122319, 0.615109444, 0.212728381}},
       {{0.219887912, 0.282225609, 0.633019209}},
       {{0.406492800, 0.277921379, 0.489826143}}}}},
    {"noise_color",
     [] {
         CameraRawOptions o;
         o.noiseColor = 70;
         return o;
     }(),
     {{{{0.232346743, 0.296972662, 0.282948643}},
       {{0.183637738, 0.165018737, 0.199865162}},
       {{0.645550430, 0.609918177, 0.568890452}},
       {{0.280247182, 0.278455704, 0.343086332}},
       {{0.331572503, 0.304515630, 0.357070416}}}}},
    {"grain_amount",
     [] {
         CameraRawOptions o;
         o.grainAmount = 50;
         return o;
     }(),
     {{{{0.074725077, 0.310439378, 0.263613969}},
       {{0.188855231, 0.122621462, 0.324208766}},
       {{0.666141629, 0.629128695, 0.226747662}},
       {{0.207322448, 0.269660115, 0.620453715}},
       {{0.398189306, 0.269617885, 0.481522650}}}}},
    {"vignette_amount",
     [] {
         CameraRawOptions o;
         o.vignetteAmount = -80;
         return o;
     }(),
     {{{{0.032987695, 0.110744417, 0.095297791}},
       {{0.068974279, 0.047125284, 0.113624290}},
       {{0.221917227, 0.209707513, 0.076971292}},
       {{0.088690504, 0.115132391, 0.263929188}},
       {{0.131950781, 0.089538038, 0.159440532}}}}},
    {"combined",
     [] {
         CameraRawOptions o;
         o.temperature = 20;
         o.tint = -10;
         o.exposure = .25;
         o.contrast = 20;
         o.highlights = -30;
         o.shadows = 35;
         o.whites = 10;
         o.blacks = -10;
         o.texture = 20;
         o.clarity = 15;
         o.dehaze = 12;
         o.vibrance = 20;
         o.saturation = -5;
         o.curveShadows = 10;
         o.noiseLuminance = 20;
         o.noiseColor = 25;
         o.sharpenAmount = 35;
         o.sharpenRadius = .5;
         o.sharpenDetail = 60;
         o.sharpenMasking = 30;
         o.grainAmount = 12;
         o.vignetteAmount = -20;
         o.seed = 42;
         o.hslHue[0] = 50;
         o.hslHue[1] = 25;
         o.hslHue[5] = -35;
         o.gradeShadows = {210, 12, -5};
         return o;
     }(),
     {{{{0.095857725, 0.214277923, 0.146506712}},
       {{0.079006322, 0.084980249, 0.062757470}},
       {{0.605657995, 0.605024755, 0.499736547}},
       {{0.176791206, 0.217364997, 0.204212487}},
       {{0.290496409, 0.229844123, 0.298251420}}}}},
};
Image fixture() {
    Image out{17, 11, {}};
    for (int i = 0; i < 187; ++i)
        out.pixels.push_back({.1F + .6F * ((i * 17) % 23) / 22.F,
                              .1F + .6F * ((i * 7 + 11) % 29) / 28.F,
                              .15F + .5F * ((i * 3 + 5) % 19) / 18.F, 1});
    return out;
}
bool finite(const Image &output) {
    for (auto p : output.pixels)
        for (float v : {p.r, p.g, p.b, p.a})
            if (!std::isfinite(v) || v < 0 || v > 1)
                return false;
    return true;
}
double variance(const Image &image) {
    double mean = 0, variance = 0;
    for (auto p : image.pixels)
        mean += p.g;
    mean /= image.pixels.size();
    for (auto p : image.pixels)
        variance += (p.g - mean) * (p.g - mean);
    return variance / image.pixels.size();
}
constexpr std::array<double CameraRawOptions::*, 36> scalarFields{
    &CameraRawOptions::temperature,
    &CameraRawOptions::tint,
    &CameraRawOptions::exposure,
    &CameraRawOptions::contrast,
    &CameraRawOptions::highlights,
    &CameraRawOptions::shadows,
    &CameraRawOptions::whites,
    &CameraRawOptions::blacks,
    &CameraRawOptions::texture,
    &CameraRawOptions::clarity,
    &CameraRawOptions::dehaze,
    &CameraRawOptions::vibrance,
    &CameraRawOptions::saturation,
    &CameraRawOptions::curveHighlights,
    &CameraRawOptions::curveLights,
    &CameraRawOptions::curveDarks,
    &CameraRawOptions::curveShadows,
    &CameraRawOptions::gradeBlending,
    &CameraRawOptions::gradeBalance,
    &CameraRawOptions::sharpenAmount,
    &CameraRawOptions::sharpenRadius,
    &CameraRawOptions::sharpenDetail,
    &CameraRawOptions::sharpenMasking,
    &CameraRawOptions::noiseLuminance,
    &CameraRawOptions::noiseLuminanceDetail,
    &CameraRawOptions::noiseColor,
    &CameraRawOptions::noiseColorDetail,
    &CameraRawOptions::grainAmount,
    &CameraRawOptions::grainSize,
    &CameraRawOptions::grainRoughness,
    &CameraRawOptions::vignetteAmount,
    &CameraRawOptions::vignetteMidpoint,
    &CameraRawOptions::vignetteRoundness,
    &CameraRawOptions::vignetteFeather,
    &CameraRawOptions::vignetteHighlights,
    &CameraRawOptions::pixelScale};
} // namespace
int main(int argc, char **argv) {
    const std::string group = argc > 1 ? argv[1] : "all";
    auto run = [&](const char *name) { return group == "all" || group == name; };
    int failures = 0;
    auto check = [&](bool ok, const std::string &why) {
        if (!ok) {
            std::cerr << why << '\n';
            ++failures;
        }
    };
    auto near = [](double a, double b, double tolerance = 5e-5) {
        return std::abs(a - b) <= tolerance;
    };
    const auto image = fixture();
    if (run("oracle")) {
        constexpr std::array<std::size_t, 5> indices{0, 7, 31, 99, 186};
        for (const auto &oracle : oracles) {
            auto out = cameraRawFilter(image.view(), oracle.options);
            check(out && finite(*out), std::string(oracle.name) + " finite output");
            if (out)
                for (std::size_t i = 0; i < indices.size(); ++i) {
                    const auto actual = out->pixels[indices[i]];
                    const auto &expected = oracle.pixels[i];
                    check(near(actual.r, expected[0]) && near(actual.g, expected[1]) &&
                              near(actual.b, expected[2]),
                          std::string(oracle.name) + " Rust numeric oracle " + std::to_string(i));
                }
        }
    }
    if (run("basic")) {
        auto neutral = cameraRawFilter(image.view());
        check(neutral && neutral->pixels == image.pixels &&
                  neutral->pixels.data() != image.pixels.data(),
              "neutral detached exact copy");
        CameraRawOptions o;
        o.exposure = 1;
        auto exposed = cameraRawFilter(Image{1, 1, {{.5F, .5F, .5F, .2F}}}.view(), o);
        check(exposed && near(exposed->pixels[0].r, .68583611906437) && exposed->pixels[0].a == .2F,
              "one stop sRGB exposure oracle");
        o = {};
        o.temperature = 50;
        auto warm = cameraRawFilter(Image{1, 1, {{.5F, .5F, .5F, 1}}}.view(), o);
        check(warm && warm->pixels[0].r > .5F && warm->pixels[0].b < .5F,
              "relative temperature warms directly");
        o = {};
        o.tint = 60;
        auto tint = cameraRawFilter(Image{1, 1, {{.5F, .5F, .5F, 1}}}.view(), o);
        check(tint && tint->pixels[0].r > tint->pixels[0].g &&
                  tint->pixels[0].b > tint->pixels[0].g,
              "positive tint adds magenta");
        o = {};
        o.exposure = 2;
        o.saturation = -100;
        auto combined = cameraRawFilter(Image{1, 1, {{.8F, .1F, .1F, 1}}}.view(), o);
        auto transfer = [](double v) {
            double l = v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
            l *= 4;
            return l <= .0031308 ? l * 12.92 : 1.055 * std::pow(l, 1 / 2.4) - .055;
        };
        const auto gray = .2126 * transfer(.8F) + (.7152 + .0722) * transfer(.1F);
        check(combined && near(combined->pixels[0].r, gray) &&
                  combined->pixels[0].r == combined->pixels[0].g &&
                  combined->pixels[0].r == combined->pixels[0].b,
              "extended intermediate exposure is preserved before saturation and final clamp");
    }
    if (run("color")) {
        CameraRawOptions o;
        o.saturation = -100;
        auto gray = cameraRawFilter(image.view(), o);
        check(gray && std::all_of(gray->pixels.begin(), gray->pixels.end(),
                                  [&](Rgba p) { return near(p.r, p.g) && near(p.g, p.b); }),
              "desaturation produces BT709 encoded gray");
        o = {};
        o.pointCurve = {{0, 30}, {255, 255}};
        auto lifted = cameraRawFilter(
            Image{3, 1, {{0, 0, 0, 1}, {.5F, .5F, .5F, 1}, {1, 1, 1, 1}}}.view(), o);
        check(lifted && near(lifted->pixels[0].r, 30 / 255.) &&
                  near(lifted->pixels[1].r, (30 + 225 * .5) / 255.) && lifted->pixels[2].r == 1,
              "point-curve linear endpoint oracle");
        for (auto curve : {&CameraRawOptions::pointCurve, &CameraRawOptions::pointCurveRed,
                           &CameraRawOptions::pointCurveGreen, &CameraRawOptions::pointCurveBlue}) {
            o = {};
            o.*curve = {{0, 20}, {64, 45}, {192, 210}, {255, 255}};
            auto out = cameraRawFilter(image.view(), o);
            check(out && out->pixels != image.pixels, "each point-curve channel acts");
        }
        o = {};
        o.curveShadows = 70;
        auto a = cameraRawFilter(image.view(), o);
        o.curveSplits = {15, 45, 85};
        auto b = cameraRawFilter(image.view(), o);
        check(a && b && a->pixels != b->pixels,
              "curve region splits affect active parametric control");
        for (std::size_t band = 0; band < 8; ++band)
            for (auto member : {&CameraRawOptions::hslHue, &CameraRawOptions::hslSaturation,
                                &CameraRawOptions::hslLuminance}) {
                o = {};
                (o.*member)[band] = 60;
                auto out = cameraRawFilter(image.view(), o);
                check(out && out->pixels != image.pixels, "each HSL band and component acts");
            }
        o = {};
        o.gradeShadows = {220, 40, 5};
        o.gradeMidtones = {45, 30, 2};
        o.gradeHighlights = {30, 30, 5};
        auto aGrade = cameraRawFilter(image.view(), o);
        o.gradeBlending = 0;
        auto bGrade = cameraRawFilter(image.view(), o);
        o.gradeBalance = 80;
        auto cGrade = cameraRawFilter(image.view(), o);
        check(aGrade && bGrade && cGrade && aGrade->pixels != bGrade->pixels &&
                  bGrade->pixels != cGrade->pixels,
              "grade blending and balance affect active wheels");
    }
    if (run("detail")) {
        Image noise{24, 16, {}};
        for (int i = 0; i < 384; ++i) {
            float v = .4F + ((i * 73) % 97) / 960.F;
            noise.pixels.push_back({v, v, v, 1});
        }
        CameraRawOptions o;
        o.noiseLuminance = 80;
        auto nr = cameraRawFilter(noise.view(), o);
        check(nr && variance(*nr) < variance(noise) * .6, "luminance NR reduces variance");
        auto low = nr;
        o.noiseLuminanceDetail = 100;
        auto high = cameraRawFilter(noise.view(), o);
        check(low && high && variance(*high) > variance(*low),
              "luminance NR Detail preserves more variance");
        o = {};
        o.sharpenAmount = 100;
        o.sharpenRadius = .5;
        auto small = cameraRawFilter(noise.view(), o);
        check(small && variance(*small) > variance(noise),
              "radius .5 real sharpening includes upstream small-sigma fix");
        o.sharpenRadius = 3;
        auto large = cameraRawFilter(noise.view(), o);
        check(large && small && large->pixels != small->pixels,
              "sharpen radius controls spatial scale");
        o.sharpenDetail = 100;
        auto detail = cameraRawFilter(noise.view(), o);
        check(detail && large && detail->pixels != large->pixels, "sharpen Detail affects halos");
        o.sharpenMasking = 100;
        auto masked = cameraRawFilter(noise.view(), o);
        check(masked && detail && masked->pixels != detail->pixels, "sharpen Masking acts");
        o = {};
        o.noiseColor = 80;
        auto colorLow = cameraRawFilter(image.view(), o);
        o.noiseColorDetail = 100;
        auto colorHigh = cameraRawFilter(image.view(), o);
        check(colorLow && colorHigh && colorLow->pixels != image.pixels &&
                  colorLow->pixels != colorHigh->pixels,
              "color NR amount and Detail act");
        o.pixelScale = .3;
        auto scaled = cameraRawFilter(image.view(), o);
        check(scaled && colorHigh && scaled->pixels != colorHigh->pixels,
              "pixel scale changes active detail radii");
    }
    if (run("effects")) {
        CameraRawOptions o;
        o.grainAmount = 40;
        o.seed = 42;
        auto a = cameraRawFilter(image.view(), o), b = cameraRawFilter(image.view(), o);
        check(a && b && a->pixels == b->pixels && a->pixels != image.pixels, "grain deterministic");
        for (auto member : {&CameraRawOptions::grainSize, &CameraRawOptions::grainRoughness}) {
            auto changed = o;
            changed.*member = 80;
            auto out = cameraRawFilter(image.view(), changed);
            check(a && out && out->pixels != a->pixels, "grain size and roughness act");
        }
        o.seed = 43;
        auto seed = cameraRawFilter(image.view(), o);
        check(a && seed && seed->pixels != a->pixels, "grain seed acts");
        o.pixelScale = .3;
        auto scale = cameraRawFilter(image.view(), o);
        check(scale && seed && scale->pixels != seed->pixels,
              "grain samples scaled full-resolution positions");
        o = {};
        o.vignetteAmount = -80;
        auto vig = cameraRawFilter(image.view(), o);
        check(vig && vig->pixels[0].r < image.pixels[0].r, "vignette darkens corner");
        for (auto member :
             {&CameraRawOptions::vignetteMidpoint, &CameraRawOptions::vignetteRoundness,
              &CameraRawOptions::vignetteFeather, &CameraRawOptions::vignetteHighlights}) {
            auto changed = o;
            changed.*member = 100;
            auto out = cameraRawFilter(image.view(), changed);
            check(vig && out && out->pixels != vig->pixels, "every vignette control acts");
        }
        for (auto style :
             {CameraRawVignetteStyle::ColorPriority, CameraRawVignetteStyle::PaintOverlay}) {
            o.vignetteStyle = style;
            auto out = cameraRawFilter(image.view(), o);
            check(vig && out && out->pixels != vig->pixels, "each vignette style acts");
        }
    }
    if (run("validation")) {
        for (auto member : scalarFields) {
            CameraRawOptions o;
            o.*member = std::numeric_limits<double>::quiet_NaN();
            check(!cameraRawFilter(image.view(), o), "every scalar rejects NaN");
        }
        for (auto member : {&CameraRawOptions::hslHue, &CameraRawOptions::hslSaturation,
                            &CameraRawOptions::hslLuminance})
            for (std::size_t i = 0; i < 8; ++i) {
                CameraRawOptions o;
                (o.*member)[i] = 101;
                check(!cameraRawFilter(image.view(), o), "each HSL value bounded");
            }
        for (auto member : {&CameraRawOptions::gradeShadows, &CameraRawOptions::gradeMidtones,
                            &CameraRawOptions::gradeHighlights, &CameraRawOptions::gradeGlobal}) {
            CameraRawOptions o;
            (o.*member).hue = std::numeric_limits<double>::quiet_NaN();
            check(!cameraRawFilter(image.view(), o), "each wheel rejects invalid hue");
            o = {};
            (o.*member).saturation = 101;
            check(!cameraRawFilter(image.view(), o), "each wheel saturation bounded");
            o = {};
            (o.*member).luminance = -101;
            check(!cameraRawFilter(image.view(), o), "each wheel luminance bounded");
        }
        for (auto member :
             {&CameraRawOptions::pointCurve, &CameraRawOptions::pointCurveRed,
              &CameraRawOptions::pointCurveGreen, &CameraRawOptions::pointCurveBlue}) {
            for (auto points : {std::vector<std::array<double, 2>>{{128, 128}},
                                {{0, 0}, {0, 255}},
                                {{0, 0}, {.9, 10}},
                                {{255, 255}, {0, 0}},
                                {{0, -1}, {255, 255}},
                                {{0, 0}, {256, 255}},
                                {{0, 0}, {255, std::numeric_limits<double>::quiet_NaN()}}}) {
                CameraRawOptions o;
                o.*member = points;
                check(!cameraRawFilter(image.view(), o), "each curve malformed input rejected");
            }
            CameraRawOptions o;
            for (int i = 0; i < 17; ++i)
                (o.*member).push_back({double(i * 15), double(i * 15)});
            check(!cameraRawFilter(image.view(), o), "curve point count bounded");
        }
        CameraRawOptions o;
        o.curveSplits = {25, 25, 75};
        check(!cameraRawFilter(image.view(), o), "unordered splits rejected");
        o = {};
        o.curveSplits[0] = 4;
        check(!cameraRawFilter(image.view(), o), "split bounds validated");
        o = {};
        o.vignetteStyle = static_cast<CameraRawVignetteStyle>(999);
        check(!cameraRawFilter(image.view(), o), "unknown vignette style rejected");
        o = {};
        o.exposure = 5.01;
        check(!cameraRawFilter(image.view(), o), "exposure bounds validated");
        o = {};
        o.sharpenRadius = .49;
        check(!cameraRawFilter(image.view(), o), "radius bounds validated at neutral amount");
        o = {};
        o.pixelScale = 0;
        check(!cameraRawFilter(image.view(), o), "pixelScale bounds validated");
        check(!cameraRawFilter(ImageView{0, 1, {}}) &&
                  !cameraRawFilter(ImageView{std::numeric_limits<std::size_t>::max(), 2, {}}),
              "dimensions and overflow validated");
        check(!cameraRawFilter(ImageView{65537, 1, {}}) &&
                  !cameraRawFilter(ImageView{65536, 65536, {}}),
              "bounded Camera Raw dimensions");
        check(!cameraRawFilter(ImageView{2, 2, std::span<const Rgba>(image.pixels.data(), 3)}),
              "short buffer validated");
        Image invalid = image;
        invalid.pixels[0].a = -1;
        check(!cameraRawFilter(invalid.view()), "neutral validates alpha");
        invalid = image;
        invalid.pixels[0].r = std::numeric_limits<float>::infinity();
        check(!cameraRawFilter(invalid.view()), "neutral validates RGB");
    }
    if (run("invariants")) {
        Image transparent = image;
        for (std::size_t i = 0; i < transparent.pixels.size(); ++i)
            transparent.pixels[i].a = i % 5 == 0 ? 0.F : i % 3 == 0 ? .3F : 1.F;
        Image changed = transparent;
        for (auto &p : changed.pixels)
            if (p.a == 0)
                p = {1, 0, 1, 0};
        for (const auto &oracle : oracles) {
            auto a = cameraRawFilter(transparent.view(), oracle.options),
                 b = cameraRawFilter(changed.view(), oracle.options);
            check(a && b && finite(*a) && finite(*b), "transparent output finite");
            if (a && b)
                for (std::size_t i = 0; i < a->pixels.size(); ++i) {
                    check(a->pixels[i].a == transparent.pixels[i].a, "filter preserves alpha");
                    if (transparent.pixels[i].a == 0)
                        check(a->pixels[i] == transparent.pixels[i] &&
                                  b->pixels[i] == changed.pixels[i],
                              "filter preserves hidden source pixels");
                    else
                        check(a->pixels[i] == b->pixels[i],
                              "hidden RGB excluded from every spatial stage");
                }
        }
        unsigned state = 0x734a1201;
        auto random = [&]() {
            state = state * 1664525U + 1013904223U;
            return double(state >> 8) / 16777216.;
        };
        for (int i = 0; i < 80; ++i) {
            CameraRawOptions o;
            for (auto member : scalarFields)
                o.*member = -100 + 200 * random();
            o.exposure = -5 + 10 * random();
            o.sharpenAmount = 150 * random();
            o.sharpenRadius = .5 + 2.5 * random();
            o.pixelScale = .01 + .99 * random();
            for (auto member :
                 {&CameraRawOptions::gradeBlending, &CameraRawOptions::sharpenDetail,
                  &CameraRawOptions::sharpenMasking, &CameraRawOptions::noiseLuminance,
                  &CameraRawOptions::noiseLuminanceDetail, &CameraRawOptions::noiseColor,
                  &CameraRawOptions::noiseColorDetail, &CameraRawOptions::grainAmount,
                  &CameraRawOptions::grainSize, &CameraRawOptions::grainRoughness,
                  &CameraRawOptions::vignetteMidpoint, &CameraRawOptions::vignetteFeather,
                  &CameraRawOptions::vignetteHighlights})
                o.*member = 100 * random();
            for (std::size_t k = 0; k < 8; ++k) {
                o.hslHue[k] = -100 + 200 * random();
                o.hslSaturation[k] = -100 + 200 * random();
                o.hslLuminance[k] = -100 + 200 * random();
            }
            for (auto member : {&CameraRawOptions::gradeShadows, &CameraRawOptions::gradeMidtones,
                                &CameraRawOptions::gradeHighlights, &CameraRawOptions::gradeGlobal})
                o.*member = {360 * random(), 100 * random(), -100 + 200 * random()};
            o.pointCurve = {{0, 30 * random()}, {255, 225 + 30 * random()}};
            o.seed = state;
            auto out = cameraRawFilter(transparent.view(), o);
            check(out && finite(*out), "deterministic combined-options finite invariant");
        }
        check(image.pixels == fixture().pixels, "borrowed source untouched");
    }
    check(group == "all" || group == "oracle" || group == "basic" || group == "color" ||
              group == "detail" || group == "effects" || group == "validation" ||
              group == "invariants",
          "unknown test group");
    if (!failures)
        std::cout << "Camera Raw " << group << ": passed\n";
    return failures ? 1 : 0;
}
