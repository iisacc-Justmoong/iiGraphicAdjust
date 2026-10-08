#include <algorithm>
#include <cmath>
#include <iiGraphicAdjust/Optics.h>
#include <iostream>
using namespace iiGraphicAdjust;
int main() {
    int failures = 0;
    auto check = [&](bool ok, const char *why) {
        if (!ok) {
            std::cerr << why << '\n';
            ++failures;
        }
    };
    Image image{64, 64, {}};
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            const auto v = (x / 8 + y / 8) % 2 ? .8F : .2F;
            image.pixels.push_back({v, v, v, 1});
        }
    auto n = correctOptics(image.view());
    check(n && n->pixels == image.pixels, "neutral optics exact copy");
    OpticsOptions o;
    o.distortion = 80;
    auto barrel = correctOptics(image.view(), o);
    check(barrel && barrel->pixels != image.pixels && barrel->width == 64,
          "radial warp changes sampling");
    o.distortion = -100;
    auto border = correctOptics(image.view(), o);
    check(border && border->pixels[0] == Rgba{0, 0, 0, 0},
          "out-of-input warp is transparent black");
    auto autoCa = estimateChromaticAberration(image.view());
    check(autoCa && std::abs((*autoCa)[0]) < 1e-5 && std::abs((*autoCa)[1]) < 1e-5,
          "achromatic edges have no CA");
    o = {};
    o.redChromaticAberration = 100;
    auto ca = correctOptics(image.view(), o);
    check(ca && ca->pixels != image.pixels, "manual CA moves red independently");
    Image gray{600, 400, std::vector<Rgba>(240000, {.5F, .5F, .5F, 1})};
    auto edges = correctOptics(gray.view(), o);
    check(edges && edges->pixels == gray.pixels,
          "CA alone must not create colored boundary strips on constant gray");
    o.vignetting = 50;
    auto gain = correctOptics(gray.view(), o);
    check(gain && gain->pixels[100 * 600 + 100].r == gain->pixels[100 * 600 + 100].g,
          "lens gain is shared across RGB after CA");
    o = {};
    o.vignetting = 80;
    Image flat{32, 32, std::vector<Rgba>(1024, {.3F, .3F, .3F, .4F})};
    auto lens = correctOptics(flat.view(), o);
    check(lens && lens->pixels[0].r > lens->pixels[16 * 32 + 16].r && lens->pixels[0].a == .4F,
          "lens vignette compensation preserves alpha");
    o = {};
    o.embeddedWarp = EmbeddedLensWarp{};
    auto identity = correctOptics(image.view(), o);
    check(identity && identity->pixels == image.pixels, "identity embedded warp");
    o.embeddedVignette = EmbeddedLensVignette{};
    o.embeddedVignette->coefficients[0] = .5;
    auto embedded = correctOptics(flat.view(), o);
    check(embedded && embedded->pixels[0].r > .3F, "supplied radial lens coefficients applied");
    Image fringe{9, 3, std::vector<Rgba>(27, {.05F, .05F, .05F, 1})};
    for (int y = 0; y < 3; ++y) {
        fringe.pixels[y * 9 + 4] = {.8F, .1F, .9F, 1};
        for (int x = 5; x < 9; ++x)
            fringe.pixels[y * 9 + x] = {.95F, .95F, .95F, 1};
    }
    o = {};
    o.purpleDefringe = 20;
    o.purpleHueLow = 0;
    o.purpleHueHigh = 100;
    auto df = correctOptics(fringe.view(), o);
    const auto chroma = [](Rgba p) {
        return std::max({p.r, p.g, p.b}) - std::min({p.r, p.g, p.b});
    };
    check(df && chroma(df->pixels[4]) < chroma(fringe.pixels[4]),
          "purple fringe desaturated near high contrast");
    o.purpleHueLow = 80;
    o.purpleHueHigh = 20;
    check(!correctOptics(image.view(), o), "unordered fringe hues rejected");
    o = {};
    o.embeddedWarp = EmbeddedLensWarp{};
    o.embeddedWarp->radius = 0;
    check(!correctOptics(image.view(), o), "zero embedded lens radius rejected");
    return failures ? 1 : 0;
}
