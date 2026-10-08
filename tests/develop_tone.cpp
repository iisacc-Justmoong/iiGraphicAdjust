#include <cmath>
#include <iiGraphicAdjust/DevelopTone.h>
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
        4, 1, {{.1F, .1F, .1F, 1}, {.5F, .5F, .5F, .4F}, {.9F, .9F, .9F, 1}, {.3F, .6F, .9F, 0}}};
    if (name == "wb") {
        const auto neutral = whiteBalance(image.view());
        check(neutral && neutral->pixels == image.pixels, "neutral WB exact copy");
        auto warm = whiteBalance(image.view(), {9000, 0});
        check(warm && warm->pixels[1].r > warm->pixels[1].b,
              "increasing selected Kelvin warms output");
        auto tint = whiteBalance(image.view(), {6500, 50});
        check(tint && tint->pixels[1].r > tint->pixels[1].g, "positive tint adds magenta");
        auto preset = whiteBalancePreset(WhiteBalancePreset::Tungsten);
        check(preset && preset->temperature == 2850, "preset values");
        auto gray = sampledWhiteBalance({.5F, .5F, .5F, 1});
        check(gray && std::abs(gray->temperature - 6500) < 30 && std::abs(gray->tint) < 1,
              "gray eyedropper preserves source white");
        auto sampled = sampledWhiteBalance({.6F, .5F, .4F, 1});
        auto corrected = sampled ? whiteBalance(Image{1, 1, {{.6F, .5F, .4F, 1}}}.view(), *sampled)
                                 : Result<Image>{std::unexpected(Error{})};
        check(corrected && std::abs(corrected->pixels[0].r - corrected->pixels[0].b) < .01,
              "eyedropper neutralizes warm sample");
        check(!sampledWhiteBalance({0, 0, 0, 1}) && !whiteBalance(image.view(), {1000, 0}),
              "invalid WB input");
        check(warm && warm->pixels[1].a == .4F && warm->pixels[3] == image.pixels[3],
              "WB preserves alpha and hidden RGB");
    } else if (name == "light") {
        auto neutral = developLight(image.view());
        check(neutral && neutral->pixels == image.pixels, "neutral Light exact copy");
        DevelopLightOptions o;
        o.exposure = 1;
        auto exposed = developLight(image.view(), o);
        check(exposed && std::abs(exposed->pixels[1].r - .685836F) < 2e-5,
              "one stop reference value");
        o = {};
        o.shadows = 70;
        auto sh = developLight(image.view(), o);
        check(sh && sh->pixels[0].r > image.pixels[0].r, "positive Shadows lifts dark tones");
        o = {};
        o.highlights = -70;
        auto hl = developLight(image.view(), o);
        check(hl && hl->pixels[2].r < image.pixels[2].r,
              "negative Highlights recovers bright tones");
        o = {};
        o.contrast = 60;
        auto con = developLight(image.view(), o);
        check(con && con->pixels[0].r < image.pixels[0].r && con->pixels[2].r > image.pixels[2].r,
              "contrast separates dark and light");
        auto a = autoDevelopLight(image.view());
        check(a && std::isfinite(a->exposure), "Auto Light returns finite settings");
        o.exposure = 6;
        check(!developLight(image.view(), o), "out-of-range exposure rejected");
        auto hidden = image;
        hidden.pixels[3] = {1, 0, 1, 0};
        o = {};
        o.shadows = 80;
        auto x = developLight(image.view(), o), y = developLight(hidden.view(), o);
        check(x && y && x->pixels[2] == y->pixels[2],
              "hidden RGB excluded from spatial statistics");
    } else if (name == "curve") {
        auto n = developCurve(image.view());
        check(n && n->pixels == image.pixels, "neutral curves exact");
        DevelopCurveOptions o;
        o.master.points = {{0, 0}, {.25, .15}, {.5, .5}, {.75, .9}, {1, 1}};
        Image ramp{1001, 1, {}};
        for (int i = 0; i <= 1000; ++i) {
            float x = i / 1000.F;
            ramp.pixels.push_back({x, x, x, 1});
        }
        auto c = developCurve(ramp.view(), o);
        check(c && std::abs(c->pixels[750].r - .9) < 2e-6, "cubic curve passes through knots");
        if (c)
            for (std::size_t i = 1; i < c->pixels.size(); ++i)
                check(c->pixels[i].r >= c->pixels[i - 1].r, "no accidental tone inversion");
        o = {};
        o.darks = 70;
        auto d = developCurve(ramp.view(), o);
        check(d && d->pixels[350].r > ramp.pixels[350].r && d->pixels[0].r == 0 &&
                  d->pixels.back().r == 1,
              "parametric dark region and endpoints");
        o.splitMid = o.splitShadows;
        check(!developCurve(image.view(), o), "unordered splits rejected");
        o = {};
        o.red.points = {{0, 0}, {0, 1}};
        check(!developCurve(image.view(), o), "duplicate point inputs rejected");
        o = {};
        o.master.points = {{0, .5}, {1, .5}};
        o.refineSaturation = 0;
        Image hues{3, 1, {{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}}};
        auto gray = developCurve(hues.view(), o);
        check(gray && gray->pixels[0] == Rgba{.5F, .5F, .5F, 1} &&
                  gray->pixels[2] == gray->pixels[0],
              "Refine Saturation must not amplify numerical gray chroma");
    } else {
        return 2;
    }
    return failures ? 1 : 0;
}
