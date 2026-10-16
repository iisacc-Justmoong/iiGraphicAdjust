#include <iiGraphicAdjust/LocalCorrections.h>
#include <iiGraphicAdjust/CameraRawFilter.h>
#include <iiGraphicAdjust/Color.h>
#include <iiGraphicAdjust/DevelopEffects.h>
#include <iiGraphicAdjust/DevelopTone.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

using namespace iiGraphicAdjust;
namespace {
struct Checks final {
    int failures = 0;
    void require(bool ok, const char *reason) {
        if (!ok) {
            if (failures < 20)
                std::cerr << reason << '\n';
            ++failures;
        }
    }
};
bool exact(float a, float b) { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }
bool exact(Rgba a, Rgba b) {
    return exact(a.r, b.r) && exact(a.g, b.g) && exact(a.b, b.b) && exact(a.a, b.a);
}
bool exact(const Image &a, const Image &b) {
    return a.width == b.width && a.height == b.height && a.pixels.size() == b.pixels.size() &&
           std::equal(a.pixels.begin(), a.pixels.end(), b.pixels.begin(),
                      [](Rgba x, Rgba y) { return exact(x, y); });
}
void contract(Checks &c, const Result<Image> &result, const Image &base, const Image &corrected) {
    c.require(bool(result), "valid masked correction succeeds");
    if (!result) return;
    c.require(result->width == base.width && result->height == base.height &&
                  result->pixels.size() == base.pixels.size(), "output geometry is preserved");
    c.require(result->pixels.data() != base.pixels.data() &&
                  result->pixels.data() != corrected.pixels.data(), "output storage is detached from both images");
    if (result->pixels.size() != base.pixels.size()) return;
    for (std::size_t i = 0; i < base.pixels.size(); ++i) {
        const auto p = result->pixels[i];
        c.require(exact(p.a, base.pixels[i].a), "base alpha is preserved bitwise");
        if (base.pixels[i].a == 0)
            c.require(exact(p, base.pixels[i]), "hidden base RGB is preserved bitwise");
        for (float channel : {p.r, p.g, p.b, p.a})
            c.require(std::isfinite(channel) && channel >= 0 && channel <= 1,
                      "output channels are finite and normalized");
    }
}
void identity(Checks &c) {
    Image base{4, 1, {{.04045F, .123F, .6F, .8F}, {-0.F, .8F, .4F, 1},
                      {.2F, .4F, .6F, 0}, {std::numeric_limits<float>::denorm_min(), .0031308F, .9F, -0.F}}};
    const Image before = base;
    Image corrected{4, 1, {{.9F, .2F, .8F, 0}, {.7F, .2F, .9F, .4F},
                           {.8F, .2F, .1F, 1}, {.9F, .9F, .9F, .5F}}};
    const Image correctionBefore = corrected;
    Mask zero{4, 1, {0, -0.F, 0, 0}}, full{4, 1, {1, 1, 1, 1}};
    for (auto space : {CorrectionSpace::Srgb, CorrectionSpace::LinearSrgb}) {
        auto z = applyMaskedCorrection(base.view(), corrected.view(), zero.view(), {100, space});
        auto a = applyMaskedCorrection(base.view(), corrected.view(), full.view(), {0, space, true});
        auto alias = applyMaskedCorrection(base.view(), base.view(), full.view(), {63, space});
        contract(c, z, base, corrected); contract(c, a, base, corrected); contract(c, alias, base, base);
        c.require(z && exact(*z, base), "zero coverage is an exact detached copy at the transfer knee");
        c.require(a && exact(*a, base), "zero amount is an exact detached copy including signed zero");
        c.require(alias && exact(*alias, base), "aliased equal inputs preserve original bits");
    }
    c.require(exact(base, before) && exact(corrected, correctionBefore), "both image inputs remain unchanged");
    c.require(zero.coverage == std::vector<float>{0, 0, 0, 0}, "mask input remains unchanged");
}
void coverage(Checks &c) {
    Image base{4, 1, std::vector<Rgba>(4, {0, 0, 0, .375F})};
    Image corrected{4, 1, std::vector<Rgba>(4, {1, 1, 1, 0})};
    Mask mask{4, 1, {0, .25F, .5F, 1}};
    auto direct = applyMaskedCorrection(base.view(), corrected.view(), mask.view(), {100, CorrectionSpace::Srgb});
    auto half = applyMaskedCorrection(base.view(), corrected.view(), mask.view(), {50, CorrectionSpace::Srgb});
    auto inverse = applyMaskedCorrection(base.view(), corrected.view(), mask.view(), {100, CorrectionSpace::Srgb, true});
    contract(c, direct, base, corrected); contract(c, half, base, corrected); contract(c, inverse, base, corrected);
    if (direct && half && inverse)
        for (std::size_t i = 0; i < 4; ++i) {
            c.require(exact(direct->pixels[i].r, mask.coverage[i]), "encoded coverage controls the RGB fraction");
            c.require(exact(half->pixels[i].r, mask.coverage[i] * .5F), "amount scales fractional coverage");
            c.require(exact(inverse->pixels[i].r, 1 - mask.coverage[i]), "invert reverses the mask before amount");
        }
    corrected.pixels[3] = {.04045F, -0.F, std::numeric_limits<float>::denorm_min(), 1};
    auto full = applyMaskedCorrection(base.view(), corrected.view(), mask.view());
    c.require(full && exact(full->pixels[3].r, corrected.pixels[3].r) &&
                  exact(full->pixels[3].g, corrected.pixels[3].g) &&
                  exact(full->pixels[3].b, corrected.pixels[3].b), "full RGB copy bypasses transfer functions bitwise");
}
void spaces(Checks &c) {
    Image base{1, 1, {{0, 0, 0, 1}}}, corrected{1, 1, {{1, 1, 1, 1}}};
    Mask half{1, 1, {.5F}};
    auto encoded = applyMaskedCorrection(base.view(), corrected.view(), half.view(), {100, CorrectionSpace::Srgb});
    auto linear = applyMaskedCorrection(base.view(), corrected.view(), half.view());
    c.require(encoded && exact(encoded->pixels[0].r, .5F), "encoded black/white midpoint is 0.5");
    c.require(linear && std::abs(linear->pixels[0].r - .735356983F) < 2e-7,
              "linear black/white midpoint follows the independent sRGB transfer oracle");
    base.pixels[0] = {.04045F, .7F, .1F, .5F};
    corrected.pixels[0] = {.04045F, .1F, .9F, 0};
    auto equalChannel = applyMaskedCorrection(base.view(), corrected.view(), half.view());
    c.require(equalChannel && exact(equalChannel->pixels[0].r, base.pixels[0].r),
              "an unchanged channel avoids an unnecessary transfer round trip");
    std::mt19937 random(0x3ff9162U);
    for (int iteration = 0; iteration < 200; ++iteration) {
        for (auto p : {&base.pixels[0], &corrected.pixels[0]})
            *p = {float(random() % 65536) / 65535, float(random() % 65536) / 65535,
                  float(random() % 65536) / 65535, .5F};
        half.coverage[0] = float(random() % 65536) / 65535;
        Mask complement{1, 1, {1 - half.coverage[0]}};
        for (auto space : {CorrectionSpace::Srgb, CorrectionSpace::LinearSrgb}) {
            auto result = applyMaskedCorrection(base.view(), corrected.view(), half.view(), {100, space});
            auto reverse = applyMaskedCorrection(corrected.view(), base.view(), complement.view(), {100, space});
            contract(c, result, base, corrected);
            if (!result || !reverse) continue;
            const auto a = base.pixels[0], b = corrected.pixels[0], r = result->pixels[0], v = reverse->pixels[0];
            for (const auto values : {std::array{a.r, b.r, r.r, v.r}, std::array{a.g, b.g, r.g, v.g}, std::array{a.b, b.b, r.b, v.b}}) {
                c.require(values[2] >= std::min(values[0], values[1]) - 1e-7F &&
                              values[2] <= std::max(values[0], values[1]) + 1e-7F, "interpolation remains within RGB endpoints");
                c.require(std::abs(values[2] - values[3]) < 5e-7F, "swapped images and complementary coverage agree");
            }
        }
    }
}
void alpha(Checks &c) {
    Image base{4, 1, {{.2F, .3F, .4F, .2F}, {.2F, .3F, .4F, 0},
                      {.7F, .6F, .5F, -0.F}, {.1F, .2F, .3F, 1}}};
    Image corrected{4, 1, std::vector<Rgba>(4, {.8F, .7F, .6F, 0})};
    Mask full{4, 1, {1, 1, 1, 1}};
    auto result = applyMaskedCorrection(base.view(), corrected.view(), full.view());
    contract(c, result, base, corrected);
    c.require(result && exact(result->pixels[0].r, .8F) && exact(result->pixels[3].r, .8F),
              "corrected alpha zero does not suppress visible base RGB correction");
    c.require(result && exact(result->pixels[1], base.pixels[1]) && exact(result->pixels[2], base.pixels[2]),
              "full coverage preserves hidden RGB and signed-zero alpha");
}
void validation(Checks &c) {
    const Image base{2, 1, {{.2F, .3F, .4F, 1}, {.4F, .3F, .2F, 0}}};
    const Image corrected{2, 1, {{.8F, .7F, .6F, 1}, {.6F, .7F, .8F, 1}}};
    const Mask mask{2, 1, {1, 0}};
    auto error = [&](const Result<Image> &r, ErrorCode expected, const char *why) {
        c.require(!r && r.error().code == expected, why);
    };
    error(applyMaskedCorrection({}, corrected.view(), mask.view()), ErrorCode::InvalidDimensions, "empty base geometry rejected");
    error(applyMaskedCorrection(base.view(), {}, mask.view()), ErrorCode::InvalidDimensions, "empty correction geometry rejected");
    error(applyMaskedCorrection(base.view(), corrected.view(), {}), ErrorCode::InvalidDimensions, "empty mask geometry rejected");
    error(applyMaskedCorrection({std::numeric_limits<std::size_t>::max(), 2, {}}, corrected.view(), mask.view()),
          ErrorCode::InvalidDimensions, "overflow geometry rejected before storage access");
    error(applyMaskedCorrection({2, 1, std::span(base.pixels).first(1)}, corrected.view(), mask.view()),
          ErrorCode::InvalidBuffer, "short base storage rejected");
    error(applyMaskedCorrection(base.view(), {2, 1, std::span(corrected.pixels).first(1)}, mask.view()),
          ErrorCode::InvalidBuffer, "short correction storage rejected");
    error(applyMaskedCorrection(base.view(), corrected.view(), {2, 1, std::span(mask.coverage).first(1)}),
          ErrorCode::InvalidBuffer, "short mask storage rejected");
    const std::array<Rgba, 3> excess{};
    const std::array<float, 3> excessMask{};
    error(applyMaskedCorrection({2, 1, excess}, corrected.view(), mask.view()), ErrorCode::InvalidBuffer, "excess base storage rejected");
    error(applyMaskedCorrection(base.view(), {2, 1, excess}, mask.view()), ErrorCode::InvalidBuffer, "excess correction storage rejected");
    error(applyMaskedCorrection(base.view(), corrected.view(), {2, 1, excessMask}), ErrorCode::InvalidBuffer, "excess mask storage rejected");
    error(applyMaskedCorrection(base.view(), {1, 2, corrected.pixels}, mask.view()), ErrorCode::SizeMismatch, "transposed correction geometry rejected");
    error(applyMaskedCorrection(base.view(), corrected.view(), {1, 2, mask.coverage}), ErrorCode::SizeMismatch, "transposed mask geometry rejected");
    for (float bad : {-1.F, 1.1F, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()}) {
        for (int channel = 0; channel < 4; ++channel) {
            auto invalidBase = base, invalidCorrection = corrected;
            auto set = [channel, bad](Rgba &p) { switch (channel) { case 0: p.r = bad; break; case 1: p.g = bad; break; case 2: p.b = bad; break; default: p.a = bad; } };
            set(invalidBase.pixels[1]); set(invalidCorrection.pixels[1]);
            error(applyMaskedCorrection(invalidBase.view(), corrected.view(), mask.view(), {0}), ErrorCode::InvalidPixel, "inactive/hidden invalid base channel rejected");
            error(applyMaskedCorrection(base.view(), invalidCorrection.view(), mask.view(), {0}), ErrorCode::InvalidPixel, "inactive invalid correction channel rejected");
        }
        auto invalidMask = mask; invalidMask.coverage[1] = bad;
        error(applyMaskedCorrection(base.view(), corrected.view(), invalidMask.view(), {0}), ErrorCode::InvalidParameter, "inactive invalid coverage rejected");
    }
    for (double amount : {-1., 101., std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()})
        error(applyMaskedCorrection(base.view(), corrected.view(), mask.view(), {amount}), ErrorCode::InvalidParameter, "invalid amount rejected");
    error(applyMaskedCorrection(base.view(), corrected.view(), mask.view(), {0, static_cast<CorrectionSpace>(-1)}), ErrorCode::InvalidParameter, "inactive invalid interpolation enum rejected");
}
void composition(Checks &c) {
    Image base{16, 8, {}};
    Mask mask{16, 8, {}};
    for (std::size_t i = 0; i < 128; ++i) {
        base.pixels.push_back({float(i % 13) / 13, float(i % 7) / 7, float(i % 17) / 17, i % 11 == 0 ? 0.F : .6F});
        mask.coverage.push_back(std::array{0.F, .25F, .75F, 1.F}[i % 4]);
    }
    const Image original = base;
    const Mask originalMask = mask;
    LevelsOptions lev; lev.master.gamma = 1.4;
    CurvesOptions cur; cur.master.points = {{0, 0}, {.5, .65}, {1, 1}};
    DevelopCurveOptions dev; dev.master.points = {{0, 0}, {.5, .65}, {1, 1}};
    DevelopDetailOptions det; det.sharpenAmount = 55; det.luminanceNoise = 15;
    CameraRawOptions raw; raw.exposure = .5; raw.texture = 15;
    const std::array<Result<Image>, 8> corrections{
        levels(base.view(), lev), curves(base.view(), cur), whiteBalance(base.view(), {4200, 8}),
        hueSaturation(base.view(), {30, .2, 0}), developDetail(base.view(), det),
        cameraRawFilter(base.view(), raw), developCurve(base.view(), dev), developLight(base.view(), {.75})};
    for (const auto &corrected : corrections) {
        c.require(bool(corrected), "existing correction kernel succeeds");
        if (!corrected) continue;
        auto result = applyMaskedCorrection(base.view(), corrected->view(), mask.view());
        contract(c, result, base, *corrected);
        bool localizedChange = false;
        if (result)
            for (std::size_t i = 0; i < base.pixels.size(); ++i) {
                if (mask.coverage[i] == 0 || base.pixels[i].a == 0)
                    c.require(exact(result->pixels[i], base.pixels[i]), "each correction preserves unselected and hidden pixels");
                if (mask.coverage[i] == 1 && base.pixels[i].a > 0)
                    c.require(exact(result->pixels[i], corrected->pixels[i]), "each correction copies its selected full RGB exactly");
                localizedChange |= !exact(result->pixels[i], base.pixels[i]);
            }
        c.require(localizedChange, "each real correction produces a selected visible change");
    }
    c.require(exact(base, original) && mask.coverage == originalMask.coverage, "composition leaves base and mask unchanged");
}
} // namespace

int main(int argc, char **argv) {
    Checks checks;
    const std::string_view name = argc > 1 ? argv[1] : "";
    if (name == "identity") identity(checks);
    else if (name == "coverage") coverage(checks);
    else if (name == "spaces") spaces(checks);
    else if (name == "alpha") alpha(checks);
    else if (name == "validation") validation(checks);
    else if (name == "composition") composition(checks);
    else return 2;
    return checks.failures ? 1 : 0;
}
