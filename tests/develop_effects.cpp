#include <cmath>
#include <iiGraphicAdjust/DevelopEffects.h>
#include <iostream>
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
    Image flat{32, 32, std::vector<Rgba>(1024, {.5F, .5F, .5F, 1})};
    if (name == "effects") {
        auto n = developEffects(flat.view());
        check(n && n->pixels == flat.pixels, "neutral effects exact");
        auto uniform = developEffects(flat.view(), {100, 100, 0});
        check(uniform && std::abs(uniform->pixels[0].r - .5F) < 2e-6,
              "texture/clarity do not change flat fields");
        auto texture = flat;
        for (std::size_t i = 0; i < texture.pixels.size(); ++i) {
            float v = i % 2 ? .52F : .48F;
            texture.pixels[i] = {v, v, v, 1};
        }
        auto strong = developEffects(texture.view(), {100, 0, 0}),
             soft = developEffects(texture.view(), {-100, 0, 0});
        check(strong && soft &&
                  strong->pixels[1].r - strong->pixels[0].r > soft->pixels[1].r - soft->pixels[0].r,
              "Texture controls fine contrast");
        auto haze = developEffects(flat.view(), {0, 0, -70});
        check(haze && haze->pixels[0].r < flat.pixels[0].r,
              "negative dehaze moves toward estimated veil");
        check(!developEffects(flat.view(), {0, 0, 101}), "effects ranges validated");
    } else if (name == "vignette") {
        PostCropVignetteOptions o;
        o.amount = -80;
        auto v = postCropVignette(flat.view(), o);
        check(v && v->pixels[0].r < v->pixels[16 * 32 + 16].r && v->pixels[16 * 32 + 16].r == .5F,
              "vignette darkens edges preserving center");
        o.amount = 80;
        auto bright = postCropVignette(flat.view(), o);
        check(bright && bright->pixels[0].r > .5, "positive vignette lifts edges");
        o.style = static_cast<VignetteStyle>(42);
        check(!postCropVignette(flat.view(), o), "invalid vignette style");
    } else if (name == "grain") {
        FilmGrainOptions o{70, 40, 60, 42};
        auto a = filmGrain(flat.view(), o), b = filmGrain(flat.view(), o);
        o.seed = 43;
        auto c = filmGrain(flat.view(), o);
        check(a && b && c && a->pixels == b->pixels && a->pixels != c->pixels,
              "film grain seed determinism");
        check(a && a->pixels != flat.pixels && a->pixels[0].r == a->pixels[0].g,
              "grain is visible and monochrome");
        flat.pixels[0] = {.1F, .2F, .3F, 0};
        auto alpha = filmGrain(flat.view(), o);
        check(alpha && alpha->pixels[0] == flat.pixels[0], "grain preserves hidden color");
    } else if (name == "detail") {
        auto n = developDetail(flat.view());
        check(n && n->pixels == flat.pixels, "neutral detail exact");
        auto noise = flat;
        for (std::size_t i = 0; i < noise.pixels.size(); ++i) {
            float v = i % 2 ? .53F : .47F;
            noise.pixels[i] = {v, v, v, 1};
        }
        DevelopDetailOptions o;
        o.luminanceNoise = 100;
        auto smooth = developDetail(noise.view(), o);
        check(smooth && std::abs(smooth->pixels[1].r - smooth->pixels[0].r) < .06,
              "luminance NR reduces oscillation");
        o = {};
        o.sharpenAmount = 100;
        auto sharp = developDetail(noise.view(), o);
        check(sharp && sharp->pixels[1].r - sharp->pixels[0].r > .06,
              "sharpen increases fine contrast");
        auto color = flat;
        for (std::size_t i = 0; i < color.pixels.size(); ++i)
            color.pixels[i] = i % 2 ? Rgba{.6F, .5F, .4F, 1} : Rgba{.4F, .5F, .6F, 1};
        o = {};
        o.colorNoise = 100;
        auto nr = developDetail(color.view(), o);
        check(nr && std::abs(nr->pixels[1].r - nr->pixels[0].r) < .2,
              "color NR reduces chromatic oscillation");
        o.colorDetail = 101;
        check(!developDetail(flat.view(), o), "detail ranges validated");
    } else
        return 2;
    return failures ? 1 : 0;
}
