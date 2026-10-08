// Algorithms adapted from LightCraft; see third_party/lightcraft/LICENSE-MIT.
#include "DevelopInternal.h"
#include "Internal.h"
#include <iiGraphicAdjust/DiagnosticFilters.h>

namespace iiGraphicAdjust {
namespace {
using Color = std::array<double, 3>;
using Matrix = develop_detail::Matrix;
using Space = DiagnosticColorSpace;
auto bad() {
    return std::unexpected(
        Error{ErrorCode::InvalidParameter,
              "Diagnostic parameter must be finite and within its documented range"});
}
Result<void> validate(ImageView in) {
    if (auto r = detail::validate(in); !r)
        return r;
    if (in.pixels.size() > 64000000)
        return std::unexpected(Error{ErrorCode::InvalidDimensions,
                                     "Diagnostic filters support at most 64000000 pixels"});
    return {};
}
bool validPoint(const PointColorOptions &p) {
    return detail::inRange(p.lightness, 0, 1) && detail::inRange(p.chroma, 0, .5) &&
           detail::inRange(p.hue, -180, 360) && detail::inRange(p.hueShift, -100, 100) &&
           detail::inRange(p.saturationShift, -100, 100) &&
           detail::inRange(p.luminanceShift, -100, 100) && detail::inRange(p.variance, -100, 100) &&
           detail::inRange(p.range, 0, 100) && detail::inRange(p.hueRange, 0, 100) &&
           detail::inRange(p.saturationRange, 0, 100) && detail::inRange(p.luminanceRange, 0, 100);
}
double membership(Color lab, const PointColorOptions &p) {
    auto c = std::hypot(lab[1], lab[2]), h = std::atan2(lab[2], lab[1]);
    auto k = std::max(.05, p.range / 50), wh = (.1 + .6 * p.hueRange / 100) * k,
         wc = (.02 + .12 * p.saturationRange / 100) * k,
         wl = (.05 + .4 * p.luminanceRange / 100) * k;
    auto hue = p.chroma < .02
                   ? 1
                   : (1 - develop_detail::smooth(
                              .5 * wh, wh,
                              std::abs(develop_detail::wrap(h - p.hue / 180 * std::numbers::pi)))) *
                         develop_detail::smooth(.005, .025, c);
    return hue * (1 - develop_detail::smooth(.5 * wc, wc, std::abs(c - p.chroma))) *
           (1 - develop_detail::smooth(.5 * wl, wl, std::abs(lab[0] - p.lightness)));
}
double grey(Rgba p) {
    return .2126 * p.r + .7152 * p.g + .0722 * p.b;
}
enum class MaskKind { Color, ColorOnBw, ImageOnBlack, ImageOnWhite, WhiteOnBlack };
Result<Image> mask(ImageView in, MaskView mask, MaskKind kind, const MaskOverlayOptions &o = {}) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (auto count = detail::pixelCount(mask.width, mask.height); !count)
        return std::unexpected(count.error());
    else if (*count != mask.coverage.size())
        return std::unexpected(
            Error{ErrorCode::InvalidBuffer, "Mask coverage must match its dimensions"});
    if (mask.width != in.width || mask.height != in.height)
        return std::unexpected(
            Error{ErrorCode::SizeMismatch, "Diagnostic mask must match image dimensions"});
    for (auto a : mask.coverage)
        if (!detail::inRange(a, 0, 1))
            return bad();
    if (!detail::inRange(o.opacity, 0, 100))
        return bad();
    for (auto c : o.color)
        if (!detail::inRange(c, 0, 1))
            return bad();
    return detail::allocated([&] {
        auto out = detail::copy(in);
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            auto p = in.pixels[i];
            if (p.a == 0)
                continue;
            Color c{p.r, p.g, p.b};
            double a = mask.coverage[i], g = grey(p);
            for (int channel = 0; channel < 3; ++channel)
                switch (kind) {
                case MaskKind::Color:
                    c[channel] += (o.color[channel] - c[channel]) * a * o.opacity / 100;
                    break;
                case MaskKind::ColorOnBw:
                    c[channel] = g + (o.color[channel] - g) * a * o.opacity / 100;
                    break;
                case MaskKind::ImageOnBlack:
                    c[channel] *= a;
                    break;
                case MaskKind::ImageOnWhite:
                    c[channel] = c[channel] * a + 1 - a;
                    break;
                case MaskKind::WhiteOnBlack:
                    c[channel] = a;
                    break;
                }
            out.pixels[i] = {float(detail::clamp(c[0])), float(detail::clamp(c[1])),
                             float(detail::clamp(c[2])), p.a};
        }
        return out;
    });
}
using Xy = std::array<double, 2>;
struct SpaceDefinition {
    std::array<Xy, 3> primaries;
    Xy white;
};
constexpr Xy d65{.3127, .3290}, d50{.3457, .3585};
constexpr std::array<SpaceDefinition, 5> spaces{{
    {{{{.64, .33}, {.30, .60}, {.15, .06}}}, d65},
    {{{{.680, .320}, {.265, .690}, {.150, .060}}}, d65},
    {{{{.64, .33}, {.21, .71}, {.15, .06}}}, d65},
    {{{{.734699, .265301}, {.159597, .840403}, {.036598, .000105}}}, d50},
    {{{{.708, .292}, {.170, .797}, {.131, .046}}}, d65},
}};
Color xyz(Xy p) {
    return {p[0] / p[1], 1, (1 - p[0] - p[1]) / p[1]};
}
Matrix spaceMatrix(const SpaceDefinition &s) {
    Matrix m{};
    for (int c = 0; c < 3; ++c) {
        auto v = xyz(s.primaries[c]);
        for (int r = 0; r < 3; ++r)
            m[r][c] = v[r];
    }
    auto scale = develop_detail::multiply(develop_detail::inverse(m), xyz(s.white));
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            m[r][c] *= scale[c];
    return m;
}
const std::array<Matrix, 5> &spaceMatrices() {
    static const std::array<Matrix, 5> matrices = [] {
        std::array<Matrix, 5> m{};
        for (std::size_t i = 0; i < m.size(); ++i)
            m[i] = spaceMatrix(spaces[i]);
        return m;
    }();
    return matrices;
}
Matrix adaptation(Xy source, Xy dest) {
    Matrix identity{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    if (source == dest)
        return identity;
    constexpr Matrix bradford{
        {{.8951, .2664, -.1614}, {-.7502, 1.7135, .0367}, {.0389, -.0685, 1.0296}}};
    auto a = develop_detail::multiply(bradford, xyz(source));
    auto b = develop_detail::multiply(bradford, xyz(dest));
    for (int i = 0; i < 3; ++i)
        identity[i][i] = b[i] / a[i];
    return develop_detail::multiply(develop_detail::inverse(bradford),
                                    develop_detail::multiply(identity, bradford));
}
Matrix conversion(Space source, Space dest) {
    auto src = std::size_t(source), dst = std::size_t(dest);
    return develop_detail::multiply(
        develop_detail::inverse(spaceMatrices()[dst]),
        develop_detail::multiply(adaptation(spaces[src].white, spaces[dst].white),
                                 spaceMatrices()[src]));
}
Color luma(Space s) {
    return s == Space::Srgb ? Color{.2126, .7152, .0722} : spaceMatrices()[std::size_t(s)][1];
}
struct Mapped {
    Color c;
    double chromaScale = 1;
};
Mapped gamutMap(Color c, Color luma) {
    double y = detail::clamp(c[0] * luma[0] + c[1] * luma[1] + c[2] * luma[2]), scale = 1;
    for (auto v : c)
        if (v < 0)
            scale = std::min(scale, y / std::max(1e-9, y - v));
        else if (v > 1)
            scale = std::min(scale, (1 - y) / std::max(1e-9, v - y));
    if (scale < 1)
        for (auto &v : c)
            v = y + (v - y) * scale;
    return {c, scale};
}
bool outOfGamut(Color c, double scale) {
    auto [lo, hi] = std::minmax_element(c.begin(), c.end());
    return scale < .995 && *hi - *lo > .02 * std::max(std::abs(*hi), .001) && *hi - *lo > .001 &&
           (*lo < -.001 || (*hi > 1 && *lo < 1));
}
Result<Image> gamut(ImageView in, const GamutWarningOptions &o, bool destination) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (o.proofSpace < Space::Srgb || o.proofSpace > Space::Rec2020)
        return bad();
    if (!o.linearRec2020.empty() && o.linearRec2020.size() != in.pixels.size())
        return std::unexpected(Error{ErrorCode::SizeMismatch,
                                     "Pre-gamut RGB sample count must match the display image"});
    for (auto c : o.linearRec2020)
        for (auto v : c)
            if (!detail::inRange(v, -64, 64))
                return bad();
    return detail::allocated([&] {
        auto out = detail::copy(in);
        auto toProof = conversion(Space::Rec2020, o.proofSpace);
        auto proofToDisplay = conversion(o.proofSpace, Space::Srgb);
        auto srgbToWorking = conversion(Space::Srgb, Space::Rec2020);
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            auto p = in.pixels[i];
            if (p.a == 0)
                continue;
            auto scene = o.linearRec2020.empty()
                             ? develop_detail::multiply(srgbToWorking, Color{detail::linear(p.r),
                                                                             detail::linear(p.g),
                                                                             detail::linear(p.b)})
                             : o.linearRec2020[i];
            auto proof = develop_detail::multiply(toProof, scene);
            auto mapped = gamutMap(proof, luma(o.proofSpace));
            auto display = develop_detail::multiply(proofToDisplay, mapped.c);
            auto displayMapped = gamutMap(display, luma(Space::Srgb));
            bool warn = destination ? outOfGamut(proof, mapped.chromaScale)
                                    : outOfGamut(display, displayMapped.chromaScale);
            if (warn)
                out.pixels[i] = destination ? Rgba{1, 0, 0, p.a} : Rgba{0, .25F, 1, p.a};
        }
        return out;
    });
}
} // namespace
Result<Image> pointColorRangePreview(ImageView in, const PointColorOptions &o) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (!validPoint(o))
        return bad();
    return detail::allocated([&] {
        auto out = detail::copy(in);
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            auto p = in.pixels[i];
            if (p.a == 0)
                continue;
            auto lab = develop_detail::lab(
                {detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)});
            auto weight = membership(lab, o), g = grey(p);
            out.pixels[i] = {float(g + (p.r - g) * weight), float(g + (p.g - g) * weight),
                             float(g + (p.b - g) * weight), p.a};
        }
        return out;
    });
}
Result<Image> maskColorOverlay(ImageView in, MaskView m, const MaskOverlayOptions &o) {
    return mask(in, m, MaskKind::Color, o);
}
Result<Image> maskColorOnBwOverlay(ImageView in, MaskView m, const MaskOverlayOptions &o) {
    return mask(in, m, MaskKind::ColorOnBw, o);
}
Result<Image> maskImageOnBlack(ImageView in, MaskView m) {
    return mask(in, m, MaskKind::ImageOnBlack);
}
Result<Image> maskImageOnWhite(ImageView in, MaskView m) {
    return mask(in, m, MaskKind::ImageOnWhite);
}
Result<Image> maskWhiteOnBlack(ImageView in, MaskView m) {
    return mask(in, m, MaskKind::WhiteOnBlack);
}
Result<Image> destinationGamutWarning(ImageView in, const GamutWarningOptions &o) {
    return gamut(in, o, true);
}
Result<Image> displayGamutWarning(ImageView in, const GamutWarningOptions &o) {
    return gamut(in, o, false);
}
} // namespace iiGraphicAdjust
