#pragma once
#include "Internal.h"
#include <array>
#include <deque>
#include <numbers>

namespace iiGraphicAdjust::develop_detail {
using Rgb = std::array<double, 3>;
using Matrix = std::array<Rgb, 3>;
using Plane = std::vector<double>;
struct Frame {
    std::size_t width, height;
    std::vector<Rgb> rgb;
    Plane alpha;
};
inline double smooth(double a, double b, double x) {
    const auto t = detail::clamp((x - a) / (b - a));
    return t * t * (3 - 2 * t);
}
inline double luminance(Rgb c) {
    return .2126 * c[0] + .7152 * c[1] + .0722 * c[2];
}
inline double logLum(Rgb c) {
    return std::log2(std::max(1e-7, luminance(c)) / .18);
}
inline Rgb scale(Rgb c, double k) {
    for (auto &v : c)
        v *= k;
    return c;
}
inline Rgb multiply(const Matrix &m, Rgb v) {
    Rgb out{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i] += m[i][j] * v[j];
    return out;
}
inline Matrix multiply(const Matrix &a, const Matrix &b) {
    Matrix out{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                out[i][j] += a[i][k] * b[k][j];
    return out;
}
inline Matrix inverse(const Matrix &m) {
    const double d = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                     m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                     m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    Matrix out{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[j][i] = (m[(i + 1) % 3][(j + 1) % 3] * m[(i + 2) % 3][(j + 2) % 3] -
                         m[(i + 1) % 3][(j + 2) % 3] * m[(i + 2) % 3][(j + 1) % 3]) /
                        d;
    return out;
}
inline constexpr Matrix rgbXyz{{{.4124564, .3575761, .1804375},
                                {.2126729, .7151522, .0721750},
                                {.0193339, .1191920, .9503041}}};
inline Frame decode(ImageView image) {
    Frame f{image.width, image.height, {}, {}};
    f.rgb.reserve(image.pixels.size());
    f.alpha.reserve(image.pixels.size());
    for (auto p : image.pixels) {
        f.rgb.push_back({detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)});
        f.alpha.push_back(p.a);
    }
    return f;
}
// Preserve hue by reducing chroma towards luminance before encoding into sRGB.
inline Rgb gamut(Rgb c) {
    const auto y = detail::clamp(luminance(c));
    double k = 1;
    for (auto v : c) {
        const auto d = v - y;
        if (d > 0)
            k = std::min(k, (1 - y) / d);
        else if (d < 0)
            k = std::min(k, -y / d);
    }
    if (k == 1)
        return c;
    for (auto &v : c)
        v = detail::clamp(y + (v - y) * k);
    return c;
}
inline Image encode(const Frame &f, ImageView original) {
    auto out = detail::copy(original);
    for (std::size_t i = 0; i < f.rgb.size(); ++i)
        if (f.alpha[i] > 0) {
            const auto c = gamut(f.rgb[i]);
            out.pixels[i] = {detail::encoded(c[0]), detail::encoded(c[1]), detail::encoded(c[2]),
                             original.pixels[i].a};
        }
    return out;
}
// Gaussian windows; alpha-normalized statistics exclude hidden RGB.
inline Plane gaussianRaw(const Plane &p, std::size_t w, std::size_t h, double sigma) {
    if (sigma <= 0)
        return p;
    if (sigma >= 3) {
        // Three variance-matched box passes approximate Gaussian windows in O(N).
        auto lower = static_cast<std::size_t>(std::floor(std::sqrt(4 * sigma * sigma + 1)));
        if (lower % 2 == 0)
            --lower;
        lower = std::max(std::size_t{1}, lower);
        const auto upper = lower + 2;
        const auto count = std::clamp(
            std::lround((12 * sigma * sigma - 3 * double(lower) * lower - 12 * lower - 9) /
                        (-4 * double(lower) - 4)),
            0L, 3L);
        Plane src = p, dst(p.size());
        for (int pass = 0; pass < 3; ++pass)
            for (int axis = 0; axis < 2; ++axis) {
                const auto radius = std::min((pass < count ? lower : upper) / 2, std::max(w, h));
                const auto length = axis == 0 ? w : h, lines = axis == 0 ? h : w;
                for (std::size_t line = 0; line < lines; ++line) {
                    const auto at = [&](std::size_t x) {
                        return axis == 0 ? line * w + x : x * w + line;
                    };
                    double sum = double(radius + 1) * src[at(0)];
                    for (std::size_t k = 1; k <= radius; ++k)
                        sum += src[at(std::min(k, length - 1))];
                    for (std::size_t x = 0; x < length; ++x) {
                        dst[at(x)] = sum / double(2 * radius + 1);
                        const auto leave = x < radius ? 0 : x - radius,
                                   enter = radius >= length - 1 - x ? length - 1 : x + radius + 1;
                        sum += src[at(enter)] - src[at(leave)];
                    }
                }
                src.swap(dst);
            }
        return src;
    }
    const auto radius =
        static_cast<std::size_t>(std::ceil(std::min(3 * sigma, double(std::max(w, h)))));
    Plane kernel(radius + 1), tmp(p.size()), out(p.size());
    for (std::size_t k = 0; k <= radius; ++k)
        kernel[k] = std::exp(-.5 * std::pow(double(k) / sigma, 2));
    for (int axis = 0; axis < 2; ++axis) {
        const auto &src = axis == 0 ? p : tmp;
        auto &dst = axis == 0 ? tmp : out;
        const auto length = axis == 0 ? w : h;
        for (std::size_t y = 0; y < h; ++y)
            for (std::size_t x = 0; x < w; ++x) {
                const auto at = axis == 0 ? x : y;
                double sum = src[y * w + x], weight = 1;
                for (std::size_t k = 1; k <= radius; ++k) {
                    const auto lo = k > at ? 0 : at - k,
                               hi = k > length - 1 - at ? length - 1 : at + k;
                    const auto a = axis == 0 ? y * w + lo : lo * w + x,
                               b = axis == 0 ? y * w + hi : hi * w + x;
                    sum += (src[a] + src[b]) * kernel[k];
                    weight += 2 * kernel[k];
                }
                dst[y * w + x] = sum / weight;
            }
    }
    return out;
}
inline Plane gaussian(const Frame &f, const Plane &p, double sigma) {
    auto weighted = p;
    for (std::size_t i = 0; i < p.size(); ++i)
        weighted[i] *= f.alpha[i];
    auto out = gaussianRaw(weighted, f.width, f.height, sigma);
    auto mass = gaussianRaw(f.alpha, f.width, f.height, sigma);
    for (std::size_t i = 0; i < p.size(); ++i)
        out[i] = mass[i] > 0 ? out[i] / mass[i] : p[i];
    return out;
}
inline Plane guided(const Frame &f, const Plane &p, double sigma, double epsilon) {
    auto m = gaussian(f, p, sigma), square = p;
    for (auto &v : square)
        v *= v;
    auto corr = gaussian(f, square, sigma), a = p, b = p;
    for (std::size_t i = 0; i < p.size(); ++i) {
        const auto var = std::max(0., corr[i] - m[i] * m[i]);
        a[i] = var / (var + epsilon);
        b[i] = m[i] * (1 - a[i]);
    }
    a = gaussian(f, a, sigma);
    b = gaussian(f, b, sigma);
    for (std::size_t i = 0; i < p.size(); ++i)
        a[i] = a[i] * p[i] + b[i];
    return a;
}
inline Plane logs(const Frame &f) {
    Plane p;
    p.reserve(f.rgb.size());
    for (auto c : f.rgb)
        p.push_back(logLum(c));
    return p;
}
inline Plane extrema(const Frame &f, const Plane &p, std::size_t radius, bool maximum) {
    Plane src = p, dst(p.size());
    for (std::size_t i = 0; i < p.size(); ++i)
        if (f.alpha[i] == 0)
            src[i] = maximum ? -std::numeric_limits<double>::infinity()
                             : std::numeric_limits<double>::infinity();
    for (int axis = 0; axis < 2; ++axis) {
        const auto length = axis == 0 ? f.width : f.height, lines = axis == 0 ? f.height : f.width;
        for (std::size_t line = 0; line < lines; ++line) {
            const auto at = [&](std::size_t x) {
                return axis == 0 ? line * f.width + x : x * f.width + line;
            };
            std::deque<std::size_t> queue;
            std::size_t right = 0;
            for (std::size_t x = 0; x < length; ++x) {
                const auto end = radius >= length - 1 - x ? length - 1 : x + radius;
                while (right <= end) {
                    while (!queue.empty() && (maximum ? src[at(queue.back())] <= src[at(right)]
                                                      : src[at(queue.back())] >= src[at(right)]))
                        queue.pop_back();
                    queue.push_back(right++);
                }
                const auto left = x > radius ? x - radius : 0;
                while (queue.front() < left)
                    queue.pop_front();
                dst[at(x)] = src[at(queue.front())];
            }
        }
        src.swap(dst);
    }
    return src;
}
inline Rgb lab(Rgb c) {
    const auto l = std::cbrt(.4122214708 * c[0] + .5363325363 * c[1] + .0514459929 * c[2]);
    const auto m = std::cbrt(.2119034982 * c[0] + .6806995451 * c[1] + .1073969566 * c[2]);
    const auto s = std::cbrt(.0883024619 * c[0] + .2817188376 * c[1] + .6299787005 * c[2]);
    return {.2104542553 * l + .793617785 * m - .0040720468 * s,
            1.9779984951 * l - 2.428592205 * m + .4505937099 * s,
            .0259040371 * l + .7827717662 * m - .808675766 * s};
}
inline Rgb fromLab(Rgb c) {
    const auto l = std::pow(c[0] + .3963377774 * c[1] + .2158037573 * c[2], 3),
               m = std::pow(c[0] - .1055613458 * c[1] - .0638541728 * c[2], 3),
               s = std::pow(c[0] - .0894841775 * c[1] - 1.291485548 * c[2], 3);
    return {4.0767416621 * l - 3.3077115913 * m + .2309699292 * s,
            -1.2684380046 * l + 2.6097574011 * m - .3413193965 * s,
            -.0041960863 * l - .7034186147 * m + 1.707614701 * s};
}
inline double wrap(double h) {
    return h - 2 * std::numbers::pi * std::floor((h + std::numbers::pi) / (2 * std::numbers::pi));
}
inline Error invalid(const char *message) {
    return {ErrorCode::InvalidParameter, message};
}
} // namespace iiGraphicAdjust::develop_detail
