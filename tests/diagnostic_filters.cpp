#include <algorithm>
#include <cmath>
#include <functional>
#include <iiGraphicAdjust/DiagnosticFilters.h>
#include <iostream>
#include <limits>
#include <string>

using namespace iiGraphicAdjust;
namespace {
int failures = 0;
void check(bool condition, const char *message) {
    if (!condition) {
        ++failures;
        std::cerr << message << '\n';
    }
}
bool near(double a, double b, double tolerance = 1e-6) {
    return std::abs(a - b) <= tolerance;
}
Image image(std::size_t w, std::size_t h, Rgba p) {
    return {w, h, std::vector<Rgba>(w * h, p)};
}
void bounded(const Result<Image> &result, ImageView original) {
    check(bool(result), "valid diagnostic request succeeds");
    if (!result)
        return;
    check(result->width == original.width && result->height == original.height,
          "diagnostic dimensions unchanged");
    for (std::size_t i = 0; i < original.pixels.size(); ++i) {
        auto p = result->pixels[i];
        check(std::isfinite(p.r) && std::isfinite(p.g) && std::isfinite(p.b) && p.r >= 0 &&
                  p.r <= 1 && p.g >= 0 && p.g <= 1 && p.b >= 0 && p.b <= 1,
              "diagnostic output finite normalized RGB");
        check(p.a == original.pixels[i].a, "diagnostic preserves input alpha");
    }
}
} // namespace
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    const std::string mode = argv[1];
    if (mode == "point") {
        Image in{3, 1, {{.8F, .3F, .1F, 1}, {.1F, .2F, .8F, .6F}, {.4F, .4F, .4F, 1}}};
        auto sample = samplePointColor(in.pixels[0]);
        check(bool(sample), "visible Point Color sample available");
        if (!sample)
            return 1;
        auto selected = pointColorRangePreview(in.view(), *sample);
        bounded(selected, in.view());
        double grey = .2126 * double(in.pixels[1].r) + .7152 * double(in.pixels[1].g) +
                      .0722 * double(in.pixels[1].b);
        check(selected && selected->pixels[0] == in.pixels[0] &&
                  near(selected->pixels[1].r, grey) && near(selected->pixels[1].g, grey) &&
                  near(selected->pixels[1].b, grey),
              "sampled orange retains exact color; distant blue becomes encoded-luminance grey");
        auto shifted = *sample;
        shifted.hueShift = 100;
        shifted.saturationShift = -100;
        shifted.luminanceShift = 80;
        shifted.variance = -80;
        auto noEdit = pointColorRangePreview(in.view(), shifted);
        check(noEdit && selected && noEdit->pixels == selected->pixels,
              "Point Color preview selects the range without executing its own adjustments");
        auto partial = *sample;
        partial.lightness += .75 * (.05 + .4 * partial.luminanceRange / 100);
        auto midpoint = pointColorRangePreview(in.view(), partial);
        grey = .2126 * double(in.pixels[0].r) + .7152 * double(in.pixels[0].g) +
               .0722 * double(in.pixels[0].b);
        check(midpoint && near(midpoint->pixels[0].r, (grey + in.pixels[0].r) / 2) &&
                  near(midpoint->pixels[0].g, (grey + in.pixels[0].g) / 2) &&
                  near(midpoint->pixels[0].b, (grey + in.pixels[0].b) / 2),
              "75% lightness half-width independently yields smoothstep membership .5");
        auto narrow = partial;
        narrow.range = 25;
        auto wide = partial;
        wide.range = 100;
        auto n = pointColorRangePreview(in.view(), narrow),
             w = pointColorRangePreview(in.view(), wide);
        check(n && w && near(n->pixels[0].r, grey) && w->pixels[0] == in.pixels[0],
              "overall range scales the three selection widths");
        auto zero = *sample, full = *sample;
        zero.hue = 0;
        full.hue = 360;
        auto a = pointColorRangePreview(in.view(), zero),
             b = pointColorRangePreview(in.view(), full);
        check(a && b && a->pixels == b->pixels, "Point Color hue wraps across a full turn");
        auto neutral = image(1, 1, {.4F, .42F, .4F, .5F});
        auto neutralSample = samplePointColor(neutral.pixels[0]);
        if (neutralSample) {
            neutralSample->chroma = .01;
            neutralSample->hue = 0;
            auto left = pointColorRangePreview(neutral.view(), *neutralSample);
            neutralSample->hue = 180;
            auto right = pointColorRangePreview(neutral.view(), *neutralSample);
            check(left && right && left->pixels == right->pixels,
                  "near-neutral sample ignores hue and selects chroma/lightness only");
        }
    } else if (mode == "masks") {
        auto in = image(3, 1, {.2F, .4F, .8F, .3F});
        Mask m{3, 1, {0, .5F, 1}};
        MaskOverlayOptions o{{1, 0, 0}, 40};
        auto color = maskColorOverlay(in.view(), m.view(), o);
        auto bw = maskColorOnBwOverlay(in.view(), m.view(), o);
        auto black = maskImageOnBlack(in.view(), m.view());
        auto white = maskImageOnWhite(in.view(), m.view());
        auto coverage = maskWhiteOnBlack(in.view(), m.view());
        for (auto *r : {&color, &bw, &black, &white, &coverage})
            bounded(*r, in.view());
        double grey = .2126 * double(.2F) + .7152 * double(.4F) + .0722 * double(.8F);
        check(color && color->pixels[0] == in.pixels[0] && near(color->pixels[1].r, .36) &&
                  near(color->pixels[1].g, .32) && near(color->pixels[1].b, .64),
              "color tint independently combines mask .5 and opacity .4 into blend .2");
        check(bw && near(bw->pixels[0].r, grey) && near(bw->pixels[0].b, grey) &&
                  near(bw->pixels[1].r, grey + (1 - grey) * .2) && near(bw->pixels[1].g, grey * .8),
              "ColorOnBw uses encoded luminance as the whole-image base");
        check(black && black->pixels[0].r == 0 && near(black->pixels[1].r, .1) &&
                  near(black->pixels[1].b, .4) && black->pixels[2] == in.pixels[2],
              "ImageOnBlack multiplies encoded RGB by evaluated mask");
        check(white && white->pixels[0].r == 1 && near(white->pixels[1].r, .6) &&
                  near(white->pixels[1].b, .9) && white->pixels[2] == in.pixels[2],
              "ImageOnWhite composites onto a white background");
        check(coverage && coverage->pixels[0].r == 0 && coverage->pixels[1].r == .5 &&
                  coverage->pixels[2].r == 1,
              "WhiteOnBlack renders evaluated mask coverage directly");
        o.opacity = 0;
        auto noTint = maskColorOverlay(in.view(), m.view(), o);
        auto greyBase = maskColorOnBwOverlay(in.view(), m.view(), o);
        check(noTint && noTint->pixels == in.pixels && greyBase &&
                  std::all_of(greyBase->pixels.begin(), greyBase->pixels.end(),
                              [&](auto p) { return near(p.r, grey) && p.r == p.g && p.g == p.b; }),
              "zero tint opacity is identity for Color and a grey base for ColorOnBw");
    } else if (mode == "gamut") {
        auto in = image(5, 1, {.3F, .4F, .5F, .6F});
        const std::vector<std::array<double, 3>> scenes{
            {1, 0, 0}, {0, 1, 0}, {.2, .2, .2}, {2, 2, 2}, {-.2, -.2, -.2}};
        GamutWarningOptions o;
        o.linearRec2020 = scenes;
        auto dest = destinationGamutWarning(in.view(), o);
        bounded(dest, in.view());
        check(dest && dest->pixels[0] == Rgba{1, 0, 0, .6F} &&
                  dest->pixels[1] == Rgba{1, 0, 0, .6F} && dest->pixels[2] == in.pixels[2] &&
                  dest->pixels[3] == in.pixels[3] && dest->pixels[4] == in.pixels[4],
              "sRGB destination flags Rec2020 primaries but excludes neutral highlight/shadow "
              "clipping");
        o.proofSpace = DiagnosticColorSpace::Rec2020;
        auto wide = destinationGamutWarning(in.view(), o);
        auto display = displayGamutWarning(in.view(), o);
        check(
            wide && wide->pixels == in.pixels && display &&
                display->pixels[0] == Rgba{0, .25F, 1, .6F} &&
                display->pixels[1] == Rgba{0, .25F, 1, .6F},
            "Rec2020 proof holds its own primaries while sRGB display paints blue gamut warnings");
        const std::vector<std::array<double, 3>> neutral(5, {.4, .4, .4});
        o.linearRec2020 = neutral;
        for (int k = 0; k < 5; ++k) {
            o.proofSpace = DiagnosticColorSpace(k);
            auto a = destinationGamutWarning(in.view(), o), b = displayGamutWarning(in.view(), o);
            check(a && b && a->pixels == in.pixels && b->pixels == in.pixels,
                  "neutral Rec2020 maps to neutral for all five spaces, including Bradford D50 "
                  "ProPhoto");
        }
        auto ordinary = image(5, 1, {.9F, .1F, .6F, .5F});
        o.linearRec2020 = {};
        for (int k = 0; k < 5; ++k) {
            o.proofSpace = DiagnosticColorSpace(k);
            auto a = destinationGamutWarning(ordinary.view(), o),
                 b = displayGamutWarning(ordinary.view(), o);
            check(a && b && a->pixels == ordinary.pixels && b->pixels == ordinary.pixels,
                  "ordinary sRGB fallback does not pretend to recover already-lost wide gamut");
        }
        auto srgbScene = [](double red) {
            // Independent published D65 linear-sRGB -> Rec2020 transform.
            return std::array<double, 3>{.627403895934699 * red + .0372596104065301,
                                         .069097289358232 * red + .0930902710641768,
                                         .016391438875150 * red + .0983608561124850};
        };
        const std::vector<std::array<double, 3>> boundary{
            srgbScene(1.001), srgbScene(1.1), {.2, .2, .2}, {.2, .2, .2}, {.2, .2, .2}};
        o.proofSpace = DiagnosticColorSpace::Srgb;
        o.linearRec2020 = boundary;
        auto tolerance = destinationGamutWarning(in.view(), o);
        check(
            tolerance && tolerance->pixels[0] == in.pixels[0] &&
                tolerance->pixels[1] == Rgba{1, 0, 0, .6F},
            "reference .995 chroma-scale tolerance suppresses roundoff but flags real gamut loss");
    } else if (mode == "validation") {
        auto in = image(48, 32, {.4F, .5F, .6F, 1});
        Mask mask{48, 32, std::vector<float>(1536, .5F)};
        double nan = std::numeric_limits<double>::quiet_NaN();
        PointColorOptions point;
        point.hueShift = nan;
        check(!pointColorRangePreview(in.view(), point),
              "inactive Point Color adjustment NaN rejected");
        point = {};
        point.range = 101;
        check(!pointColorRangePreview(in.view(), point), "Point Color range bounded");
        MaskOverlayOptions tint;
        tint.opacity = 0;
        tint.color[0] = nan;
        check(!maskColorOverlay(in.view(), mask.view(), tint) &&
                  !maskColorOnBwOverlay(in.view(), mask.view(), tint),
              "inactive overlay color NaN rejected");
        check(!maskImageOnBlack(in.view(), {1, 1, {mask.coverage.data(), 1}}),
              "diagnostic mask dimensions must match photo");
        check(!maskImageOnWhite(in.view(), {48, 32, {}}), "mask buffer length validated");
        auto invalidMask = mask;
        invalidMask.coverage[0] = float(nan);
        check(!maskWhiteOnBlack(in.view(), invalidMask.view()), "mask NaN coverage rejected");
        GamutWarningOptions gamut;
        gamut.proofSpace = DiagnosticColorSpace(99);
        check(!destinationGamutWarning(in.view(), gamut), "diagnostic gamut space enum validated");
        std::vector<std::array<double, 3>> raw(1, {0, 0, 0});
        gamut = {};
        gamut.linearRec2020 = raw;
        check(!displayGamutWarning(in.view(), gamut), "pre-gamut pixel count validated");
        raw.assign(1536, {nan, 0, 0});
        gamut.linearRec2020 = raw;
        check(!destinationGamutWarning(in.view(), gamut), "pre-gamut finite values validated");
        raw.assign(1536, {65, 0, 0});
        gamut.linearRec2020 = raw;
        check(!displayGamutWarning(in.view(), gamut), "pre-gamut channel magnitude bounded");
        std::vector<std::function<Result<Image>(ImageView)>> functions = {
            [](ImageView i) { return pointColorRangePreview(i); },
            [&](ImageView i) { return maskColorOverlay(i, mask.view()); },
            [&](ImageView i) { return maskColorOnBwOverlay(i, mask.view()); },
            [&](ImageView i) { return maskImageOnBlack(i, mask.view()); },
            [&](ImageView i) { return maskImageOnWhite(i, mask.view()); },
            [&](ImageView i) { return maskWhiteOnBlack(i, mask.view()); },
            [](ImageView i) { return destinationGamutWarning(i); },
            [](ImageView i) { return displayGamutWarning(i); }};
        auto altered = in;
        for (std::size_t i = 0; i < in.pixels.size(); i += 4) {
            in.pixels[i] = {1, 0, 1, 0};
            altered.pixels[i] = {0, 1, 0, 0};
        }
        for (auto fn : functions) {
            auto a = fn(in.view()), b = fn(altered.view());
            bounded(a, in.view());
            bounded(b, altered.view());
            if (a && b)
                for (std::size_t i = 0; i < in.pixels.size(); ++i)
                    if (in.pixels[i].a > 0)
                        check(a->pixels[i] == b->pixels[i],
                              "diagnostics never propagate hidden RGB");
            check(!fn({0, 1, {}}), "diagnostic invalid dimensions rejected");
            auto invalid = in;
            invalid.pixels[0].r = float(nan);
            check(!fn(invalid.view()), "diagnostic invalid image pixel rejected");
        }
        for (auto extent : {std::array<std::size_t, 2>{1, 1}, {1, 8}, {8, 1}}) {
            auto thin = image(extent[0], extent[1], {.2F, .5F, .7F, .5F});
            Mask m{thin.width, thin.height, std::vector<float>(thin.pixels.size(), .5F)};
            bounded(pointColorRangePreview(thin.view()), thin.view());
            bounded(maskColorOverlay(thin.view(), m.view()), thin.view());
            bounded(maskColorOnBwOverlay(thin.view(), m.view()), thin.view());
            bounded(maskImageOnBlack(thin.view(), m.view()), thin.view());
            bounded(maskImageOnWhite(thin.view(), m.view()), thin.view());
            bounded(maskWhiteOnBlack(thin.view(), m.view()), thin.view());
            bounded(destinationGamutWarning(thin.view()), thin.view());
            bounded(displayGamutWarning(thin.view()), thin.view());
        }
    } else
        return 2;
    return failures ? 1 : 0;
}
