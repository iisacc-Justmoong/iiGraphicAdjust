#include <cmath>
#include <iiGraphicAdjust/Develop.h>
#include <iiGraphicAdjust/Masks.h>
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
    Image image{32, 16, std::vector<Rgba>(512, {.5F, .5F, .5F, .6F})};
    if (name == "shapes") {
        auto linear = linearMask(100, 50);
        check(linear && linear->coverage[0] > .99 && linear->coverage[99] < .01 &&
                  std::abs(linear->coverage[50] - .5) < .02,
              "linear normalized gradient");
        auto radial = radialMask(32, 16);
        check(radial && radial->coverage[8 * 32 + 16] > .99 && radial->coverage[0] == 0,
              "radial center and corners");
        auto inverted = radialMask(32, 16, {{.5, .5}, .25, .25, 0, 50, true});
        check(inverted && inverted->coverage[0] == 1, "radial invert");
        BrushStroke stroke;
        stroke.points = {{.25, .5}, {.75, .5}};
        stroke.radius = .1;
        stroke.feather = 0;
        std::array<BrushStroke, 1> strokes{stroke};
        auto brush = brushMask(image.view(), strokes);
        check(brush && brush->coverage[8 * 32 + 16] == 1 && brush->coverage[0] == 0,
              "brush interpolation covers the stroke center");
        auto erase = stroke;
        erase.points = {{.5, .5}};
        erase.erase = true;
        std::array<BrushStroke, 2> both{stroke, erase};
        auto eraser = brushMask(image.view(), both);
        check(eraser && eraser->coverage[8 * 32 + 16] == 0, "erase brush subtracts paint");
        auto luminosity = luminanceRangeMask(image.view(), {.6, .8, .1, .1, 0});
        check(luminosity && luminosity->coverage[0] > .9, "luminance EV range selects mid-gray");
        auto range = colorRangeMask(image.view(), {{{.5F, .5F, .5F, 1}}, 0, 0});
        check(range && range->coverage[0] == 1, "color range selects its sample");
        check(!linearMask(1, 1, {{.5, .5}, {.5, .5}}) && !radialMask(1, 1, {{.5, .5}, 0, .2}),
              "degenerate mask geometry rejected");
        check(!linearMask(1, 1, {{0, 0}, {std::numeric_limits<double>::denorm_min(), 0}}),
              "subnormal gradient endpoints rejected before squared length underflow");
    } else if (name == "combine") {
        const Mask a{2, 1, {.2F, .8F}}, b{2, 1, {.5F, .5F}};
        auto add = combineMasks(a.view(), b.view(), MaskCombine::Add),
             sub = combineMasks(a.view(), b.view(), MaskCombine::Subtract),
             inter = combineMasks(a.view(), b.view(), MaskCombine::Intersect),
             inv = invertMask(a.view());
        check(add && add->coverage == std::vector<float>{.5F, .8F}, "Add=max");
        check(sub && std::abs(sub->coverage[0] - .1) < 1e-6 && inter &&
                  std::abs(inter->coverage[1] - .4) < 1e-6,
              "Subtract=a(1-b), Intersect=ab");
        check(inv && std::abs(inv->coverage[0] - .8) < 1e-6, "invert mask");
        Mask all{32, 16, std::vector<float>(512, 1)};
        auto refined = refineMask(image.view(), all.view(), 100);
        check(refined && refined->coverage[0] == 1 && refined->coverage.back() == 1,
              "guided refine preserves constant selection");
        auto invalid = a;
        invalid.coverage[0] = std::numeric_limits<float>::quiet_NaN();
        check(!invertMask(invalid.view()), "nonfinite coverage rejected");
        check(!combineMasks(a.view(), all.view(), MaskCombine::Add), "mask dimension mismatch");
    } else if (name == "local") {
        Mask mask{32, 16, std::vector<float>(512, 0)};
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                mask.coverage[y * 32 + x] = 1;
        LocalAdjustment local{mask.view(), {}};
        local.options.exposure = 1;
        std::array<LocalAdjustment, 1> adjustments{local};
        auto applied = applyLocalAdjustments(image.view(), adjustments);
        check(applied && std::abs(applied->pixels[0].r - .685836F) < 2e-5 &&
                  applied->pixels[20] == image.pixels[20],
              "local exposure radiometric oracle and outside identity");
        check(applied && applied->pixels[0].a == .6F, "local alpha unchanged");
        Image knee{1, 1, {{.04045F, .123F, .6F, 1}}};
        Mask zero{1, 1, {0}};
        std::array<LocalAdjustment, 1> noop{{{zero.view(), local.options}}};
        auto exact = applyLocalAdjustments(knee.view(), noop);
        check(exact && exact->pixels == knee.pixels,
              "zero coverage exact identity at the sRGB transfer knee");
        adjustments[0].options.amount = 200;
        auto twice = applyLocalAdjustments(image.view(), adjustments);
        std::array<LocalAdjustment, 2> overlap{local, local};
        auto sum = applyLocalAdjustments(image.view(), overlap);
        check(twice && sum && twice->pixels == sum->pixels,
              "overlapping masks accumulate slider contributions");
        image.pixels[0] = {.2F, .4F, .6F, 0};
        adjustments[0].options = {};
        adjustments[0].options.texture = 80;
        auto hidden = applyLocalAdjustments(image.view(), adjustments);
        check(hidden && hidden->pixels[0] == image.pixels[0], "local effects preserve hidden RGB");
        adjustments[0].options.hue = 101;
        check(!applyLocalAdjustments(image.view(), adjustments), "invalid local slider rejected");
    } else if (name == "pipeline") {
        auto neutral = develop(image.view());
        check(neutral && neutral->pixels == image.pixels &&
                  neutral->pixels.data() != image.pixels.data(),
              "neutral composition detached exact copy");
        DevelopOptions o;
        o.light.exposure = .5;
        o.color.grading.global = {40, 10, 0};
        o.grain = {10, 25, 50, 7};
        auto result = develop(image.view(), o);
        check(result && result->pixels != image.pixels && result->pixels[0].a == .6F,
              "composed development changes colors preserving alpha");
        o.grain.amount = 101;
        check(!develop(image.view(), o), "late-stage validation propagated");
    } else
        return 2;
    return failures ? 1 : 0;
}
