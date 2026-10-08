// Algorithms adapted from LightCraft; see third_party/lightcraft/LICENSE-MIT.
#include "DevelopInternal.h"
#include <iiGraphicAdjust/DevelopColor.h>

namespace iiGraphicAdjust {
namespace dd = develop_detail;
namespace {
using dd::Rgb;
double hueOf(double degree) {
    const auto hp = degree / 60, x = 1 - std::abs(std::fmod(hp, 2) - 1);
    const auto sector = static_cast<int>(hp) % 6;
    Rgb c{};
    switch (sector) {
    case 0:
        c = {1, x, 0};
        break;
    case 1:
        c = {x, 1, 0};
        break;
    case 2:
        c = {0, 1, x};
        break;
    case 3:
        c = {0, x, 1};
        break;
    case 4:
        c = {x, 0, 1};
        break;
    default:
        c = {1, 0, x};
    }
    for (auto &v : c)
        v = detail::linear(v);
    const auto l = dd::lab(c);
    return std::atan2(l[2], l[1]);
}
std::array<double, 8> centers() {
    constexpr std::array<double, 8> hsv{0, 30, 60, 120, 180, 225, 270, 315};
    std::array<double, 8> out{};
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = hueOf(hsv[i]);
    return out;
}
std::array<double, 8> weights(double h, const std::array<double, 8> &hues) {
    std::array<double, 8> out{};
    const auto positive = [](double x) {
        if (x < 0)
            x += 2 * std::numbers::pi;
        return x;
    };
    for (int i = 0; i < 8; ++i) {
        const auto span = positive(dd::wrap(hues[(i + 1) % 8] - hues[i])),
                   d = positive(dd::wrap(h - hues[i]));
        if (d <= span) {
            const auto t = .5 - .5 * std::cos(d / span * std::numbers::pi);
            out[i] = 1 - t;
            out[(i + 1) % 8] = t;
            break;
        }
    }
    return out;
}
bool zero(const ColorWheel &w) {
    return w.saturation == 0 && w.luminance == 0;
}
bool neutral(const DevelopColorOptions &o) {
    return o.vibrance == 0 && o.saturation == 0 && !o.blackAndWhite &&
           std::all_of(
               o.mixer.begin(), o.mixer.end(),
               [](auto b) { return b.hue == 0 && b.saturation == 0 && b.luminance == 0; }) &&
           std::all_of(o.points.begin(), o.points.end(),
                       [](auto p) {
                           return p.hueShift == 0 && p.saturationShift == 0 &&
                                  p.luminanceShift == 0 && p.variance == 0;
                       }) &&
           zero(o.grading.shadows) && zero(o.grading.midtones) && zero(o.grading.highlights) &&
           zero(o.grading.global);
}
bool valid(const DevelopColorOptions &o) {
    if (!detail::inRange(o.vibrance, -100, 100) || !detail::inRange(o.saturation, -100, 100) ||
        o.points.size() > 8 || !detail::inRange(o.grading.blending, 0, 100) ||
        !detail::inRange(o.grading.balance, -100, 100))
        return false;
    for (auto b : o.mixer)
        if (!detail::inRange(b.hue, -100, 100) || !detail::inRange(b.saturation, -100, 100) ||
            !detail::inRange(b.luminance, -100, 100))
            return false;
    for (auto b : o.blackAndWhiteMix)
        if (!detail::inRange(b, -100, 100))
            return false;
    for (auto w : {o.grading.shadows, o.grading.midtones, o.grading.highlights, o.grading.global})
        if (!detail::inRange(w.hue, 0, 360) || !detail::inRange(w.saturation, 0, 100) ||
            !detail::inRange(w.luminance, -100, 100))
            return false;
    for (auto p : o.points) {
        if (!detail::inRange(p.lightness, 0, 1) || !detail::inRange(p.chroma, 0, .5) ||
            !detail::inRange(p.hue, -180, 360) || !detail::inRange(p.hueShift, -100, 100) ||
            !detail::inRange(p.saturationShift, -100, 100) ||
            !detail::inRange(p.luminanceShift, -100, 100) ||
            !detail::inRange(p.variance, -100, 100) || !detail::inRange(p.range, 0, 100) ||
            !detail::inRange(p.hueRange, 0, 100) || !detail::inRange(p.saturationRange, 0, 100) ||
            !detail::inRange(p.luminanceRange, 0, 100))
            return false;
    }
    return true;
}
} // namespace
Result<PointColorOptions> samplePointColor(Rgba p) {
    if (!detail::inRange(p.r, 0, 1) || !detail::inRange(p.g, 0, 1) || !detail::inRange(p.b, 0, 1) ||
        !detail::inRange(p.a, 0, 1) || p.a == 0)
        return std::unexpected(
            dd::invalid("Point Color sample must be a visible normalized color"));
    const auto l = dd::lab({detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)});
    PointColorOptions o;
    o.lightness = detail::clamp(l[0]);
    o.chroma = std::hypot(l[1], l[2]);
    o.hue = std::atan2(l[2], l[1]) * 180 / std::numbers::pi;
    return o;
}
Result<std::array<double, 8>> autoBlackAndWhiteMix(ImageView image) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    const auto hues = centers();
    std::array<double, 8> mass{}, sum{};
    double all = 0, count = 0;
    for (auto p : image.pixels)
        if (p.a > 0) {
            const auto l = dd::lab({detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)});
            const auto w = weights(std::atan2(l[2], l[1]), hues);
            const auto k = std::min(1., std::hypot(l[1], l[2]) / .2);
            all += l[0] * p.a;
            count += p.a;
            if (k < .05)
                continue;
            for (int i = 0; i < 8; ++i) {
                mass[i] += w[i] * k * p.a;
                sum[i] += w[i] * k * p.a * l[0];
            }
        }
    if (count == 0)
        return std::unexpected(dd::invalid("Auto B&W requires visible pixels"));
    double total = 0;
    for (auto v : mass)
        total += v;
    std::array<double, 8> out{};
    for (int i = 0; i < 8; ++i)
        if (total > 0 && mass[i] / total >= .01)
            out[i] = std::round(std::clamp((sum[i] / mass[i] - all / count) * 400, -60., 60.));
    return out;
}
Result<Image> developColor(ImageView image, const DevelopColorOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (!valid(o))
        return std::unexpected(dd::invalid("Invalid development color settings"));
    return detail::allocated([&] {
        if (neutral(o))
            return detail::copy(image);
        auto f = dd::decode(image);
        const auto hues = centers();
        const auto skin = hueOf(25);
        const std::array<ColorWheel, 4> wheels{o.grading.shadows, o.grading.midtones,
                                               o.grading.highlights, o.grading.global};
        std::array<Rgb, 4> offsets{};
        for (int i = 0; i < 4; ++i) {
            const auto h = hueOf(wheels[i].hue);
            offsets[i] = {wheels[i].luminance / 100 * .12,
                          wheels[i].saturation / 100 * .09 * std::cos(h),
                          wheels[i].saturation / 100 * .09 * std::sin(h)};
        }
        for (std::size_t i = 0; i < f.rgb.size(); ++i)
            if (f.alpha[i] > 0) {
                auto lch = dd::lab(f.rgb[i]);
                double l = lch[0], c = std::hypot(lch[1], lch[2]), h = std::atan2(lch[2], lch[1]);
                if (c < 1e-7)
                    c = 0; // Hue is meaningless on the neutral axis.
                const auto w = weights(h, hues);
                double dh = 0, ds = 0, dl = 0;
                for (int j = 0; j < 8; ++j) {
                    dh += w[j] * o.mixer[j].hue / 100 * .5;
                    ds += w[j] * o.mixer[j].saturation / 100;
                    dl += w[j] * o.mixer[j].luminance / 100 * .18;
                }
                const auto cw = std::min(1., c / .12);
                h += dh * cw;
                c *= std::max(0., 1 + ds);
                l += dl * cw * std::sqrt(std::max(.05, l));
                if (!o.blackAndWhite)
                    for (auto p : o.points) {
                        const auto k = std::max(.05, p.range / 50),
                                   wh = (.1 + .6 * p.hueRange / 100) * k,
                                   wc = (.02 + .12 * p.saturationRange / 100) * k,
                                   wl = (.05 + .4 * p.luminanceRange / 100) * k,
                                   ph = p.hue / 180 * std::numbers::pi;
                        const auto membership =
                            (p.chroma < .02
                                 ? 1
                                 : (1 - dd::smooth(.5 * wh, wh, std::abs(dd::wrap(h - ph)))) *
                                       dd::smooth(.005, .025, c)) *
                            (1 - dd::smooth(.5 * wc, wc, std::abs(c - p.chroma))) *
                            (1 - dd::smooth(.5 * wl, wl, std::abs(l - p.lightness)));
                        h += membership *
                             (p.variance / 100 * .8 * dd::wrap(h - ph) + p.hueShift / 100 * .5);
                        c += membership * p.variance / 100 * .8 * (c - p.chroma);
                        c = std::max(0., c * (1 + membership * p.saturationShift / 100));
                        l += membership * (p.variance / 100 * .8 * (l - p.lightness) +
                                           p.luminanceShift / 100 * .2);
                    }
                const auto low = 1 - detail::clamp(c / .22),
                           protect = o.vibrance > 0
                                         ? 1 - .6 * std::exp(-std::pow(dd::wrap(h - skin) / .35, 2))
                                         : 1;
                c *= std::max(0., 1 + o.vibrance / 100 * low * low * protect * 1.2) *
                     std::max(0., 1 + o.saturation / 100);
                if (o.blackAndWhite) {
                    const auto bw = weights(h, hues);
                    double mix = 0;
                    for (int j = 0; j < 8; ++j)
                        mix += bw[j] * o.blackAndWhiteMix[j] / 100;
                    l = std::max(0., l + mix * std::min(1., c / .2) * .25);
                    c = 0;
                }
                Rgb lab{l, c * std::cos(h), c * std::sin(h)};
                const auto mid = .5 - o.grading.balance / 100 * .25,
                           width = .15 + o.grading.blending / 100 * .5,
                           ws = 1 - dd::smooth(mid - width, mid + width * .25, l),
                           wh = dd::smooth(mid - width * .25, mid + width, l);
                const std::array<double, 4> grade{ws, std::max(0., 1 - ws - wh), wh, 1};
                for (int j = 0; j < 4; ++j)
                    for (int k = 0; k < 3; ++k)
                        lab[k] += offsets[j][k] * grade[j];
                f.rgb[i] = dd::fromLab(lab);
            }
        return dd::encode(f, image);
    });
}
Result<Image> calibrate(ImageView image, const CalibrationOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (!detail::inRange(o.shadowsTint, -100, 100))
        return std::unexpected(dd::invalid("Invalid calibration tint"));
    bool active = false;
    for (int i = 0; i < 3; ++i) {
        if (!detail::inRange(o.primaryHue[i], -100, 100) ||
            !detail::inRange(o.primarySaturation[i], -100, 100))
            return std::unexpected(dd::invalid("Invalid calibration primary"));
        active = active || o.primaryHue[i] != 0 || o.primarySaturation[i] != 0;
    }
    dd::Matrix matrix{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    if (active) {
        for (int i = 0; i < 3; ++i) {
            Rgb p{};
            p[i] = 1;
            const auto lab = dd::lab(p);
            const auto h = std::atan2(lab[2], lab[1]) + o.primaryHue[i] / 100 * .5,
                       c = std::hypot(lab[1], lab[2]) * (1 + o.primarySaturation[i] / 100 * .6);
            const auto q = dd::fromLab({lab[0], c * std::cos(h), c * std::sin(h)});
            for (int j = 0; j < 3; ++j)
                matrix[j][i] = q[j];
        }
        const auto k = dd::multiply(dd::inverse(matrix), Rgb{1, 1, 1});
        for (auto v : k)
            if (!std::isfinite(v) || std::abs(v) > 1e6)
                return std::unexpected(
                    dd::invalid("Calibration primaries produce a singular matrix"));
        for (int r = 0; r < 3; ++r)
            for (int col = 0; col < 3; ++col)
                matrix[r][col] *= k[col];
    }
    return detail::allocated([&] {
        if (!active && o.shadowsTint == 0)
            return detail::copy(image);
        auto f = dd::decode(image);
        for (auto &c : f.rgb) {
            c = dd::multiply(matrix, c);
            for (auto &v : c)
                v = std::max(0., v);
            if (o.shadowsTint != 0) {
                const auto y = dd::luminance(c), w = 1 - dd::smooth(-5, -.5, dd::logLum(c));
                c[1] *= 1 - .3 * o.shadowsTint / 100 * w;
                const auto after = dd::luminance(c);
                if (after > 0)
                    c = dd::scale(c, y / after);
            }
        }
        return dd::encode(f, image);
    });
}
} // namespace iiGraphicAdjust
