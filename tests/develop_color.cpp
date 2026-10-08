#include <algorithm>
#include <cmath>
#include <iiGraphicAdjust/DevelopColor.h>
#include <iostream>
#include <limits>
using namespace iiGraphicAdjust;
int main(int argc, char **argv) {
    int failures = 0;
    auto check = [&](bool ok, const char *why) {
        if (!ok) {
            std::cerr << why << '\n';
            ++failures;
        }
    };
    const std::string name = argc > 1 ? argv[1] : "";
    const Image image{
        4, 1, {{.8F, .1F, .05F, 1}, {.2F, .3F, .8F, .6F}, {.5F, .5F, .5F, 1}, {.2F, .9F, .4F, 0}}};
    const auto chroma = [](Rgba p) {
        return std::max({p.r, p.g, p.b}) - std::min({p.r, p.g, p.b});
    };
    if (name == "mixer") {
        auto n = developColor(image.view());
        check(n && n->pixels == image.pixels, "neutral color exact copy");
        DevelopColorOptions o;
        o.mixer[static_cast<int>(MixerBand::Blue)].saturation = -100;
        auto b = developColor(image.view(), o);
        check(b && chroma(b->pixels[1]) < chroma(image.pixels[1]) * .35 &&
                  chroma(b->pixels[0]) > .65,
              "blue band selected independently");
        o = {};
        o.saturation = -100;
        auto gray = developColor(image.view(), o);
        check(gray && chroma(gray->pixels[0]) < 1e-5, "perceptual desaturation");
        o = {};
        o.vibrance = 70;
        auto v = developColor(image.view(), o);
        check(v && chroma(v->pixels[2]) < 1e-5 && v->pixels[3] == image.pixels[3],
              "vibrance preserves gray and hidden colors");
        o = {};
        o.blackAndWhite = true;
        o.blackAndWhiteMix[5] = 80;
        auto bw = developColor(image.view(), o);
        o.blackAndWhiteMix[5] = 0;
        auto flat = developColor(image.view(), o);
        check(bw && flat && chroma(bw->pixels[1]) < 1e-5 && bw->pixels[1].r > flat->pixels[1].r,
              "eight-band B&W brightens blue");
        auto mix = autoBlackAndWhiteMix(image.view());
        check(mix.has_value(), "Auto B&W returns settings");
        o.points.resize(9);
        check(!developColor(image.view(), o), "sample limit enforced");
    } else if (name == "point") {
        auto sample = samplePointColor(image.pixels[0]);
        check(sample.has_value(), "sample OkLCh");
        DevelopColorOptions o;
        if (sample) {
            sample->saturationShift = -80;
            o.points.push_back(*sample);
        }
        auto selected = developColor(image.view(), o);
        check(selected && chroma(selected->pixels[0]) < chroma(image.pixels[0]) * .4,
              "sample desaturates its range");
        check(selected && std::abs(selected->pixels[1].b - image.pixels[1].b) < 2e-6,
              "far colors untouched");
        check(!samplePointColor({1, 0, 0, 0}), "invisible point rejected");
        o.blackAndWhite = true;
        auto bw = developColor(image.view(), o);
        o.points.clear();
        auto noPoint = developColor(image.view(), o);
        check(bw && noPoint && bw->pixels == noPoint->pixels,
              "B&W disables Point Color as in the reference");
    } else if (name == "grading") {
        Image ramp{2, 1, {{.08F, .08F, .08F, 1}, {.9F, .9F, .9F, 1}}};
        DevelopColorOptions o;
        o.grading.shadows = {220, 60, 0};
        auto g = developColor(ramp.view(), o);
        check(g && g->pixels[0].b > g->pixels[0].r &&
                  std::abs(g->pixels[1].b - g->pixels[1].r) < .005,
              "shadow wheel applies blue tint to darks");
        o = {};
        o.grading.global = {40, 50, 20};
        auto global = developColor(ramp.view(), o);
        check(global && global->pixels[0].r > ramp.pixels[0].r, "global wheel luminance");
        o.grading.blending = 101;
        check(!developColor(image.view(), o), "grading range enforced");
    } else if (name == "calibration") {
        auto n = calibrate(image.view());
        check(n && n->pixels == image.pixels, "neutral calibration exact");
        CalibrationOptions o;
        o.primaryHue[0] = 60;
        o.primarySaturation[2] = -50;
        auto c = calibrate(image.view(), o);
        check(c && std::abs(c->pixels[2].r - .5) < 2e-6 && std::abs(c->pixels[2].b - .5) < 2e-6,
              "calibration preserves neutral axis");
        check(c && c->pixels[0] != image.pixels[0] && c->pixels[3] == image.pixels[3],
              "calibration changes primaries preserving hidden RGB");
        Image dark{1, 1, {{.08F, .08F, .08F, 1}}};
        o = {};
        o.shadowsTint = 100;
        auto tint = calibrate(dark.view(), o);
        check(tint && tint->pixels[0].r > tint->pixels[0].g, "calibration shadow magenta");
        o.primaryHue[0] = std::numeric_limits<double>::quiet_NaN();
        check(!calibrate(image.view(), o), "nonfinite calibration rejected");
    } else
        return 2;
    return failures ? 1 : 0;
}
