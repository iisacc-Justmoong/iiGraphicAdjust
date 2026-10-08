#include <algorithm>
#include <cmath>
#include <iiGraphicAdjust/LensFilter.h>
#include <iostream>
#include <limits>
using namespace iiGraphicAdjust;
namespace {
int failures = 0;
void check(bool ok, const char *message) {
    if (!ok) {
        std::cerr << message << '\n';
        ++failures;
    }
}
bool near(double a, double b, double tolerance = 2e-6) {
    return std::abs(a - b) <= tolerance;
}
Image pattern(std::size_t w = 17, std::size_t h = 13) {
    Image image{w, h, {}};
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x)
            image.pixels.push_back({float(x) / std::max<std::size_t>(1, w - 1),
                                    float(y) / std::max<std::size_t>(1, h - 1),
                                    float((x * 7 + y * 11) % 19) / 18,
                                    (x + y) % 11 == 0 ? 0.f : ((x + y) % 4 == 0 ? .4f : 1.f)});
    return image;
}
void contract(const Result<Image> &out, const Image &input) {
    check(bool(out), "lens processing failed");
    if (!out)
        return;
    check(out->width == input.width && out->height == input.height &&
              out->pixels.size() == input.pixels.size(),
          "lens shape changed");
    for (auto p : out->pixels)
        for (auto v : {p.r, p.g, p.b, p.a})
            check(std::isfinite(v) && v >= 0 && v <= 1, "lens invalid output");
}
} // namespace
int main() {
    auto input = pattern();
    auto original = input.pixels;
    auto identity = lensCorrection(input.view());
    check(identity && identity->pixels == original, "neutral lens exact copy");
    auto profile = genericLensProfile(24);
    check(profile && near(profile->k1, -.06) && near(profile->k2, .012) &&
              near(profile->vignette[0], -.38),
          "generic 24mm profile oracle");
    auto interpolated = genericLensProfile(std::sqrt(24. * 35));
    check(interpolated && near(interpolated->k1, (-.06 - .025) / 2) &&
              near(interpolated->vignette[1], (.08 + .05) / 2),
          "log profile interpolation oracle");
    check(genericLensProfile(1)->k1 == genericLensProfile(12)->k1 &&
              genericLensProfile(10000)->k1 == genericLensProfile(200)->k1,
          "profile endpoints clamp");
    LensCorrectionOptions o;
    o.profile = *profile;
    o.correctDistortion = o.correctVignette = o.correctCa = false;
    check(lensCorrection(input.view(), o)->pixels == original, "disabled profile exact identity");
    Image flat{21, 21, std::vector<Rgba>(441, {.2f, .3f, .4f, 1})};
    o = {};
    o.vignetteAmount = 50;
    o.edge = LensFilterEdge::Extension;
    auto vignette = lensCorrection(flat.view(), o);
    contract(vignette, flat);
    check(vignette && near(vignette->pixels[220].r, .2) && vignette->pixels[0].r > .2,
          "vignette center and corner");
    if (vignette) {
        double sr2 = std::pow(10 / std::hypot(21., 21.) * 2, 2) * 2,
               t = std::clamp((std::sqrt(sr2) - .45) / .55, 0., 1.);
        double gain = std::exp2(.75 * t * t * (3 - 2 * t));
        check(near(vignette->pixels[0].r, .2 * gain), "manual vignette independent formula");
    }
    o = {};
    o.profile = LensFilterProfile{0, 0, 0, 0, {-.2, .05, 0}, 0, 0};
    o.edge = LensFilterEdge::Extension;
    auto compensation = lensCorrection(flat.view(), o);
    const double r2 = 400. / 441.;
    check(compensation && near(compensation->pixels[0].r, .2 / (1 - .2 * r2 + .05 * r2 * r2)),
          "profile falloff compensation oracle");
    o = {};
    o.angle = 90;
    o.edge = LensFilterEdge::Extension;
    Image square{3,
                 3,
                 {{0, 0, 0, 1},
                  {.1, 0, 0, 1},
                  {.2, 0, 0, 1},
                  {.3, 0, 0, 1},
                  {.4, 0, 0, 1},
                  {.5, 0, 0, 1},
                  {.6, 0, 0, 1},
                  {.7, 0, 0, 1},
                  {.8, 0, 0, 1}}};
    auto rotated = lensCorrection(square.view(), o);
    check(rotated && near(rotated->pixels[0].r, .6) && near(rotated->pixels[8].r, .2),
          "lens inverse rotation oracle");
    for (auto edge :
         {LensFilterEdge::Extension, LensFilterEdge::Transparency, LensFilterEdge::Color}) {
        o = {};
        o.angle = 45;
        o.scale = 50;
        o.edge = edge;
        o.edgeColor = {.25, .5, .75, .3};
        auto output = lensCorrection(flat.view(), o);
        contract(output, flat);
        if (output) {
            if (edge == LensFilterEdge::Transparency)
                check(output->pixels[0].a == 0, "transparent outside");
            if (edge == LensFilterEdge::Color)
                check(output->pixels[0] == o.edgeColor, "colored outside");
            if (edge == LensFilterEdge::Extension)
                check(near(output->pixels[0].r, .2) && near(output->pixels[0].a, 1),
                      "edge extension");
        }
    }
    o = {};
    o.distortion = -75;
    o.horizontal = 25;
    o.vertical = -20;
    o.redCyan = 100;
    o.blueYellow = -100;
    o.angle = 10;
    auto warped = lensCorrection(input.view(), o);
    contract(warped, input);
    check(warped && warped->pixels != original, "combined lens controls change image");
    auto alternate = input;
    for (auto &p : alternate.pixels)
        if (p.a == 0)
            p = {1 - p.r, 1 - p.g, 1 - p.b, 0};
    auto other = lensCorrection(alternate.view(), o);
    if (warped && other)
        for (std::size_t i = 0; i < original.size(); ++i)
            if (warped->pixels[i].a > 0)
                check(warped->pixels[i] == other->pixels[i], "hidden RGB lens leakage");
    o = {};
    o.angle = 45;
    auto scale = autoLensScale(flat.view(), o);
    check(scale && *scale > 100 && *scale < 150, "auto scale rotated square");
    if (scale) {
        o.scale = *scale;
        auto covered = lensCorrection(flat.view(), o);
        contract(covered, flat);
        check(covered && covered->pixels[0].a > .99, "auto scale edge covered");
        check(autoLensScale(flat.view(), o) && near(*autoLensScale(flat.view(), o), *scale),
              "auto scale idempotence");
    }
    check(autoLensScale(input.view()) && *autoLensScale(input.view()) == 100, "neutral autoscale");
    for (auto dims : {std::pair{1ul, 1ul}, std::pair{1ul, 17ul}, std::pair{17ul, 1ul}}) {
        auto tiny = pattern(dims.first, dims.second);
        o = {};
        o.distortion = 100;
        o.profile = *profile;
        contract(lensCorrection(tiny.view(), o), tiny);
    }
    for (auto bad : {std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity(), 101.}) {
        o = {};
        o.distortion = bad;
        check(!lensCorrection(input.view(), o) && !autoLensScale(input.view(), o),
              "invalid lens scalar");
    }
    o = {};
    o.profile = LensFilterProfile{};
    o.profile->caRed = .051;
    check(!lensCorrection(input.view(), o), "invalid profile CA");
    o = {};
    o.edge = static_cast<LensFilterEdge>(99);
    check(!lensCorrection(input.view(), o), "invalid edge enum");
    o = {};
    o.edgeColor.a = -1;
    check(!lensCorrection(input.view(), o), "invalid inactive color");
    o = {};
    o.scale = 49;
    check(!autoLensScale(input.view(), o), "invalid scale");
    check(!genericLensProfile(0) && !genericLensProfile(std::numeric_limits<double>::quiet_NaN()),
          "invalid focal length");
    check(!lensCorrection(ImageView{}) && !autoLensScale(ImageView{}), "invalid image");
    check(input.pixels == original, "lens mutated source");
    if (!failures)
        std::cout << "Lens profile, geometry, alpha, boundaries and numeric oracles passed\n";
    return failures ? 1 : 0;
}
