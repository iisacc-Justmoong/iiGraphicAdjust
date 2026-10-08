// Algorithms adapted from LightCraft; see third_party/lightcraft/LICENSE-MIT.
#include "DevelopInternal.h"
#include <iiGraphicAdjust/DevelopTone.h>

namespace iiGraphicAdjust {
namespace dd = develop_detail;
namespace {
using dd::Matrix;
using dd::Rgb;
constexpr Matrix bradford{
    {{.8951, .2664, -.1614}, {-.7502, 1.7135, .0367}, {.0389, -.0685, 1.0296}}};
std::array<double, 2> locus(double t) {
    const auto kim = [](double k) {
        const double k2 = k * k, k3 = k2 * k;
        const auto x = k <= 4000 ? -.2661239e9 / k3 - .2343589e6 / k2 + .8776956e3 / k + .179910
                                 : -3.0258469e9 / k3 + 2.1070379e6 / k2 + .2226347e3 / k + .240390;
        const auto y =
            k <= 2222   ? -1.1063814 * x * x * x - 1.34811020 * x * x + 2.18555832 * x - .20219683
            : k <= 4000 ? -.9549476 * x * x * x - 1.37418593 * x * x + 2.09137015 * x - .16748867
                        : 3.0817580 * x * x * x - 5.87338670 * x * x + 3.75112997 * x - .37001483;
        const auto d = -2 * x + 12 * y + 3;
        return std::array<double, 2>{4 * x / d, 6 * y / d};
    };
    if (t >= 1667 && t <= 25000)
        return kim(t);
    const auto a = t < 1667 ? 1667. : 24500., b = t < 1667 ? 1717. : 25000.;
    const auto u = kim(a), v = kim(b);
    const auto f = (1e6 / t - 1e6 / a) / (1e6 / b - 1e6 / a);
    return {u[0] + (v[0] - u[0]) * f, u[1] + (v[1] - u[1]) * f};
}
std::array<double, 2> normal(double t) {
    const auto m = 1e6 / t;
    const auto a = locus(1e6 / (m + 1)), b = locus(1e6 / std::max(1., m - 1));
    const auto dx = b[0] - a[0], dy = b[1] - a[1], length = std::max(1e-12, std::hypot(dx, dy));
    const auto sign = dx < 0 ? -1. : 1.;
    return {-dy / length * sign, dx / length * sign};
}
Rgb white(double t, double tint) {
    const auto uv = locus(t), n = normal(t);
    const auto u = uv[0] - n[0] * tint / 3000, v = uv[1] - n[1] * tint / 3000;
    const auto d = 2 * u - 8 * v + 4, x = 3 * u / d, y = 2 * v / d;
    return {x / y, 1, (1 - x - y) / y};
}
Matrix adapt(Rgb source, Rgb destination) {
    const auto a = dd::multiply(bradford, source), b = dd::multiply(bradford, destination);
    Matrix diagonal{};
    for (int i = 0; i < 3; ++i)
        diagonal[i][i] = b[i] / a[i];
    return dd::multiply(dd::multiply(dd::inverse(bradford), diagonal), bradford);
}
bool valid(const WhiteBalanceOptions &o) {
    return detail::inRange(o.temperature, 2000, 50000) &&
           detail::inRange(o.asShotTemperature, 2000, 50000) &&
           detail::inRange(o.tint, -150, 150) && detail::inRange(o.asShotTint, -150, 150);
}
WhiteBalanceOptions estimated(Rgb mean, const WhiteBalanceOptions &source) {
    const auto xyz = dd::multiply(dd::rgbXyz, mean);
    const auto seen = dd::multiply(
        adapt({.95047, 1, 1.08883}, white(source.asShotTemperature, -source.asShotTint)), xyz);
    const auto total = seen[0] + seen[1] + seen[2], x = seen[0] / total, y = seen[1] / total;
    const auto denominator = -2 * x + 12 * y + 3, u = 4 * x / denominator, v = 6 * y / denominator;
    const auto distance = [&](double m) {
        const auto p = locus(1e6 / m);
        return std::pow(u - p[0], 2) + std::pow(v - p[1], 2);
    };
    double best = 20, minimum = distance(best);
    for (double m = 25; m <= 500; m += 5)
        if (distance(m) < minimum) {
            minimum = distance(m);
            best = m;
        }
    double lo = std::max(20., best - 5), hi = std::min(500., best + 5);
    for (int i = 0; i < 60; ++i) {
        const auto a = hi - .61803398875 * (hi - lo), b = lo + .61803398875 * (hi - lo);
        if (distance(a) < distance(b))
            hi = b;
        else
            lo = a;
    }
    const auto t = 1e6 / ((lo + hi) / 2);
    const auto p = locus(t), n = normal(t);
    auto o = source;
    o.temperature = std::clamp(t, 2000., 50000.);
    o.tint = std::clamp(3000 * ((u - p[0]) * n[0] + (v - p[1]) * n[1]), -150., 150.);
    return o;
}
bool valid(const DevelopLightOptions &o) {
    return detail::inRange(o.exposure, -5, 5) && detail::inRange(o.contrast, -100, 100) &&
           detail::inRange(o.highlights, -100, 100) && detail::inRange(o.shadows, -100, 100) &&
           detail::inRange(o.whites, -100, 100) && detail::inRange(o.blacks, -100, 100);
}
bool neutral(const DevelopLightOptions &o) {
    return o.exposure == 0 && o.contrast == 0 && o.highlights == 0 && o.shadows == 0 &&
           o.whites == 0 && o.blacks == 0;
}
bool validCurve(const CurveChannel &c) {
    if (c.points.size() < 2 || c.points.size() > 65536)
        return false;
    double previous = -1;
    for (auto p : c.points) {
        if (!detail::inRange(p.input, 0, 1) || !detail::inRange(p.output, 0, 1) ||
            (previous >= 0 && p.input - previous < 1e-9))
            return false;
        previous = p.input;
    }
    return true;
}
struct Cubic {
    const CurveChannel &c;
    std::vector<double> slope;
    explicit Cubic(const CurveChannel &channel) : c(channel), slope(c.points.size()) {
        const auto n = c.points.size();
        std::vector<double> d(n - 1);
        for (std::size_t i = 0; i + 1 < n; ++i)
            d[i] = (c.points[i + 1].output - c.points[i].output) /
                   (c.points[i + 1].input - c.points[i].input);
        slope.front() = d.front();
        slope.back() = d.back();
        for (std::size_t i = 1; i + 1 < n; ++i)
            slope[i] = d[i - 1] * d[i] <= 0 ? 0 : (d[i - 1] + d[i]) / 2;
        for (std::size_t i = 0; i + 1 < n; ++i) {
            if (d[i] == 0) {
                slope[i] = slope[i + 1] = 0;
                continue;
            }
            const auto a = slope[i] / d[i], b = slope[i + 1] / d[i], r = std::hypot(a, b);
            if (r > 3) {
                slope[i] = 3 / r * a * d[i];
                slope[i + 1] = 3 / r * b * d[i];
            }
        }
    }
    double eval(double x) const {
        if (x <= c.points.front().input)
            return c.points.front().output;
        if (x >= c.points.back().input)
            return c.points.back().output;
        const auto it = std::upper_bound(c.points.begin(), c.points.end(), x,
                                         [](double v, CurvePoint p) { return v < p.input; });
        const auto i = static_cast<std::size_t>(it - c.points.begin() - 1);
        const auto a = c.points[i], b = c.points[i + 1];
        const auto h = b.input - a.input, t = (x - a.input) / h, t2 = t * t, t3 = t2 * t;
        return (2 * t3 - 3 * t2 + 1) * a.output + (t3 - 2 * t2 + t) * h * slope[i] +
               (-2 * t3 + 3 * t2) * b.output + (t3 - t2) * h * slope[i + 1];
    }
};
} // namespace

Result<WhiteBalanceOptions> whiteBalancePreset(WhiteBalancePreset p) {
    switch (p) {
    case WhiteBalancePreset::Daylight:
        return WhiteBalanceOptions{5500, 10};
    case WhiteBalancePreset::Cloudy:
        return WhiteBalanceOptions{6500, 10};
    case WhiteBalancePreset::Shade:
        return WhiteBalanceOptions{7500, 10};
    case WhiteBalancePreset::Tungsten:
        return WhiteBalanceOptions{2850, 0};
    case WhiteBalancePreset::Fluorescent:
        return WhiteBalanceOptions{3800, 21};
    case WhiteBalancePreset::Flash:
        return WhiteBalanceOptions{5500, 0};
    }
    return std::unexpected(dd::invalid("Unknown white balance preset"));
}
Result<WhiteBalanceOptions> sampledWhiteBalance(Rgba p, const WhiteBalanceOptions &source) {
    if (!valid(source) || !detail::inRange(p.r, 0, 1) || !detail::inRange(p.g, 0, 1) ||
        !detail::inRange(p.b, 0, 1) || !detail::inRange(p.a, 0, 1) || p.a == 0 ||
        std::max({p.r, p.g, p.b}) == 0)
        return std::unexpected(
            dd::invalid("WB sampling requires valid settings and a visible nonblack sample"));
    return estimated({detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)}, source);
}
Result<WhiteBalanceOptions> autoWhiteBalance(ImageView image, const WhiteBalanceOptions &source) {
    if (auto check = detail::validate(image); !check)
        return std::unexpected(check.error());
    if (!valid(source))
        return std::unexpected(dd::invalid("WB temperatures/tints outside supported ranges"));
    Rgb sum{};
    double weight = 0;
    for (auto p : image.pixels)
        if (p.a > 0) {
            const Rgb c{detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)};
            const auto y = dd::luminance(c);
            if (y < .01 || y > 2)
                continue;
            const auto mx = std::max({c[0], c[1], c[2]}), mn = std::min({c[0], c[1], c[2]});
            const auto w = p.a * std::pow(1 - (mx - mn) / (mx + 1e-6), 2) *
                           (1 - std::min(1., std::abs((std::log2(y) + 2.5) / 4)));
            for (int k = 0; k < 3; ++k)
                sum[k] += w * c[k];
            weight += w;
        }
    if (weight == 0) {
        auto o = source;
        o.temperature = o.asShotTemperature;
        o.tint = o.asShotTint;
        return o;
    }
    return estimated(dd::scale(sum, 1 / weight), source);
}
Result<Image> whiteBalance(ImageView image, const WhiteBalanceOptions &o) {
    if (auto check = detail::validate(image); !check)
        return std::unexpected(check.error());
    if (!valid(o))
        return std::unexpected(dd::invalid("WB temperatures/tints outside supported ranges"));
    return detail::allocated([&] {
        if (o.temperature == o.asShotTemperature && o.tint == o.asShotTint)
            return detail::copy(image);
        auto f = dd::decode(image);
        // set * inverse(as-shot): adapt from selected illuminant to source white.
        // UI tint is a correction: positive adds magenta, opposite to illuminant Duv.
        const auto matrix = dd::multiply(
            dd::multiply(dd::inverse(dd::rgbXyz), adapt(white(o.temperature, -o.tint),
                                                        white(o.asShotTemperature, -o.asShotTint))),
            dd::rgbXyz);
        const auto gain = 1 / dd::luminance(dd::multiply(matrix, Rgb{1, 1, 1}));
        for (auto &c : f.rgb) {
            c = dd::scale(dd::multiply(matrix, c), gain);
            for (auto &v : c)
                v = std::max(0., v);
        }
        return dd::encode(f, image);
    });
}
Result<DevelopLightOptions> autoDevelopLight(ImageView image) {
    if (auto check = detail::validate(image); !check)
        return std::unexpected(check.error());
    auto result = detail::allocated([&]() -> Result<DevelopLightOptions> {
        std::vector<double> ev;
        for (auto p : image.pixels)
            if (p.a > 0)
                ev.push_back(
                    dd::logLum({detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)}));
        if (ev.empty())
            return std::unexpected(dd::invalid("Auto Light requires visible pixels"));
        std::sort(ev.begin(), ev.end());
        const auto q = [&](double percentile) {
            return ev[static_cast<std::size_t>((ev.size() - 1) * percentile)];
        };
        DevelopLightOptions o;
        o.exposure = std::round(std::clamp(-q(.5) * .85 - .1, -4., 4.) * 100) / 100;
        const auto p01 = q(.01) + o.exposure, p05 = q(.05) + o.exposure, p95 = q(.95) + o.exposure,
                   p995 = q(.995) + o.exposure;
        o.highlights = p995 > 2.2 ? -std::min(90., (p995 - 2.2) * 38) : 0;
        o.shadows = p05 < -4 ? std::min(70., (-4 - p05) * 22) : 0;
        o.contrast = std::clamp((6.5 - p95 + p05) * 6, -20., 30.);
        o.whites = p995 < 1.8 ? std::min(40., (1.8 - p995) * 25)
                              : -std::min(30., std::max(0., p995 - 3.5) * 10);
        o.blacks =
            p01 > -5 ? -std::min(35., (p01 + 5) * 10) : std::min(20., std::max(0., -7 - p01) * 8);
        o.contrast = std::round(o.contrast);
        o.highlights = std::round(o.highlights);
        o.shadows = std::round(o.shadows);
        o.whites = std::round(o.whites);
        o.blacks = std::round(o.blacks);
        return o;
    });
    if (!result)
        return std::unexpected(result.error());
    return *result;
}
Result<Image> developLight(ImageView image, const DevelopLightOptions &o) {
    if (auto check = detail::validate(image); !check)
        return std::unexpected(check.error());
    if (!valid(o))
        return std::unexpected(dd::invalid("Light sliders outside supported ranges"));
    return detail::allocated([&] {
        if (neutral(o))
            return detail::copy(image);
        auto f = dd::decode(image);
        auto log = dd::logs(f), base = log;
        if (o.highlights != 0 || o.shadows != 0)
            base = dd::guided(f, log, std::max(1., .015 * std::max(f.width, f.height)), .35);
        for (std::size_t i = 0; i < f.rgb.size(); ++i)
            if (f.alpha[i] > 0) {
                const auto b = base[i] + o.exposure, ws = 1 - dd::smooth(-4.8, .3, b),
                           wh = dd::smooth(-1, 2.8, b);
                const auto delta =
                    o.shadows / 100 * 1.7 * ws * std::sqrt(ws) + o.highlights / 100 * 1.7 * wh;
                auto c = dd::scale(f.rgb[i], std::exp2(o.exposure + delta));
                const auto y = dd::luminance(c);
                if (o.contrast != 0 || o.whites != 0 || o.blacks != 0) {
                    auto p = std::pow(y, 1 / 2.2);
                    const auto grey = std::pow(.18, 1 / 2.2);
                    if (p <= 1) {
                        p += o.contrast / 100 * .35 * (p - grey) * (1 - std::pow(2 * p - 1, 2));
                        p += o.whites / 100 * .12 * dd::smooth(.45, 1, p) * (1 - p * .5);
                        p += o.blacks / 100 * .07 * (1 - dd::smooth(0, .45, p)) *
                             std::min(1., p * 2);
                    } else
                        p += o.whites / 100 * .06;
                    auto out = std::pow(std::max(0., p), 2.2);
                    if (out > .95) {
                        const auto d = std::min(.1, out - .95);
                        out = .95 + d - d * d / .2;
                    }
                    c = y > 0 ? dd::scale(c, detail::clamp(out) / y) : Rgb{};
                }
                f.rgb[i] = c;
            }
        return dd::encode(f, image);
    });
}
Result<Image> developCurve(ImageView image, const DevelopCurveOptions &o) {
    if (auto check = detail::validate(image); !check)
        return std::unexpected(check.error());
    if (!detail::inRange(o.shadows, -100, 100) || !detail::inRange(o.darks, -100, 100) ||
        !detail::inRange(o.lights, -100, 100) || !detail::inRange(o.highlights, -100, 100) ||
        !detail::inRange(o.refineSaturation, 0, 100) || !detail::inRange(o.splitShadows, 0, 100) ||
        !detail::inRange(o.splitMid, 0, 100) || !detail::inRange(o.splitHighlights, 0, 100) ||
        o.splitShadows <= 0 || o.splitShadows >= o.splitMid || o.splitMid >= o.splitHighlights ||
        o.splitHighlights >= 100 || !validCurve(o.master) || !validCurve(o.red) ||
        !validCurve(o.green) || !validCurve(o.blue))
        return std::unexpected(dd::invalid("Invalid parametric or point curve"));
    return detail::allocated([&] {
        const auto identity = [](const CurveChannel &c) {
            return std::all_of(c.points.begin(), c.points.end(),
                               [](auto p) { return p.input == p.output; }) &&
                   c.points.front().input == 0 && c.points.back().input == 1;
        };
        if (o.shadows == 0 && o.darks == 0 && o.lights == 0 && o.highlights == 0 &&
            identity(o.master) && identity(o.red) && identity(o.green) && identity(o.blue))
            return detail::copy(image);
        Cubic master(o.master), red(o.red), green(o.green), blue(o.blue);
        std::array<const Cubic *, 3> channels{&red, &green, &blue};
        std::array<double, 1024> base{};
        const std::array<double, 5> split{0, o.splitShadows / 100, o.splitMid / 100,
                                          o.splitHighlights / 100, 1};
        const std::array<double, 4> amount{o.shadows, o.darks, o.lights, o.highlights};
        for (std::size_t i = 0; i < base.size(); ++i) {
            const auto x = double(i) / (base.size() - 1);
            double d = 0;
            for (int j = 0; j < 4; ++j) {
                const auto t = std::clamp((x - (split[j] + split[j + 1]) / 2) /
                                              ((split[j + 1] - split[j]) * .75 + .05),
                                          -1., 1.);
                d += amount[j] / 100 * .22 * (.5 + .5 * std::cos(t * std::numbers::pi));
            }
            base[i] = detail::clamp(x + d * 4 * x * (1 - x));
            if (i > 0)
                base[i] = std::max(base[i], base[i - 1]);
        }
        auto out = detail::copy(image);
        for (std::size_t i = 0; i < image.pixels.size(); ++i)
            if (image.pixels[i].a > 0) {
                const auto p = image.pixels[i];
                const Rgb input{p.r, p.g, p.b};
                Rgb c{};
                for (int k = 0; k < 3; ++k) {
                    const auto at = input[k] * (base.size() - 1);
                    const auto j = std::min(base.size() - 2, static_cast<std::size_t>(at));
                    const auto value = base[j] + (base[j + 1] - base[j]) * (at - j);
                    c[k] = detail::clamp(channels[k]->eval(master.eval(value)));
                }
                if (o.refineSaturation < 100) {
                    const auto y0 = dd::luminance(input), y1 = dd::luminance(c);
                    const auto s0 = (std::max({input[0], input[1], input[2]}) -
                                     std::min({input[0], input[1], input[2]})) /
                                    std::max(1e-4, y0);
                    const auto s1 = (std::max({c[0], c[1], c[2]}) - std::min({c[0], c[1], c[2]})) /
                                    std::max(1e-4, y1);
                    if (s0 > 1e-6 && s1 > 1e-6) {
                        const auto k =
                            std::clamp(std::pow(s0 / s1, 1 - o.refineSaturation / 100), 0., 4.);
                        for (auto &v : c)
                            v = detail::clamp(y1 + (v - y1) * k);
                    }
                }
                out.pixels[i] = {static_cast<float>(c[0]), static_cast<float>(c[1]),
                                 static_cast<float>(c[2]), p.a};
            }
        return out;
    });
}
} // namespace iiGraphicAdjust
