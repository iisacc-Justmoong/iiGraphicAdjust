#include <iiGraphicAdjust/Masks.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

using namespace iiGraphicAdjust;

namespace {
struct Checks final {
    int failures = 0;
    void require(bool ok, const char *reason) {
        if (!ok) {
            if (failures < 25)
                std::cerr << reason << '\n';
            ++failures;
        }
    }
};

struct Values final {
    std::uint64_t state = 0xa86e1d427bd530c9ULL;
    double unit() {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return double((state * 0x2545f4914f6cdd1dULL) >> 11) / 9007199254740992.;
    }
    double between(double low, double high) {
        return low + (high - low) * unit();
    }
};

double linear(double v) {
    return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
}
float encoded(double v) {
    v = std::clamp(v, 0., 1.);
    return float(v <= .0031308 ? v * 12.92 : 1.055 * std::pow(v, 1. / 2.4) - .055);
}
bool normalized(float v) {
    return std::isfinite(v) && v >= 0 && v <= 1;
}
bool normalized(Rgba p) {
    return normalized(p.r) && normalized(p.g) && normalized(p.b) && normalized(p.a);
}
void maskContract(Checks &checks, const Result<Mask> &result, std::size_t w, std::size_t h) {
    checks.require(bool(result), "valid mask operation succeeds");
    if (!result)
        return;
    checks.require(result->width == w && result->height == h && result->coverage.size() == w * h,
                   "mask dimensions and storage preserved");
    for (auto v : result->coverage)
        checks.require(normalized(v), "mask coverage is finite and normalized");
}
void imageContract(Checks &checks, const Result<Image> &result, const Image &source) {
    checks.require(bool(result), "valid local adjustment succeeds");
    if (!result)
        return;
    checks.require(result->width == source.width && result->height == source.height &&
                       result->pixels.size() == source.pixels.size(),
                   "local adjustment dimensions preserved");
    checks.require(result->pixels.data() != source.pixels.data(), "output owns detached storage");
    if (result->pixels.size() != source.pixels.size())
        return;
    for (std::size_t i = 0; i < source.pixels.size(); ++i) {
        checks.require(normalized(result->pixels[i]), "local output is finite and normalized");
        checks.require(result->pixels[i].a == source.pixels[i].a, "local alpha preserved exactly");
        if (source.pixels[i].a == 0)
            checks.require(result->pixels[i] == source.pixels[i], "local hidden RGB preserved");
    }
}

void automask(Checks &checks) {
    Image image{64, 32, std::vector<Rgba>(64 * 32, {.25F, .25F, .25F, 1})};
    BrushStroke stroke;
    stroke.points = {{28. / 64, .5}};
    stroke.radius = .125;
    stroke.feather = 0;
    const std::size_t selected = 16 * 64 + 28, across = 16 * 64 + 35;
    for (int edge = 0; edge < 2; ++edge) {
        const Rgba left = edge == 0 ? Rgba{.25F, .25F, .25F, 1}
                                    : Rgba{encoded(.18), encoded(.18), encoded(.18), 1};
        // The second boundary changes chromaticity while holding linear luminance fixed.
        const Rgba right =
            edge == 0 ? Rgba{.7F, .7F, .7F, 1}
                      : Rgba{encoded(.7), encoded((.18 - .2126 * .7 - .0722 * .02) / .7152),
                             encoded(.02), 1};
        for (std::size_t y = 0; y < image.height; ++y)
            for (std::size_t x = 0; x < image.width; ++x)
                image.pixels[y * image.width + x] = x < 32 ? left : right;
        stroke.autoMask = false;
        auto plain = brushMask(image.view(), std::span{&stroke, 1});
        stroke.autoMask = true;
        auto automatic = brushMask(image.view(), std::span{&stroke, 1});
        maskContract(checks, plain, 64, 32);
        maskContract(checks, automatic, 64, 32);
        if (plain && automatic) {
            checks.require(plain->coverage[across] == 1, "unrestricted brush crosses boundary");
            checks.require(automatic->coverage[selected] == 1,
                           "Auto Mask retains paint on the sampled region");
            checks.require(automatic->coverage[across] < .2F,
                           "Auto Mask suppresses both luminance and chromaticity boundaries");
        }
    }
    std::fill(image.pixels.begin(), image.pixels.end(), Rgba{.4F, .4F, .4F, 1});
    stroke.points = {{.5, .5}, {.5, .5}, {.5, .5}};
    stroke.flow = 50;
    stroke.density = 60;
    auto accumulated = brushMask(image.view(), std::span{&stroke, 1});
    maskContract(checks, accumulated, 64, 32);
    if (accumulated)
        checks.require(std::abs(accumulated->coverage[16 * 64 + 32] - .6F) < 1e-6,
                       "Auto Mask flow accumulates up to density on a uniform region");
    std::array<BrushStroke, 2> erase{stroke, stroke};
    erase[0].flow = erase[0].density = 100;
    erase[1].erase = true;
    auto erased = brushMask(image.view(), erase);
    maskContract(checks, erased, 64, 32);
    if (erased)
        checks.require(std::abs(erased->coverage[16 * 64 + 32] - .4F) < 1e-6,
                       "Auto Mask erase removes the painted fraction");
}

void nearDegenerate(Checks &checks) {
    for (double separation :
         {0., std::numeric_limits<double>::denorm_min(), 1e-200, std::nextafter(1e-9, 0.)})
        checks.require(!linearMask(7, 3, {{0, 0}, {separation, 0}}),
                       "gradient below minimum separation rejected");
    for (const LinearMaskOptions &options :
         {LinearMaskOptions{{0, 0}, {1e-9, 0}}, LinearMaskOptions{{0, 0}, {.8e-9, .8e-9}},
          LinearMaskOptions{{.5, .5}, {.500000002, .5}}})
        maskContract(checks, linearMask(7, 3, options), 7, 3);
    checks.require(!linearMask(1, 1, {{0, 0}, {std::numeric_limits<double>::infinity(), 0}}),
                   "nonfinite linear endpoints rejected");
    maskContract(checks, radialMask(7, 3, {{.5, .5}, 1e-6, 1e-6, 180, 0, false}), 7, 3);
}

void exactOutside(Checks &checks) {
    Image source{64, 1, {}};
    Mask coverage{64, 1, {}};
    float knee = .0404499F;
    for (int i = 0; i < 64; ++i) {
        source.pixels.push_back(
            {knee, i % 2 ? knee : .6F, i % 3 ? knee : .123F, i % 11 ? 1.F : 0.F});
        coverage.coverage.push_back(i % 3 == 0 ? 1.F : 0.F);
        knee = std::nextafter(knee, 1.F);
    }
    const Image original = source;
    LocalAdjustment adjustment{coverage.view(), {}};
    adjustment.options.exposure = 1;
    auto applied = applyLocalAdjustments(source.view(), std::span{&adjustment, 1});
    imageContract(checks, applied, source);
    bool changed = false;
    if (applied)
        for (std::size_t i = 0; i < source.pixels.size(); ++i) {
            if (coverage.coverage[i] == 0 || source.pixels[i].a == 0)
                checks.require(applied->pixels[i] == source.pixels[i],
                               "outside-mask source is exact at transfer-function knee");
            else
                changed = changed || applied->pixels[i] != source.pixels[i];
        }
    checks.require(changed, "covered visible pixels receive the adjustment");
    checks.require(source.pixels == original.pixels, "local processing leaves source untouched");
    std::array<LocalAdjustment, 2> cancellation{adjustment, adjustment};
    cancellation[0].options.amount = 200;
    cancellation[1].options.exposure = -2;
    auto canceled = applyLocalAdjustments(source.view(), cancellation);
    checks.require(canceled && canceled->pixels == source.pixels,
                   "weighted cancellation returns an exact copy including transfer knee");
}

void randomContributions(Checks &checks) {
    Values values;
    Image source{9, 5, {}};
    for (std::size_t i = 0; i < 45; ++i) {
        const auto gray = float(values.between(.005, .7));
        source.pixels.push_back({gray, gray, gray,
                                 i % 13 == 0  ? 0.F
                                 : i % 7 == 0 ? std::numeric_limits<float>::denorm_min()
                                              : float(values.between(.01, 1))});
    }
    std::array<Mask, 32> masks;
    std::array<LocalAdjustment, 32> adjustments;
    for (std::size_t k = 0; k < masks.size(); ++k) {
        masks[k] = {source.width, source.height, {}};
        for (std::size_t i = 0; i < source.pixels.size(); ++i)
            masks[k].coverage.push_back(i % 9 ? float(values.unit()) : 0.F);
        adjustments[k].mask = masks[k].view();
        adjustments[k].options.exposure = values.between(-5, 5);
        adjustments[k].options.amount = values.between(0, 200);
    }
    auto result = applyLocalAdjustments(source.view(), adjustments);
    imageContract(checks, result, source);
    if (result)
        for (std::size_t i = 0; i < source.pixels.size(); ++i) {
            double stops = 0;
            for (const auto &a : adjustments)
                stops += a.mask.coverage[i] * a.options.amount / 100 * a.options.exposure;
            if (stops == 0 || source.pixels[i].a == 0) {
                checks.require(result->pixels[i] == source.pixels[i],
                               "zero summed exposure preserves exact source");
                continue;
            }
            const auto expected = encoded(linear(source.pixels[i].r) * std::exp2(stops));
            checks.require(std::abs(result->pixels[i].r - expected) < 3e-6 &&
                               std::abs(result->pixels[i].g - expected) < 3e-6 &&
                               std::abs(result->pixels[i].b - expected) < 3e-6,
                           "random mask contributions obey the radiometric exposure oracle");
        }
    std::reverse(adjustments.begin(), adjustments.end());
    auto reverse = applyLocalAdjustments(source.view(), adjustments);
    imageContract(checks, reverse, source);
    if (result && reverse)
        for (std::size_t i = 0; i < source.pixels.size(); ++i)
            checks.require(std::abs(result->pixels[i].r - reverse->pixels[i].r) < 3e-6,
                           "exposure accumulation is independent of mask ordering");
}

Image coloredFixture() {
    Values values;
    Image image{8, 6, {}};
    for (std::size_t i = 0; i < 48; ++i)
        image.pixels.push_back({float(values.unit()), float(values.unit()), float(values.unit()),
                                i % 7 == 0 ? 0.F : float(values.between(.01, 1))});
    image.pixels[1] = {0, 0, 0, 1};
    image.pixels[2] = {1, 1, 1, 1};
    image.pixels[3] = {1, 0, 0, .5F};
    image.pixels[4] = {0, 1, 0, .5F};
    image.pixels[5] = {0, 0, 1, .5F};
    image.pixels[6] = {
        std::numeric_limits<float>::denorm_min(), std::numeric_limits<float>::denorm_min(),
        std::numeric_limits<float>::denorm_min(), std::numeric_limits<float>::denorm_min()};
    return image;
}

void extremes(Checks &checks) {
    const Image source = coloredFixture();
    Mask full{source.width, source.height, std::vector<float>(source.pixels.size(), 1)};
    std::array<LocalAdjustment, 32> adjustments;
    for (double sign : {-1., 1.}) {
        for (auto &a : adjustments) {
            a = {full.view(), {}};
            auto &o = a.options;
            o.exposure = sign * 5;
            o.temperature = o.tint = o.contrast = o.highlights = o.shadows = o.whites = o.blacks =
                o.texture = o.clarity = o.dehaze = o.hue = o.saturation = o.sharpness = o.noise =
                    o.moire = sign * 100;
            o.defringe = o.colorSaturation = 100;
            o.colorHue = sign < 0 ? 0 : 360;
            o.amount = 200;
        }
        imageContract(checks, applyLocalAdjustments(source.view(), adjustments), source);
    }
    Values values;
    for (int run = 0; run < 16; ++run) {
        for (auto &a : adjustments) {
            auto &o = a.options;
            o.exposure = values.between(-5, 5);
            for (auto member :
                 {&LocalAdjustmentOptions::temperature, &LocalAdjustmentOptions::tint,
                  &LocalAdjustmentOptions::contrast, &LocalAdjustmentOptions::highlights,
                  &LocalAdjustmentOptions::shadows, &LocalAdjustmentOptions::whites,
                  &LocalAdjustmentOptions::blacks, &LocalAdjustmentOptions::texture,
                  &LocalAdjustmentOptions::clarity, &LocalAdjustmentOptions::dehaze,
                  &LocalAdjustmentOptions::hue, &LocalAdjustmentOptions::saturation,
                  &LocalAdjustmentOptions::sharpness, &LocalAdjustmentOptions::noise,
                  &LocalAdjustmentOptions::moire})
                o.*member = values.between(-100, 100);
            o.defringe = values.between(0, 100);
            o.colorHue = values.between(0, 360);
            o.colorSaturation = values.between(0, 100);
            o.amount = values.between(0, 200);
        }
        imageContract(checks, applyLocalAdjustments(source.view(), adjustments), source);
    }
}

void hiddenIsolation(Checks &checks) {
    const Image source = coloredFixture();
    Image alternate = source;
    for (std::size_t i = 0; i < source.pixels.size(); ++i)
        if (source.pixels[i].a == 0)
            alternate.pixels[i] = {1 - source.pixels[i].r, 1 - source.pixels[i].g,
                                   1 - source.pixels[i].b, 0};
    Mask full{source.width, source.height, std::vector<float>(source.pixels.size(), 1)};
    LocalAdjustment adjustment{full.view(), {}};
    auto &o = adjustment.options;
    o.highlights = 50;
    o.shadows = -50;
    o.texture = 75;
    o.clarity = 50;
    o.dehaze = 65;
    o.sharpness = 30;
    o.moire = 60;
    o.noise = 50;
    o.defringe = 80;
    auto a = applyLocalAdjustments(source.view(), std::span{&adjustment, 1}),
         b = applyLocalAdjustments(alternate.view(), std::span{&adjustment, 1});
    imageContract(checks, a, source);
    imageContract(checks, b, alternate);
    if (a && b)
        for (std::size_t i = 0; i < source.pixels.size(); ++i)
            if (source.pixels[i].a > 0)
                checks.require(a->pixels[i] == b->pixels[i],
                               "all local spatial planes exclude hidden RGB");
    Mask boundary = full;
    for (std::size_t i = 0; i < boundary.coverage.size(); ++i)
        boundary.coverage[i] = i % 3 ? .25F : .75F;
    auto ra = refineMask(source.view(), boundary.view(), 100),
         rb = refineMask(alternate.view(), boundary.view(), 100);
    maskContract(checks, ra, source.width, source.height);
    maskContract(checks, rb, source.width, source.height);
    if (ra && rb)
        for (std::size_t i = 0; i < source.pixels.size(); ++i)
            if (source.pixels[i].a > 0)
                checks.require(ra->coverage[i] == rb->coverage[i],
                               "guided mask refinement excludes hidden RGB from visible coverage");
    BrushStroke brush;
    brush.points = {{4.5 / 8, 2.5 / 6}};
    brush.radius = .3;
    brush.autoMask = true;
    auto ba = brushMask(source.view(), std::span{&brush, 1}),
         bb = brushMask(alternate.view(), std::span{&brush, 1});
    maskContract(checks, ba, source.width, source.height);
    maskContract(checks, bb, source.width, source.height);
    if (ba && bb) {
        checks.require(ba->coverage[20] > 0 && bb->coverage[20] > 0,
                       "hidden-isolation brush samples a visible pixel and paints coverage");
        for (std::size_t i = 0; i < source.pixels.size(); ++i)
            if (source.pixels[i].a > 0)
                checks.require(ba->coverage[i] == bb->coverage[i],
                               "Auto Mask sampling and refinement isolate hidden RGB");
    }
}
} // namespace

int main() {
    Checks checks;
    automask(checks);
    nearDegenerate(checks);
    exactOutside(checks);
    randomContributions(checks);
    extremes(checks);
    hiddenIsolation(checks);
    if (checks.failures)
        std::cerr << "Mask invariant failures: " << checks.failures << '\n';
    return checks.failures ? 1 : 0;
}
