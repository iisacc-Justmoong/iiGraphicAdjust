#include <algorithm>
#include <cmath>
#include <iiGraphicAdjust/BlurFilters.h>
#include <iiGraphicAdjust/DistortionFilters.h>
#include <iiGraphicAdjust/UtilityFilters.h>
#include <iostream>
#include <limits>
#include <string_view>
using namespace iiGraphicAdjust;
namespace {
int failures = 0;
void check(bool ok, const char *reason) {
    if (!ok) {
        std::cerr << reason << '\n';
        ++failures;
    }
}
bool near(double a, double b, double eps = 2e-6) {
    return std::abs(a - b) <= eps;
}
Image fixture(std::size_t w = 17, std::size_t h = 13) {
    Image i{w, h, {}};
    std::uint32_t seed = 11;
    for (std::size_t k = 0; k < w * h; ++k) {
        seed = seed * 1664525U + 1013904223U;
        i.pixels.push_back({float(seed & 255) / 255, float((seed >> 8) & 255) / 255,
                            float((seed >> 16) & 255) / 255,
                            k % 7 ? float((seed >> 24) | 1) / 255 : 0.F});
    }
    return i;
}
void contract(const Result<Image> &r, ImageView source, bool keepAlpha = false) {
    check(bool(r), "valid filter succeeds");
    if (!r)
        return;
    check(r->width == source.width && r->height == source.height &&
              r->pixels.size() == source.pixels.size(),
          "filter dimensions preserved");
    for (std::size_t k = 0; k < r->pixels.size(); ++k) {
        auto p = r->pixels[k];
        for (auto c : {p.r, p.g, p.b, p.a})
            check(std::isfinite(c) && c >= 0 && c <= 1, "filter output finite normalized");
        if (keepAlpha) {
            check(p.a == source.pixels[k].a, "filter preserves alpha");
            if (p.a == 0)
                check(p == source.pixels[k], "hidden color retained");
        }
    }
}
void blurCases() {
    auto image = fixture();
    auto view = image.view();
    for (auto r :
         {filterGaussianBlur(view), boxBlur(view), blur(view), blurMore(view), motionBlur(view),
          radialBlur(view), surfaceBlur(view), unsharpMask(view), filterSharpen(view),
          sharpenMore(view), sharpenEdges(view), smartSharpen(view), highPass(view),
          despeckle(view), smartBlur(view), shapeBlur(view), lensBlur(view), tiltShift(view)})
        contract(r, view);
    std::array iris{IrisBlurPin{}};
    std::array field{FieldBlurPin{}};
    std::array spin{SpinBlurPin{}};
    std::array paths{BlurPath{}};
    contract(irisBlur(view, iris), view);
    contract(fieldBlur(view, field), view);
    contract(spinBlur(view, spin), view);
    contract(pathBlur(view, paths), view);
    Image line{3, 1, {{0, 0, 0, 1}, {1, 1, 1, 1}, {0, 0, 0, 1}}};
    auto b = boxBlur(line.view(), 1);
    check(b && near(b->pixels[0].r, 1. / 3) && near(b->pixels[1].r, 1. / 3) &&
              near(b->pixels[2].r, 1. / 3),
          "box blur exact 3-sample oracle");
    Image edge{2, 1, {{1, 0, 0, 1}, {0, 0, 1, 0}}};
    auto transparent = boxBlur(edge.view(), 1);
    check(transparent && near(transparent->pixels[0].r, 1) && transparent->pixels[0].b == 0 &&
              near(transparent->pixels[0].a, 2. / 3),
          "encoded premultiplication hides transparent blue");
    for (auto method : {RadialBlurMethod::Spin, RadialBlurMethod::Zoom})
        contract(radialBlur(view, {20, method}), view);
    for (unsigned s = 0; s <= 8; ++s)
        contract(shapeBlur(view, {3, BlurShape(s)}), view);
    for (unsigned q = 0; q <= 2; ++q)
        for (unsigned m = 0; m <= 2; ++m)
            contract(smartBlur(view, {3, 25, FilterBlurQuality(q), SmartBlurMode(m)}), view, true);
    LensBlurOptions lens;
    lens.depth = LensDepthSource::Map;
    lens.depthMap = view;
    lens.brightness = 50;
    lens.noise = 5;
    contract(lensBlur(view, lens), view);
    auto one = lensBlur(view, lens), two = lensBlur(view, lens);
    check(one && two && one->pixels == two->pixels, "lens noise reproducible");
    check(filterGaussianBlur(view, 0)->pixels == image.pixels, "neutral blur exact copy");
    check(unsharpMask(view, {0, 1, 0})->pixels == image.pixels, "neutral unsharp exact copy");
    auto high = highPass(Image{1, 1, {{.2F, .6F, .8F, .3F}}}.view(), 5);
    check(high && near(high->pixels[0].r, .5) && near(high->pixels[0].g, .5) &&
              near(high->pixels[0].b, .5),
          "highpass flat image yields mid-gray");
    auto flat = Image{7, 3, std::vector<Rgba>(21, {.3F, .4F, .5F, .75F})};
    for (auto r : {filterGaussianBlur(flat.view(), 1000), boxBlur(flat.view(), 2000),
                   shapeBlur(flat.view(), {20, BlurShape::Square})}) {
        contract(r, flat.view());
        if (r)
            for (auto p : r->pixels)
                check(near(p.r, .3) && near(p.a, .75), "large window preserves constants");
    }
    const auto tinySigma = 1e-200;
    auto microscopic = filterGaussianBlur(line.view(), tinySigma);
    contract(microscopic, line.view());
    check(microscopic && microscopic->pixels == line.pixels,
          "subnormal Gaussian variance has an exact finite center tap");
    for (auto result :
         {unsharpMask(line.view(), {50, tinySigma, 0}),
          smartSharpen(line.view(), {100, tinySigma, 0}), highPass(line.view(), tinySigma)})
        contract(result, line.view(), true);
    const auto tinyAlpha = std::numeric_limits<float>::denorm_min();
    Image lowCoverage{2, 1, {{.4F, .25F, .5F, tinyAlpha}, {1, 0, 1, 0}}};
    auto transported = boxBlur(lowCoverage.view(), 1);
    check(transported && transported->pixels[1] == Rgba{0, 0, 0, 0},
          "transported alpha underflow produces canonical transparent black");
    Image isolated{7, 1, std::vector<Rgba>(7, {1, 0, 1, 0})};
    isolated.pixels[3] = lowCoverage.pixels[0];
    for (auto result :
         {unsharpMask(isolated.view(), {100, 2, 0}), smartSharpen(isolated.view(), {100, 2, 0}),
          reduceNoise(isolated.view(), {0, 0, 0, 100, false})}) {
        contract(result, isolated.view(), true);
        check(result && near(result->pixels[3].r, isolated.pixels[3].r) &&
                  near(result->pixels[3].g, isolated.pixels[3].g) &&
                  near(result->pixels[3].b, isolated.pixels[3].b),
              "source-alpha detail retains color below transported-alpha precision");
    }
    auto tinyHigh = highPass(isolated.view(), 2);
    check(tinyHigh && near(tinyHigh->pixels[3].r, .5) && near(tinyHigh->pixels[3].g, .5) &&
              near(tinyHigh->pixels[3].b, .5) && tinyHigh->pixels[3].a == tinyAlpha,
          "source-alpha high pass uses unbounded weighted color ratio");
    LensBlurOptions neutralLens;
    neutralLens.radius = 0;
    check(lensBlur(image.view(), neutralLens)->pixels == image.pixels,
          "neutral lens blur preserves hidden colors exactly");
    Image mapBlack{image.width, image.height, std::vector<Rgba>(image.pixels.size(), {0, 0, 0, 0})};
    Image mapWhite{image.width, image.height, std::vector<Rgba>(image.pixels.size(), {1, 1, 1, 0})};
    LensBlurOptions mappedLens;
    mappedLens.radius = 6;
    mappedLens.depth = LensDepthSource::Map;
    mappedLens.focalDistance = 64;
    for (bool invert : {false, true}) {
        mappedLens.invertDepth = invert;
        mappedLens.depthMap = mapBlack.view();
        auto a = lensBlur(image.view(), mappedLens);
        mappedLens.depthMap = mapWhite.view();
        auto b = lensBlur(image.view(), mappedLens);
        check(a && b && a->pixels == b->pixels,
              "invisible depth-map RGB cannot alter defocus, including inverted depth");
        check(a && a->pixels == image.pixels,
              "uncovered depth map stays on focal plane with exact in-focus pixels");
    }
    mappedLens.brightness = 50;
    mappedLens.threshold = 0;
    mappedLens.noise = 10;
    mappedLens.depthMap = mapBlack.view();
    auto focusedEffects = lensBlur(image.view(), mappedLens);
    mappedLens.depthMap = mapWhite.view();
    auto alternateEffects = lensBlur(image.view(), mappedLens);
    check(focusedEffects && alternateEffects && focusedEffects->pixels == alternateEffects->pixels,
          "focused highlight and noise effects ignore invisible depth-map color");
    check(focusedEffects && focusedEffects->pixels != image.pixels,
          "uncovered depth map retains requested highlight and noise processing");
    mappedLens.brightness = 0;
    mappedLens.noise = 0;
    for (auto &p : mapBlack.pixels)
        p.a = 1;
    mappedLens.invertDepth = false;
    mappedLens.depthMap = mapBlack.view();
    auto coveredDepth = lensBlur(image.view(), mappedLens);
    check(coveredDepth && coveredDepth->pixels != image.pixels,
          "covered depth-map value still controls defocus");
}
void distortionCases() {
    auto image = fixture();
    auto view = image.view();
    for (auto r : {offset(view, 3, -2), twirl(view), pinch(view), spherize(view), wave(view),
                   ripple(view), polarCoordinates(view), displace(view, view),
                   shear(view, std::array{ShearPoint{0, 0}, ShearPoint{1, .2}}), zigZag(view)})
        contract(r, view);
    Image line{3, 1, {{0, 0, 0, 1}, {.5F, .5F, .5F, 1}, {1, 1, 1, 1}}};
    auto o = offset(line.view(), 1, 0);
    check(o && o->pixels == std::vector<Rgba>({line.pixels[2], line.pixels[0], line.pixels[1]}),
          "wrapped offset matches independent permutation");
    check(twirl(view, 0)->pixels == image.pixels && pinch(view, 0)->pixels == image.pixels &&
              spherize(view, {0})->pixels == image.pixels &&
              zigZag(view, {0})->pixels == image.pixels,
          "neutral warps exact copy");
    auto neutral = Image{2, 2, std::vector<Rgba>(4, {.5F, .5F, .5F, 1})};
    auto d = displace(view, neutral.view());
    check(d && d->pixels == image.pixels, "mid-gray displacement map neutral");
    for (unsigned mode = 0; mode < 3; ++mode)
        contract(spherize(view, {-100, SpherizeMode(mode)}), view);
    for (unsigned type = 0; type < 3; ++type) {
        WaveOptions w;
        w.type = WaveType(type);
        auto a = wave(view, w), b = wave(view, w);
        contract(a, view);
        check(a && b && a->pixels == b->pixels, "wave deterministic across all types");
    }
    for (unsigned size = 0; size < 3; ++size)
        contract(ripple(view, {40, RippleSize(size)}), view);
    contract(polarCoordinates(view, PolarMode::PolarToRectangular), view);
    for (unsigned style = 0; style < 3; ++style)
        contract(zigZag(view, {-50, 4, ZigZagStyle(style)}), view);
    contract(offset(view, std::numeric_limits<std::int32_t>::min(),
                    std::numeric_limits<std::int32_t>::max()),
             view);
}
void utilityCases() {
    auto image = fixture();
    auto view = image.view();
    for (auto r : {average(view), filterAddNoise(view), median(view), dustAndScratches(view),
                   minimum(view), maximum(view), reduceNoise(view), customFilter(view),
                   hsbHsl(view, FilterColorModel::Rgb, FilterColorModel::Hsb), deInterlace(view),
                   ntscColors(view), guidedBlur(view), crossGuidedBlur(view, view)})
        contract(r, view);
    check(customFilter(view)->pixels == image.pixels, "identity custom kernel exact copy");
    Image line{3, 1, {{0, 0, 0, 1}, {1, 1, 1, 1}, {0, 0, 0, 1}}};
    auto m = median(line.view());
    check(m && m->pixels[1].r == 0, "median removes isolated impulse");
    auto dust = dustAndScratches(line.view(), {1, 255});
    check(dust && dust->pixels == line.pixels, "dust threshold protects small differences");
    auto min = minimum(line.view()), max = maximum(line.view());
    check(min && min->pixels[1].r == 0 && max && max->pixels[0].r == 1 && max->pixels[2].r == 1,
          "morphology interval oracle");
    Image coverage{3, 1, {{1, 0, 0, 0}, {.2F, .4F, .6F, 1}, {0, 0, 1, 0}}};
    for (auto shape : {MorphologyShape::Square, MorphologyShape::Round}) {
        auto eroded = minimum(coverage.view(), {1, shape, true});
        auto dilated = maximum(coverage.view(), {1, shape, true});
        auto colorsOnly = minimum(coverage.view(), {1, shape, false});
        check(eroded && eroded->pixels[1].a == 0, "minimum erodes opaque center to zero alpha");
        check(dilated && dilated->pixels[0] == coverage.pixels[1] &&
                  dilated->pixels[2] == coverage.pixels[1],
              "maximum expands alpha with visible neighbor color");
        check(colorsOnly && colorsOnly->pixels[1].a == 1 && colorsOnly->pixels[0].a == 0,
              "morphology color-only preserves coverage");
    }
    for (auto shape : {MorphologyShape::Square, MorphologyShape::Round}) {
        contract(minimum(view, {2, shape}), view);
        contract(maximum(view, {2, shape, false}), view, true);
    }
    Image avg{2, 1, {{1, 0, 0, 1}, {0, 0, 1, .5F}}};
    auto av = average(avg.view());
    check(av && near(av->pixels[0].r, 2. / 3) && near(av->pixels[0].b, 1. / 3) &&
              av->pixels[1].a == .5F,
          "average alpha weighted and original alpha retained");
    const auto tinyAlpha = std::numeric_limits<float>::denorm_min();
    Image tinyAverage{2, 1, {{.4F, .25F, .5F, tinyAlpha}, {1, 0, 1, 0}}};
    auto visibleAverage = average(tinyAverage.view());
    check(visibleAverage && visibleAverage->pixels[0] == tinyAverage.pixels[0],
          "visible mean retains tiny source coverage and color");
    auto a = filterAddNoise(view), b = filterAddNoise(view);
    check(a && b && a->pixels == b->pixels, "noise hash repeatable");
    for (auto model : {FilterColorModel::Hsb, FilterColorModel::Hsl}) {
        auto code = hsbHsl(view, FilterColorModel::Rgb, model);
        check(bool(code), "color encoding succeeds");
        if (code) {
            auto restored = hsbHsl(code->view(), model, FilterColorModel::Rgb);
            check(bool(restored), "color decoding succeeds");
            if (restored)
                for (std::size_t i = 0; i < image.pixels.size(); ++i)
                    check(near(restored->pixels[i].r, image.pixels[i].r) &&
                              near(restored->pixels[i].g, image.pixels[i].g) &&
                              near(restored->pixels[i].b, image.pixels[i].b),
                          "HSB/HSL round trip oracle");
        }
    }
    Image tinyColor{1, 1, {{1e-30F, 2e-31F, 2e-31F, 1}}};
    auto tinyHsl = hsbHsl(tinyColor.view(), FilterColorModel::Rgb, FilterColorModel::Hsl);
    check(tinyHsl && near(tinyHsl->pixels[0].g, 2. / 3),
          "tiny-lightness HSL keeps finite correct saturation");
    if (tinyHsl) {
        auto restored = hsbHsl(tinyHsl->view(), FilterColorModel::Hsl, FilterColorModel::Rgb);
        check(restored && std::abs(restored->pixels[0].r / tinyColor.pixels[0].r - 1) < 1e-5 &&
                  std::abs(restored->pixels[0].g / tinyColor.pixels[0].g - 1) < 1e-5,
              "tiny-lightness HSL inverse keeps RGB");
    }
    Image video{1, 3, {{0, 0, 0, 1}, {.5F, .5F, .5F, 1}, {1, 1, 1, 1}}};
    auto v = deInterlace(video.view(), false, true);
    check(v && near(v->pixels[0].r, .5) && near(v->pixels[2].r, .5) && near(v->pixels[1].r, .5),
          "deinterlace odd fields rebuilt from retained line");
    contract(reduceNoise(view, {10, 100, 100, 100, true}), view, true);
}
void isolation() {
    auto image = fixture(), other = image;
    for (auto &p : other.pixels)
        if (p.a == 0)
            p = {1 - p.r, 1 - p.g, 1 - p.b, 0};
    auto compare = [&](const Result<Image> &a, const Result<Image> &b) {
        check(a && b, "hidden-isolation filters succeed");
        if (a && b)
            for (std::size_t i = 0; i < a->pixels.size(); ++i)
                if (a->pixels[i].a > 0)
                    check(a->pixels[i] == b->pixels[i], "hidden RGB cannot affect output");
    };
    compare(boxBlur(image.view()), boxBlur(other.view()));
    compare(motionBlur(image.view()), motionBlur(other.view()));
    compare(shapeBlur(image.view()), shapeBlur(other.view()));
    compare(surfaceBlur(image.view()), surfaceBlur(other.view()));
    compare(lensBlur(image.view()), lensBlur(other.view()));
    compare(median(image.view()), median(other.view()));
    compare(reduceNoise(image.view()), reduceNoise(other.view()));
    compare(customFilter(image.view()), customFilter(other.view()));
    compare(guidedBlur(image.view()), guidedBlur(other.view()));
    compare(minimum(image.view()), minimum(other.view()));
    compare(maximum(image.view()), maximum(other.view()));
    compare(twirl(image.view()), twirl(other.view()));
    for (auto [w, h] : std::array<std::array<std::size_t, 2>, 3>{{{1, 31}, {31, 1}, {1, 1}}}) {
        auto i = fixture(w, h);
        contract(shapeBlur(i.view(), {3, BlurShape::Heart}), i.view());
        contract(pathBlur(i.view(), std::array{BlurPath{}}), i.view());
        contract(reduceNoise(i.view()), i.view());
        contract(polarCoordinates(i.view()), i.view());
    }
}
void validation() {
    auto image = fixture();
    auto view = image.view();
    auto nan = std::numeric_limits<double>::quiet_NaN();
    check(!filterGaussianBlur(view, nan) && !boxBlur(view, 2001), "blur bounds reject invalid");
    check(!motionBlur(view, {nan, 1}) && !radialBlur(view, {1, RadialBlurMethod(99)}),
          "blur invalid options reject");
    check(!smartBlur(view, {3, 25, FilterBlurQuality(99)}) && !shapeBlur(view, {1, BlurShape(99)}),
          "unknown blur enum reject");
    LensBlurOptions l;
    l.depth = LensDepthSource::Map;
    check(!lensBlur(view, l), "missing depth map rejected");
    check(!unsharpMask(view, {501}) && !smartSharpen(view, {1, 1, nan}), "sharpen bounds reject");
    check(!filterAddNoise(view, {1, NoiseDistribution(99)}) && !median(view, 501),
          "noise bounds reject");
    check(!minimum(view, {1, MorphologyShape(99)}) &&
              !hsbHsl(view, FilterColorModel(99), FilterColorModel::Rgb),
          "utility unknown enums reject");
    CustomFilterOptions c;
    c.kernel[0] = nan;
    check(!customFilter(view, c), "nonfinite kernel reject");
    check(!guidedBlur(view, {1, 0}) && !crossGuidedBlur(view, Image{1, 1, {{}}}.view()),
          "guide epsilon and dimensions reject");
    check(!twirl(view, nan) && !spherize(view, {1, SpherizeMode(99)}) && !wave(view, {65}),
          "distortion invalid settings reject");
    check(!shear(view, std::array{ShearPoint{0, 0}, ShearPoint{0, 1}}),
          "duplicate shear knots reject");
    check(!polarCoordinates(view, PolarMode(99)), "polar unknown enum reject");
    check(!fieldBlur(view, std::array{FieldBlurPin{.5, .5, nan}}) &&
              !pathBlur(view, std::array{BlurPath{{{.5, .5}}}}),
          "malformed blur pins paths reject");
    check(!boxBlur({}) && !minimum({}) && !offset({}, 0, 0), "empty images reject");
}
} // namespace
int main(int argc, char **argv) {
    std::string_view mode = argc > 1 ? argv[1] : "all";
    if (mode == "all" || mode == "blur")
        blurCases();
    if (mode == "all" || mode == "distortion")
        distortionCases();
    if (mode == "all" || mode == "utility")
        utilityCases();
    if (mode == "all" || mode == "isolation")
        isolation();
    if (mode == "all" || mode == "validation")
        validation();
    return failures ? 1 : 0;
}
