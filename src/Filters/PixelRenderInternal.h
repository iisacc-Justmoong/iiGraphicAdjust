#pragma once
#include "DevelopInternal.h"
#include "Internal.h"
#include <array>
#include <numbers>

namespace iiGraphicAdjust::pixelrender {
using Color = std::array<double, 3>;
constexpr double pi = std::numbers::pi;
inline bool validColor(Rgba p) {
    return detail::inRange(p.r, 0, 1) && detail::inRange(p.g, 0, 1) && detail::inRange(p.b, 0, 1) &&
           detail::inRange(p.a, 0, 1);
}
inline Result<void> validate(ImageView in) {
    if (auto r = detail::validate(in); !r)
        return r;
    if (in.width > 100000000 || in.height > 100000000)
        return std::unexpected(
            Error{ErrorCode::InvalidDimensions, "Filter coordinates exceed supported extent"});
    return {};
}
inline auto bad() {
    return std::unexpected(Error{ErrorCode::InvalidParameter,
                                 "Filter parameter is invalid or outside its documented range"});
}
inline double lum(Rgba p) {
    return .299 * p.r + .587 * p.g + .114 * p.b;
}
inline Color rgb(Rgba p) {
    return {p.r, p.g, p.b};
}
inline Rgba color(Color c, double a) {
    return {float(detail::clamp(c[0])), float(detail::clamp(c[1])), float(detail::clamp(c[2])),
            float(detail::clamp(a))};
}
inline Color mix(Color a, Color b, double t) {
    for (int c = 0; c < 3; ++c)
        a[c] += (b[c] - a[c]) * t;
    return a;
}
inline Rgba at(ImageView in, std::int64_t x, std::int64_t y) {
    x = std::clamp<std::int64_t>(x, 0, in.width - 1);
    y = std::clamp<std::int64_t>(y, 0, in.height - 1);
    return in.pixels[std::size_t(y) * in.width + std::size_t(x)];
}
inline Rgba clean(Rgba p) {
    return p.a > 0 ? p : Rgba{0, 0, 0, 0};
}
inline Rgba visibleAt(ImageView in, std::int64_t x, std::int64_t y, Rgba fallback) {
    auto p = at(in, x, y);
    return p.a > 0 ? p : fallback;
}
struct Average {
    Color sums{};
    double alpha = 0, count = 0;
    void add(Rgba p, double weight = 1) {
        for (int c = 0; c < 3; ++c)
            sums[c] += rgb(p)[c] * p.a * weight;
        alpha += p.a * weight;
        count += weight;
    }
    Color rgbMean() const {
        Color c{};
        if (alpha > 0)
            for (int k = 0; k < 3; ++k)
                c[k] = sums[k] / alpha;
        return c;
    }
    Rgba result() const {
        return clean(color(rgbMean(), count > 0 ? alpha / count : 0));
    }
};
inline Rgba sample(ImageView in, double x, double y) {
    x = std::clamp(x, 0., double(in.width - 1));
    y = std::clamp(y, 0., double(in.height - 1));
    auto x0 = std::int64_t(std::floor(x)), y0 = std::int64_t(std::floor(y));
    double ax = x - x0, ay = y - y0;
    Average a;
    a.add(at(in, x0, y0), (1 - ax) * (1 - ay));
    a.add(at(in, x0 + 1, y0), ax * (1 - ay));
    a.add(at(in, x0, y0 + 1), (1 - ax) * ay);
    a.add(at(in, x0 + 1, y0 + 1), ax * ay);
    return a.result();
}
inline double hash(std::int64_t x, std::int64_t y, std::uint32_t c, std::uint32_t seed) {
    std::uint32_t h = std::uint32_t(x) * 0x8da6b343u ^ std::uint32_t(y) * 0xd8163841u ^
                      c * 0xcb1ab31fu ^ seed * 0x9e3779b9u;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return double(h >> 8) / 16777216.;
}
inline double smooth(double a, double b, double x) {
    if (std::abs(b - a) < 1e-12)
        return x >= b ? 1 : 0;
    double t = detail::clamp((x - a) / (b - a));
    return t * t * (3 - 2 * t);
}
inline double noise(double x, double y, std::uint32_t seed) {
    auto ix = std::int64_t(std::floor(x)), iy = std::int64_t(std::floor(y));
    double tx = smooth(0, 1, x - ix), ty = smooth(0, 1, y - iy);
    double a = hash(ix, iy, 0, seed), b = hash(ix + 1, iy, 0, seed), c = hash(ix, iy + 1, 0, seed),
           d = hash(ix + 1, iy + 1, 0, seed);
    return (a + (b - a) * tx) * (1 - ty) + (c + (d - c) * tx) * ty;
}
inline std::pair<double, double> cellPoint(std::int64_t x, std::int64_t y, double s,
                                           std::uint32_t seed) {
    return {(x + .1 + .8 * hash(x, y, 11, seed)) * s, (y + .1 + .8 * hash(x, y, 12, seed)) * s};
}
inline double radians(double d) {
    return d * pi / 180;
}
inline Color norm(Color v) {
    double n = std::hypot(v[0], v[1], v[2]);
    return n > 1e-12 ? Color{v[0] / n, v[1] / n, v[2] / n} : Color{0, 0, 1};
}
inline double dot(Color a, Color b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
inline std::vector<double> blurredVisible(ImageView in, std::vector<double> p, double sigma) {
    std::vector<double> w(p.size());
    for (std::size_t i = 0; i < p.size(); ++i) {
        w[i] = in.pixels[i].a;
        p[i] *= w[i];
    }
    p = develop_detail::gaussianRaw(p, in.width, in.height, sigma);
    w = develop_detail::gaussianRaw(w, in.width, in.height, sigma);
    for (std::size_t i = 0; i < p.size(); ++i)
        p[i] = w[i] > 0 ? p[i] / w[i] : 0;
    return p;
}
} // namespace iiGraphicAdjust::pixelrender
