// Filter Gallery formulas adapted from Photocraft (MIT); see third_party/photocraft.
#include "Internal.h"
#include <array>
#include <cstdint>
#include <iiGraphicAdjust/GalleryFilters.h>
#include <numbers>
#include <tuple>
#include <type_traits>

namespace iiGraphicAdjust {
namespace {
using Px = std::array<double, 3>;
using Plane = std::vector<double>;
using Direction = std::pair<int, int>;
constexpr double pi = std::numbers::pi;
double clamp01(double v) {
    return detail::clamp(v);
}
double lum(Px p) {
    return .299 * p[0] + .587 * p[1] + .114 * p[2];
}
Px mix(Px a, Px b, double t) {
    for (int c = 0; c < 3; ++c)
        a[c] += (b[c] - a[c]) * t;
    return a;
}
template <class F> Px map(Px p, F f) {
    for (auto &v : p)
        v = f(v);
    return p;
}
Px grey(double v) {
    return {v, v, v};
}
double smoothstep(double a, double b, double x) {
    if (std::abs(b - a) < 1e-9)
        return x >= b ? 1 : 0;
    const auto t = clamp01((x - a) / (b - a));
    return t * t * (3 - 2 * t);
}
double mod(double x, double period) {
    return x - period * std::floor(x / period);
}
double posterize(double v, double levels) {
    const auto l = std::max(1., levels - 1);
    return std::round(clamp01(v) * l) / l;
}
Px saturate(Px p, double k) {
    const auto l = lum(p);
    return map(p, [&](double v) { return l + (v - l) * k; });
}
bool validColor(GalleryColor c) {
    return std::all_of(c.begin(), c.end(), [](double v) { return detail::inRange(v, 0, 1); });
}
// Unsigned wrapping arithmetic reproduces Photocraft's coordinate hash without signed overflow.
double hash01(std::int64_t x, std::int64_t y, std::uint32_t channel, std::uint32_t seed) {
    auto h = std::uint32_t(x) * 0x8da6b343u ^ std::uint32_t(y) * 0xd8163841u ^
             channel * 0xcb1ab31fu ^ seed * 0x9e3779b9u;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return double(h >> 8) / 16777216.;
}
double vnoise(double x, double y, std::uint32_t seed) {
    const auto ix = std::int64_t(std::floor(x)), iy = std::int64_t(std::floor(y));
    const auto tx = x - double(ix), ty = y - double(iy), sx = tx * tx * (3 - 2 * tx),
               sy = ty * ty * (3 - 2 * ty);
    const auto top =
        hash01(ix, iy, 0, seed) + (hash01(ix + 1, iy, 0, seed) - hash01(ix, iy, 0, seed)) * sx;
    const auto bot = hash01(ix, iy + 1, 0, seed) +
                     (hash01(ix + 1, iy + 1, 0, seed) - hash01(ix, iy + 1, 0, seed)) * sx;
    return top + (bot - top) * sy;
}
double fbm(double x, double y, std::uint32_t seed) {
    return .57 * vnoise(x, y, seed) + .29 * vnoise(x * 2.03, y * 2.03, seed ^ 0x51) +
           .14 * vnoise(x * 4.1, y * 4.1, seed ^ 0xa3);
}
double strokes(std::int64_t x, std::int64_t y, Direction d, double len, double wid,
               std::uint32_t seed) {
    const auto l = std::hypot(double(d.first), double(d.second)), ux = d.first / l,
               uy = d.second / l;
    return vnoise((x * ux + y * uy) / std::max(.5, len), (-x * uy + y * ux) / std::max(.3, wid),
                  seed);
}
struct Win {
    std::size_t w, h;
    std::vector<Px> c;
    Plane alpha;
    explicit Win(ImageView image) : w(image.width), h(image.height) {
        c.reserve(image.pixels.size());
        alpha.reserve(image.pixels.size());
        for (auto p : image.pixels) {
            c.push_back({p.r, p.g, p.b});
            alpha.push_back(p.a);
        }
    }
    std::pair<std::int64_t, std::int64_t> xy(std::size_t i) const {
        return {std::int64_t(i % w), std::int64_t(i / w)};
    }
    Plane luminance() const {
        Plane l;
        l.reserve(c.size());
        for (auto p : c)
            l.push_back(lum(p));
        return l;
    }
    std::size_t index(std::int64_t x, std::int64_t y) const {
        const auto xx = std::clamp(x, std::int64_t{0}, std::int64_t(w - 1)),
                   yy = std::clamp(y, std::int64_t{0}, std::int64_t(h - 1));
        return std::size_t(yy) * w + std::size_t(xx);
    }
    // Fallback is the destination's visible color, rather than any zero-alpha sample.
    Px at(std::int64_t x, std::int64_t y, Px fallback) const {
        const auto i = index(x, y);
        return alpha[i] > 0 ? c[i] : fallback;
    }
    Px sample(double x, double y, Px fallback) const {
        const auto fx = std::clamp(x - .5, 0., double(w - 1)),
                   fy = std::clamp(y - .5, 0., double(h - 1));
        const auto x0 = std::size_t(std::floor(fx)), y0 = std::size_t(std::floor(fy)),
                   x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
        const auto ax = fx - x0, ay = fy - y0;
        const std::array<std::size_t, 4> indices{y0 * w + x0, y0 * w + x1, y1 * w + x0,
                                                 y1 * w + x1};
        const std::array<double, 4> weights{(1 - ax) * (1 - ay), ax * (1 - ay), (1 - ax) * ay,
                                            ax * ay};
        Px out{};
        double mass = 0;
        for (int k = 0; k < 4; ++k) {
            const auto weight = weights[k] * alpha[indices[k]];
            mass += weight;
            for (int ch = 0; ch < 3; ++ch)
                out[ch] += c[indices[k]][ch] * weight;
        }
        if (mass <= 0)
            return fallback;
        return map(out, [&](double v) { return v / mass; });
    }
};
void axisBox(Plane &p, std::size_t w, std::size_t h, std::size_t radius, int axis) {
    if (radius == 0)
        return;
    Plane out(p.size());
    const auto len = axis == 0 ? w : h, lines = axis == 0 ? h : w;
    for (std::size_t line = 0; line < lines; ++line) {
        const auto at = [&](std::size_t x) { return axis == 0 ? line * w + x : x * w + line; };
        double sum = double(radius + 1) * p[at(0)];
        for (std::size_t k = 1; k <= radius; ++k)
            sum += p[at(std::min(k, len - 1))];
        for (std::size_t x = 0; x < len; ++x) {
            out[at(x)] = sum / double(2 * radius + 1);
            sum += p[at(std::min(x + radius + 1, len - 1))] - p[at(x < radius ? 0 : x - radius)];
        }
    }
    p.swap(out);
}
std::array<std::size_t, 3> boxRadii(double sigma) {
    auto lower = std::max(1, int(std::floor(std::sqrt(4 * sigma * sigma + 1))));
    if (lower % 2 == 0)
        --lower;
    const auto count = std::clamp(
        std::lround((12 * sigma * sigma - 3 * lower * lower - 12 * lower - 9) / (-4. * lower - 4)),
        0L, 3L);
    std::array<std::size_t, 3> r{};
    for (int k = 0; k < 3; ++k)
        r[k] = std::size_t((k < count ? lower - 1 : lower + 1) / 2);
    if (r == std::array<std::size_t, 3>{})
        r[0] = 1;
    return r;
}
// All box passes operate on numerator and mass before division, preserving alpha weighting.
void blur1(Plane &p, const Win &win, double sigma) {
    if (sigma < .3)
        return;
    auto mass = win.alpha;
    auto weighted = p;
    for (std::size_t i = 0; i < p.size(); ++i)
        weighted[i] *= mass[i];
    const auto radii = boxRadii(sigma);
    for (int axis = 0; axis < 2; ++axis)
        for (auto r : radii) {
            axisBox(weighted, win.w, win.h, r, axis);
            axisBox(mass, win.w, win.h, r, axis);
        }
    for (std::size_t i = 0; i < p.size(); ++i)
        p[i] = mass[i] > 0 ? weighted[i] / mass[i] : p[i];
}
void box1(Plane &p, const Win &win, std::size_t radius) {
    auto mass = win.alpha;
    auto weighted = p;
    for (std::size_t i = 0; i < p.size(); ++i)
        weighted[i] *= mass[i];
    for (int axis = 0; axis < 2; ++axis) {
        axisBox(weighted, win.w, win.h, radius, axis);
        axisBox(mass, win.w, win.h, radius, axis);
    }
    for (std::size_t i = 0; i < p.size(); ++i)
        p[i] = mass[i] > 0 ? weighted[i] / mass[i] : p[i];
}
void blur3(std::vector<Px> &c, const Win &win, double sigma) {
    if (sigma < .3)
        return;
    for (int ch = 0; ch < 3; ++ch) {
        Plane p(c.size());
        for (std::size_t i = 0; i < p.size(); ++i)
            p[i] = c[i][ch];
        blur1(p, win, sigma);
        for (std::size_t i = 0; i < p.size(); ++i)
            c[i][ch] = p[i];
    }
}
std::pair<Plane, Plane> sobel(const Plane &p, const Win &win) {
    Plane gx(p.size()), gy(p.size());
    for (std::size_t i = 0; i < p.size(); ++i) {
        const auto [x, y] = win.xy(i);
        const auto at = [&](std::int64_t xx, std::int64_t yy) {
            const auto j = win.index(xx, yy);
            return win.alpha[j] > 0 ? p[j] : p[i];
        };
        gx[i] = ((at(x + 1, y - 1) + 2 * at(x + 1, y) + at(x + 1, y + 1)) -
                 (at(x - 1, y - 1) + 2 * at(x - 1, y) + at(x - 1, y + 1))) /
                8;
        gy[i] = ((at(x - 1, y + 1) + 2 * at(x, y + 1) + at(x + 1, y + 1)) -
                 (at(x - 1, y - 1) + 2 * at(x, y - 1) + at(x + 1, y - 1))) /
                8;
    }
    return {std::move(gx), std::move(gy)};
}
Plane gradMag(const Plane &p, const Win &win) {
    auto [gx, gy] = sobel(p, win);
    for (std::size_t i = 0; i < p.size(); ++i)
        gx[i] = std::hypot(gx[i], gy[i]);
    return gx;
}
void lineBlur(Plane &p, const Win &win, Direction d, std::size_t radius) {
    if (radius == 0)
        return;
    const auto inside = [&](std::int64_t x, std::int64_t y) {
        return x >= 0 && y >= 0 && x < std::int64_t(win.w) && y < std::int64_t(win.h);
    };
    auto out = p;
    std::vector<std::size_t> indices;
    Plane sum, mass;
    for (std::int64_t sy = 0; sy < std::int64_t(win.h); ++sy)
        for (std::int64_t sx = 0; sx < std::int64_t(win.w); ++sx) {
            if (inside(sx - d.first, sy - d.second))
                continue;
            indices.clear();
            for (auto x = sx, y = sy; inside(x, y); x += d.first, y += d.second)
                indices.push_back(std::size_t(y) * win.w + std::size_t(x));
            sum.assign(indices.size() + 1, 0);
            mass.assign(indices.size() + 1, 0);
            for (std::size_t k = 0; k < indices.size(); ++k) {
                sum[k + 1] = sum[k] + p[indices[k]] * win.alpha[indices[k]];
                mass[k + 1] = mass[k] + win.alpha[indices[k]];
            }
            for (std::size_t k = 0; k < indices.size(); ++k) {
                const auto lo = k > radius ? k - radius : 0,
                           hi = std::min(k + radius + 1, indices.size());
                double s = sum[hi] - sum[lo], a = mass[hi] - mass[lo];
                if (k < radius) {
                    s += double(radius - k) * p[indices.front()] * win.alpha[indices.front()];
                    a += double(radius - k) * win.alpha[indices.front()];
                }
                if (k + radius >= indices.size()) {
                    const auto extra = k + radius + 1 - indices.size();
                    s += double(extra) * p[indices.back()] * win.alpha[indices.back()];
                    a += double(extra) * win.alpha[indices.back()];
                }
                out[indices[k]] = a > 0 ? s / a : p[indices[k]];
            }
        }
    p.swap(out);
}
void lineBlur3(std::vector<Px> &c, const Win &win, Direction d, std::size_t r) {
    for (int ch = 0; ch < 3; ++ch) {
        Plane p(c.size());
        for (std::size_t i = 0; i < p.size(); ++i)
            p[i] = c[i][ch];
        lineBlur(p, win, d, r);
        for (std::size_t i = 0; i < p.size(); ++i)
            c[i][ch] = p[i];
    }
}
Direction direction(GalleryStrokeDirection d) {
    switch (d) {
    case GalleryStrokeDirection::Horizontal:
        return {1, 0};
    case GalleryStrokeDirection::Vertical:
        return {0, 1};
    case GalleryStrokeDirection::LeftDiagonal:
        return {1, 1};
    default:
        return {1, -1};
    }
}
void kuwahara(Win &win, std::size_t radius) {
    const auto half = std::max(std::size_t{1}, (radius + 1) / 2);
    auto colors = win.c;
    std::array<Plane, 5> planes;
    for (auto &p : planes)
        p.resize(colors.size());
    for (std::size_t i = 0; i < colors.size(); ++i) {
        for (int ch = 0; ch < 3; ++ch)
            planes[ch][i] = colors[i][ch];
        planes[3][i] = lum(colors[i]);
        planes[4][i] = planes[3][i] * planes[3][i];
    }
    for (auto &p : planes)
        box1(p, win, half);
    auto available = win.alpha;
    axisBox(available, win.w, win.h, half, 0);
    axisBox(available, win.w, win.h, half, 1);
    for (std::size_t i = 0; i < colors.size(); ++i) {
        const auto [x, y] = win.xy(i);
        double best = std::numeric_limits<double>::infinity();
        for (auto [dx, dy] : std::array<Direction, 4>{{{-1, -1}, {1, -1}, {-1, 1}, {1, 1}}}) {
            const auto j = win.index(x + dx * std::int64_t(half), y + dy * std::int64_t(half));
            if (available[j] <= 0)
                continue;
            const auto variance = std::max(0., planes[4][j] - planes[3][j] * planes[3][j]);
            if (variance < best) {
                best = variance;
                colors[i] = {planes[0][j], planes[1][j], planes[2][j]};
            }
        }
    }
    win.c.swap(colors);
}
void unsharp(Win &win, double sigma, double amount) {
    if (amount <= 0)
        return;
    auto blurred = win.c;
    blur3(blurred, win, sigma);
    for (std::size_t i = 0; i < win.c.size(); ++i)
        for (int ch = 0; ch < 3; ++ch)
            win.c[i][ch] += (win.c[i][ch] - blurred[i][ch]) * amount;
}
std::pair<Px, double> paintStroke(const Win &src, std::int64_t x, std::int64_t y, Direction d,
                                  double len, double wid, std::uint32_t seed, Px fallback) {
    const auto l = std::hypot(double(d.first), double(d.second)), ux = d.first / l,
               uy = d.second / l;
    len = std::max(1., len);
    wid = std::max(.5, wid);
    const auto fx = x + .5, fy = y + .5, u = fx * ux + fy * uy, v = -fx * uy + fy * ux;
    const auto lane = std::int64_t(std::floor(v / wid));
    const auto offset = hash01(lane, 0, 0, seed) * len, t = mod((u + offset) / len, 1),
               du = (.5 - t) * len;
    return {src.sample(fx + du * ux, fy + du * uy, fallback), t};
}
Direction integerLight(GalleryLightDirection d) {
    switch (d) {
    case GalleryLightDirection::Bottom:
        return {0, 1};
    case GalleryLightDirection::BottomLeft:
        return {-1, 1};
    case GalleryLightDirection::Left:
        return {-1, 0};
    case GalleryLightDirection::TopLeft:
        return {-1, -1};
    case GalleryLightDirection::Top:
        return {0, -1};
    case GalleryLightDirection::TopRight:
        return {1, -1};
    case GalleryLightDirection::Right:
        return {1, 0};
    default:
        return {1, 1};
    }
}
std::pair<double, double> lightDir(GalleryLightDirection d) {
    const auto v = integerLight(d);
    const auto n = std::hypot(double(v.first), double(v.second));
    return {v.first / n, v.second / n};
}
double shade(double gx, double gy, std::pair<double, double> light, double k) {
    const auto nx = -k * gx, ny = -k * gy, nz = 1., nl = std::sqrt(nx * nx + ny * ny + 1);
    return (nx * light.first + ny * light.second + nz) / nl;
}
double weave(double x, double y, double p, double noise, std::uint32_t seed) {
    const auto cx = std::int64_t(std::floor(x / p)), cy = std::int64_t(std::floor(y / p));
    const auto horizontal = mod(double(cx + cy), 2) == 0;
    const auto ridge = std::sin((horizontal ? mod(y, p) : mod(x, p)) / p * pi),
               n = vnoise(x / (p * .7), y / (p * 2), 97 ^ seed);
    return ridge * (1 - noise) + n * noise;
}
double texture(GalleryTexture kind, double x, double y, double scaling, std::uint32_t seed) {
    x /= scaling;
    y /= scaling;
    switch (kind) {
    case GalleryTexture::Brick: {
        const auto row = std::int64_t(std::floor(y / 12));
        const auto xo = x + (row % 2 == 0 ? 0 : 12);
        const auto u = mod(xo, 24), v = mod(y, 12), d = std::min({u, 24 - u, v, 12 - v});
        return smoothstep(.6, 2.2, d) * (.85 + .15 * vnoise(x / 3, y / 3, 91 ^ seed));
    }
    case GalleryTexture::Burlap:
        return weave(x, y, 7, .35, seed);
    case GalleryTexture::Sandstone:
        return fbm(x / 7, y / 7, 93 ^ seed) * .7 +
               hash01(std::int64_t(x), std::int64_t(y), 0, 94 ^ seed) * .3;
    default:
        return weave(x, y, 3.5, .18, seed);
    }
}
double glassTexture(GalleryGlassTexture kind, double x, double y, double scaling,
                    std::uint32_t seed) {
    if (kind == GalleryGlassTexture::Canvas)
        return texture(GalleryTexture::Canvas, x, y, scaling, seed);
    x /= scaling;
    y /= scaling;
    if (kind == GalleryGlassTexture::Blocks) {
        const auto u = mod(x, 18) / 18, v = mod(y, 18) / 18;
        return std::min(1., std::min({u, 1 - u, v, 1 - v}) * 4);
    }
    if (kind == GalleryGlassTexture::TinyLens) {
        const auto u = (mod(x, 12) / 12 - .5) * 2, v = (mod(y, 12) / 12 - .5) * 2;
        return std::sqrt(std::max(0., 1 - u * u - v * v));
    }
    return fbm(x / 2.5, y / 2.5, 95 ^ seed);
}
template <class F>
void texturizeBy(Win &win, GalleryTexture kind, double scaling, double relief,
                 GalleryLightDirection light, bool invert, std::uint32_t seed, F weight) {
    if (relief <= 0)
        return;
    const auto s = scaling / 100, k = relief / 50 * 3, sign = invert ? -1. : 1.;
    const auto l = lightDir(light);
    for (std::size_t i = 0; i < win.c.size(); ++i) {
        const auto [x, y] = win.xy(i);
        const auto gx = (texture(kind, x + 1, y, s, seed) - texture(kind, x - 1, y, s, seed)) * .5 *
                        sign,
                   gy = (texture(kind, x, y + 1, s, seed) - texture(kind, x, y - 1, s, seed)) * .5 *
                        sign;
        const auto f = 1 + (std::clamp(shade(gx, gy, l, k), 0., 2.) - 1) * weight(win.c[i]);
        win.c[i] = map(win.c[i], [&](double v) { return v * f; });
    }
}
void texturize(Win &win, GalleryTexture kind, double scaling, double relief,
               GalleryLightDirection light, bool invert, std::uint32_t seed) {
    texturizeBy(win, kind, scaling, relief, light, invert, seed, [](Px) { return 1.; });
}
std::tuple<double, double, std::pair<double, double>> voronoi(double x, double y, double cell,
                                                              std::uint32_t seed) {
    const auto cx = std::int64_t(std::floor(x / cell)), cy = std::int64_t(std::floor(y / cell));
    double d1 = std::numeric_limits<double>::infinity(), d2 = d1;
    std::pair<double, double> point{x, y};
    for (int j = -1; j <= 1; ++j)
        for (int i = -1; i <= 1; ++i) {
            const auto px = (cx + i + .1 + .8 * hash01(cx + i, cy + j, 11, seed)) * cell,
                       py = (cy + j + .1 + .8 * hash01(cx + i, cy + j, 12, seed)) * cell,
                       d = std::hypot(px - x, py - y);
            if (d < d1) {
                d2 = d1;
                d1 = d;
                point = {px, py};
            } else if (d < d2)
                d2 = d;
        }
    return {d1, d2, point};
}
Px highlights(Px p, double l, double area, double intensity) {
    if (area <= 0)
        return p;
    const auto m = smoothstep(1 - area / 20 * .75, 1.02, l) * (1 - intensity / 10 * .6);
    return map(p, [&](double v) { return v + (1 - v) * m; });
}
Px normalize3(Px p) {
    const auto n = std::max(1e-9, std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]));
    return map(p, [&](double v) { return v / n; });
}
bool valid(const GalleryColoredPencilOptions &o) {
    return detail::inRange(o.pencilWidth, 1, 24) && detail::inRange(o.strokePressure, 0, 15) &&
           detail::inRange(o.paperBrightness, 0, 50) && validColor(o.background);
}
bool valid(const GalleryCutoutOptions &o) {
    return detail::inRange(o.numberOfLevels, 2, 8) && detail::inRange(o.edgeSimplicity, 0, 10) &&
           detail::inRange(o.edgeFidelity, 1, 3);
}
bool valid(const GalleryDryBrushOptions &o) {
    return detail::inRange(o.brushSize, 0, 10) && detail::inRange(o.brushDetail, 0, 10) &&
           detail::inRange(o.texture, 1, 3);
}
bool valid(const GalleryFilmGrainOptions &o) {
    return detail::inRange(o.grain, 0, 20) && detail::inRange(o.highlightArea, 0, 20) &&
           detail::inRange(o.intensity, 0, 10);
}
bool valid(const GalleryFrescoOptions &o) {
    return detail::inRange(o.brushSize, 0, 10) && detail::inRange(o.brushDetail, 0, 10) &&
           detail::inRange(o.texture, 1, 3);
}
bool valid(const GalleryNeonGlowOptions &o) {
    return detail::inRange(o.glowSize, -24, 24) && detail::inRange(o.glowBrightness, 0, 50) &&
           validColor(o.glowColor) && validColor(o.foreground) && validColor(o.background);
}
bool valid(const GalleryPaintDaubsOptions &o) {
    return detail::inRange(o.brushSize, 1, 50) && detail::inRange(o.sharpness, 0, 40) &&
           (static_cast<unsigned>(o.brushType) < 6);
}
bool valid(const GalleryPaletteKnifeOptions &o) {
    return detail::inRange(o.strokeSize, 1, 50) && detail::inRange(o.strokeDetail, 1, 3) &&
           detail::inRange(o.softness, 0, 10);
}
bool valid(const GalleryPlasticWrapOptions &o) {
    return detail::inRange(o.highlightStrength, 0, 20) && detail::inRange(o.detail, 1, 15) &&
           detail::inRange(o.smoothness, 1, 15);
}
bool valid(const GalleryPosterEdgesOptions &o) {
    return detail::inRange(o.edgeThickness, 0, 10) && detail::inRange(o.edgeIntensity, 0, 10) &&
           detail::inRange(o.posterization, 0, 6);
}
bool valid(const GalleryRoughPastelsOptions &o) {
    return detail::inRange(o.strokeLength, 0, 40) && detail::inRange(o.strokeDetail, 1, 20) &&
           (static_cast<unsigned>(o.texture) < 4) && detail::inRange(o.scaling, 50, 200) &&
           detail::inRange(o.relief, 0, 50) && (static_cast<unsigned>(o.light) < 8);
}
bool valid(const GallerySmudgeStickOptions &o) {
    return detail::inRange(o.strokeLength, 0, 10) && detail::inRange(o.highlightArea, 0, 20) &&
           detail::inRange(o.intensity, 0, 10);
}
bool valid(const GallerySpongeOptions &o) {
    return detail::inRange(o.brushSize, 0, 10) && detail::inRange(o.definition, 0, 25) &&
           detail::inRange(o.smoothness, 1, 15);
}
bool valid(const GalleryUnderpaintingOptions &o) {
    return detail::inRange(o.brushSize, 0, 40) && detail::inRange(o.textureCoverage, 0, 40) &&
           (static_cast<unsigned>(o.texture) < 4) && detail::inRange(o.scaling, 50, 200) &&
           detail::inRange(o.relief, 0, 50) && (static_cast<unsigned>(o.light) < 8);
}
bool valid(const GalleryWatercolorOptions &o) {
    return detail::inRange(o.brushDetail, 1, 14) && detail::inRange(o.shadowIntensity, 0, 10) &&
           detail::inRange(o.texture, 1, 3);
}
bool valid(const GalleryAccentedEdgesOptions &o) {
    return detail::inRange(o.edgeWidth, 1, 14) && detail::inRange(o.edgeBrightness, 0, 50) &&
           detail::inRange(o.smoothness, 1, 15);
}
bool valid(const GalleryAngledStrokesOptions &o) {
    return detail::inRange(o.directionBalance, 0, 100) && detail::inRange(o.strokeLength, 3, 50) &&
           detail::inRange(o.sharpness, 0, 10);
}
bool valid(const GalleryCrosshatchOptions &o) {
    return detail::inRange(o.strokeLength, 3, 50) && detail::inRange(o.sharpness, 0, 20) &&
           detail::inRange(o.strength, 1, 3);
}
bool valid(const GalleryDarkStrokesOptions &o) {
    return detail::inRange(o.balance, 0, 10) && detail::inRange(o.blackIntensity, 0, 10) &&
           detail::inRange(o.whiteIntensity, 0, 10);
}
bool valid(const GalleryInkOutlinesOptions &o) {
    return detail::inRange(o.strokeLength, 1, 50) && detail::inRange(o.darkIntensity, 0, 50) &&
           detail::inRange(o.lightIntensity, 0, 50);
}
bool valid(const GallerySpatterOptions &o) {
    return detail::inRange(o.sprayRadius, 0, 25) && detail::inRange(o.smoothness, 1, 15);
}
bool valid(const GallerySprayedStrokesOptions &o) {
    return detail::inRange(o.strokeLength, 0, 20) && detail::inRange(o.sprayRadius, 0, 25) &&
           (static_cast<unsigned>(o.strokeDirection) < 4);
}
bool valid(const GallerySumiEOptions &o) {
    return detail::inRange(o.strokeWidth, 3, 15) && detail::inRange(o.strokePressure, 0, 15) &&
           detail::inRange(o.contrast, 0, 40);
}
bool valid(const GalleryDiffuseGlowOptions &o) {
    return detail::inRange(o.graininess, 0, 10) && detail::inRange(o.glowAmount, 0, 20) &&
           detail::inRange(o.clearAmount, 0, 20) && validColor(o.background);
}
bool valid(const GalleryGlassOptions &o) {
    return detail::inRange(o.distortion, 0, 20) && detail::inRange(o.smoothness, 1, 15) &&
           (static_cast<unsigned>(o.texture) < 4) && detail::inRange(o.scaling, 50, 200);
}
bool valid(const GalleryOceanRippleOptions &o) {
    return detail::inRange(o.rippleSize, 1, 15) && detail::inRange(o.rippleMagnitude, 0, 20);
}
bool valid(const GalleryBasReliefOptions &o) {
    return detail::inRange(o.detail, 1, 15) && detail::inRange(o.smoothness, 1, 15) &&
           (static_cast<unsigned>(o.light) < 8) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryChalkCharcoalOptions &o) {
    return detail::inRange(o.charcoalArea, 0, 20) && detail::inRange(o.chalkArea, 0, 20) &&
           detail::inRange(o.strokePressure, 0, 5) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryCharcoalOptions &o) {
    return detail::inRange(o.charcoalThickness, 1, 7) && detail::inRange(o.detail, 0, 5) &&
           detail::inRange(o.lightDarkBalance, 0, 100) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryChromeOptions &o) {
    return detail::inRange(o.detail, 0, 10) && detail::inRange(o.smoothness, 0, 10);
}
bool valid(const GalleryConteCrayonOptions &o) {
    return detail::inRange(o.foregroundLevel, 1, 15) && detail::inRange(o.backgroundLevel, 1, 15) &&
           (static_cast<unsigned>(o.texture) < 4) && detail::inRange(o.scaling, 50, 200) &&
           detail::inRange(o.relief, 0, 50) && (static_cast<unsigned>(o.light) < 8) &&
           validColor(o.foreground) && validColor(o.background);
}
bool valid(const GalleryGraphicPenOptions &o) {
    return detail::inRange(o.strokeLength, 1, 15) && detail::inRange(o.lightDarkBalance, 0, 100) &&
           (static_cast<unsigned>(o.strokeDirection) < 4) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryHalftonePatternOptions &o) {
    return detail::inRange(o.size, 1, 12) && detail::inRange(o.contrast, 0, 50) &&
           (static_cast<unsigned>(o.patternType) < 3) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryNotePaperOptions &o) {
    return detail::inRange(o.imageBalance, 0, 50) && detail::inRange(o.graininess, 0, 20) &&
           detail::inRange(o.relief, 0, 25) && validColor(o.foreground) && validColor(o.background);
}
bool valid(const GalleryPhotocopyOptions &o) {
    return detail::inRange(o.detail, 1, 24) && detail::inRange(o.darkness, 1, 50) &&
           validColor(o.foreground) && validColor(o.background);
}
bool valid(const GalleryPlasterOptions &o) {
    return detail::inRange(o.imageBalance, 0, 50) && detail::inRange(o.smoothness, 1, 15) &&
           (static_cast<unsigned>(o.light) < 8) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryReticulationOptions &o) {
    return detail::inRange(o.density, 0, 50) && detail::inRange(o.foregroundLevel, 0, 50) &&
           detail::inRange(o.backgroundLevel, 0, 50) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryStampOptions &o) {
    return detail::inRange(o.lightDarkBalance, 0, 50) && detail::inRange(o.smoothness, 1, 50) &&
           validColor(o.foreground) && validColor(o.background);
}
bool valid(const GalleryTornEdgesOptions &o) {
    return detail::inRange(o.imageBalance, 0, 50) && detail::inRange(o.smoothness, 1, 15) &&
           detail::inRange(o.contrast, 1, 25) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryWaterPaperOptions &o) {
    return detail::inRange(o.fiberLength, 3, 50) && detail::inRange(o.brightness, 0, 100) &&
           detail::inRange(o.contrast, 0, 100);
}
bool valid(const GalleryGlowingEdgesOptions &o) {
    return detail::inRange(o.edgeWidth, 1, 14) && detail::inRange(o.edgeBrightness, 0, 20) &&
           detail::inRange(o.smoothness, 1, 15);
}
bool valid(const GalleryCraquelureOptions &o) {
    return detail::inRange(o.crackSpacing, 2, 100) && detail::inRange(o.crackDepth, 0, 10) &&
           detail::inRange(o.crackBrightness, 0, 10);
}
bool valid(const GalleryGrainOptions &o) {
    return detail::inRange(o.intensity, 0, 100) && detail::inRange(o.contrast, 0, 100) &&
           (static_cast<unsigned>(o.grainType) < 10) && validColor(o.foreground) &&
           validColor(o.background);
}
bool valid(const GalleryMosaicTilesOptions &o) {
    return detail::inRange(o.tileSize, 2, 100) && detail::inRange(o.groutWidth, 1, 15) &&
           detail::inRange(o.lightenGrout, 0, 10);
}
bool valid(const GalleryPatchworkOptions &o) {
    return detail::inRange(o.squareSize, 0, 10) && detail::inRange(o.relief, 0, 25);
}
bool valid(const GalleryStainedGlassOptions &o) {
    return detail::inRange(o.cellSize, 2, 50) && detail::inRange(o.borderThickness, 1, 20) &&
           detail::inRange(o.lightIntensity, 0, 10) && validColor(o.foreground);
}
bool valid(const GalleryTexturizerOptions &o) {
    return (static_cast<unsigned>(o.texture) < 4) && detail::inRange(o.scaling, 50, 200) &&
           detail::inRange(o.relief, 0, 50) && (static_cast<unsigned>(o.light) < 8);
}
template <class O, class F> Result<Image> process(ImageView image, const O &options, F effect) {
    if (auto v = detail::validate(image); !v)
        return std::unexpected(v.error());
    // Coordinate algorithms use signed indices. Actual buffers exceeding this cannot be processed.
    if (image.width > std::size_t(std::numeric_limits<std::int32_t>::max()) ||
        image.height > std::size_t(std::numeric_limits<std::int32_t>::max()))
        return std::unexpected(Error{ErrorCode::InvalidDimensions,
                                     "Gallery dimensions exceed signed coordinate limits"});
    if (!valid(options))
        return std::unexpected(
            Error{ErrorCode::InvalidParameter, "Gallery option is outside its documented range"});
    return detail::allocated([&] {
        Win win(image);
        effect(win);
        auto out = detail::copy(image);
        for (std::size_t i = 0; i < win.c.size(); ++i)
            if (image.pixels[i].a > 0)
                out.pixels[i] = {float(clamp01(win.c[i][0])), float(clamp01(win.c[i][1])),
                                 float(clamp01(win.c[i][2])), image.pixels[i].a};
        return out;
    });
}
} // namespace
Result<Image> coloredPencil(ImageView image, const GalleryColoredPencilOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto paper =
            map(o.background, [&](double v) { return v * (.7 + .3 * o.paperBrightness / 50); });
        const auto l = win.luminance(), edge = gradMag(l, win);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto darkness = 1 - l[i],
                       h1 = strokes(x, y, {1, 1}, 5 + o.pencilWidth * 3, .5 + o.pencilWidth * .45,
                                    11 ^ o.seed),
                       h2 = strokes(x, y, {1, -1}, 5 + o.pencilWidth * 3, .5 + o.pencilWidth * .45,
                                    12 ^ o.seed),
                       hatch = std::max(h1, h2 * smoothstep(.3, .8, darkness));
            const auto coverage = clamp01(darkness * (.55 + o.strokePressure / 15) + edge[i] * 3 -
                                          (1 - hatch) * .8 + .25);
            win.c[i] = mix(paper, map(win.c[i], [](double v) { return v * .88; }), coverage);
        }
    });
}
Result<Image> cutout(ImageView image, const GalleryCutoutOptions &o) {
    return process(image, o, [&](Win &win) {
        blur3(win.c, win, .5 + o.edgeSimplicity * (1.1 - .3 * o.edgeFidelity));
        const auto levels = std::round(o.numberOfLevels);
        for (auto &p : win.c) {
            const auto l = std::max(1e-4, lum(p)), k = posterize(l, levels) / l;
            p = map(p, [&](double v) { return posterize(clamp01(v * k), levels * 2); });
        }
    });
}
Result<Image> dryBrush(ImageView image, const GalleryDryBrushOptions &o) {
    return process(image, o, [&](Win &win) {
        kuwahara(win, 1 + std::size_t(o.brushSize));
        unsharp(win, 1, .5);
        const auto levels = 3 + o.brushDetail, amp = .02 + (o.texture - 1) * .05;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto n = (hash01(x, y, 0, 21 ^ o.seed) - .5) * amp;
            win.c[i] =
                map(win.c[i], [&](double v) { return posterize(v, levels) * .6 + v * .4 + n; });
        }
    });
}
Result<Image> fresco(ImageView image, const GalleryFrescoOptions &o) {
    return process(image, o, [&](Win &win) {
        kuwahara(win, 1 + std::size_t(o.brushSize));
        unsharp(win, 1, .9);
        const auto levels = 3 + o.brushDetail, amp = .02 + (o.texture - 1) * .05;
        const auto edges = gradMag(win.luminance(), win);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto n = (hash01(x, y, 0, 21 ^ o.seed) - .5) * amp;
            win.c[i] = map(win.c[i], [&](double v) {
                const auto p = posterize(v, levels) * .6 + v * .4 + n;
                return std::pow(clamp01((p - .5) * 1.3 + .45), 1.15) *
                       (1 - clamp01(edges[i] * 2.5) * .6);
            });
        }
    });
}
Result<Image> galleryFilmGrain(ImageView image, const GalleryFilmGrainOptions &o) {
    return process(image, o, [&](Win &win) {
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto l = lum(win.c[i]), n = (hash01(x, y, 0, 31 ^ o.seed) - .5) * o.grain / 20 *
                                              .5 * (.3 + 2.8 * l * (1 - l));
            win.c[i] = highlights(map(win.c[i], [&](double v) { return v + n; }), l,
                                  o.highlightArea, o.intensity);
        }
    });
}
Result<Image> neonGlow(ImageView image, const GalleryNeonGlowOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto l = win.luminance();
        auto m = l;
        if (o.glowSize < 0)
            for (auto &v : m)
                v = 1 - v;
        blur1(m, win, std::abs(o.glowSize) * .6 + .5);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto base = mix(o.foreground, o.background, l[i]);
            const auto k = clamp01(m[i] * 1.2) * o.glowBrightness / 50 * 2;
            for (int ch = 0; ch < 3; ++ch)
                win.c[i][ch] = 1 - (1 - base[ch]) * (1 - clamp01(o.glowColor[ch] * k));
        }
    });
}
Result<Image> paintDaubs(ImageView image, const GalleryPaintDaubsOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto wide = o.brushType == GalleryBrushType::WideSharp ||
                          o.brushType == GalleryBrushType::WideBlurry;
        kuwahara(win, std::size_t(std::max(1., o.brushSize / 2 * (wide ? 1.5 : 1.))));
        const auto sharp = o.sharpness / 40;
        if (o.brushType == GalleryBrushType::WideBlurry)
            blur3(win.c, win, 1.5);
        else if (o.brushType == GalleryBrushType::WideSharp)
            unsharp(win, 1.5, sharp * 4);
        else
            unsharp(win, 1.2, sharp * 2.5);
        if (o.brushType == GalleryBrushType::LightRough ||
            o.brushType == GalleryBrushType::DarkRough ||
            o.brushType == GalleryBrushType::Sparkle) {
            const auto edges = gradMag(win.luminance(), win);
            for (std::size_t i = 0; i < win.c.size(); ++i) {
                const auto [x, y] = win.xy(i);
                const auto n = fbm(x / 2., y / 2., 41 ^ o.seed) - .5;
                win.c[i] = map(win.c[i], [&](double v) {
                    if (o.brushType == GalleryBrushType::LightRough)
                        return v + std::max(0., n) * .35;
                    if (o.brushType == GalleryBrushType::DarkRough)
                        return v * (1 - std::max(0., n) * .7);
                    return v + clamp01(edges[i] * 4) *
                                   smoothstep(.75, .95, hash01(x, y, 0, 42 ^ o.seed)) * .8;
                });
            }
        }
    });
}
Result<Image> paletteKnife(ImageView image, const GalleryPaletteKnifeOptions &o) {
    return process(image, o, [&](Win &win) {
        kuwahara(win, std::size_t(std::max(1., o.strokeSize / 3)));
        const auto levels = 2 + o.strokeDetail * 3;
        for (auto &p : win.c)
            p = map(p, [&](double v) { return posterize(v, levels) * .5 + v * .5; });
        blur3(win.c, win, o.softness * .5);
    });
}
Result<Image> plasticWrap(ImageView image, const GalleryPlasticWrapOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, .5 + o.smoothness * .5 + (15 - o.detail) * .2);
        const auto [gx, gy] = sobel(l, win);
        const auto light = normalize3({-.45, -.65, 1}),
                   half = normalize3({light[0], light[1], light[2] + 1});
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto n = normalize3({-14 * gx[i], -14 * gy[i], 1});
            const auto ndh = std::max(0., n[0] * half[0] + n[1] * half[1] + n[2] * half[2]),
                       tilt = 1 - n[2],
                       spec = std::pow(ndh, 30) * smoothstep(.01, .12, tilt) +
                              smoothstep(.25, .6, tilt) * .25;
            win.c[i] = map(win.c[i], [&](double v) {
                return v * .88 + spec * o.highlightStrength / 20 * 1.3;
            });
        }
    });
}
Result<Image> posterEdges(ImageView image, const GalleryPosterEdgesOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, .6 + o.edgeThickness * .6);
        const auto edge = gradMag(l, win);
        const auto intensity = o.edgeIntensity / 10, levels = o.posterization + 2;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto ink =
                smoothstep(.015, .06, edge[i] * (1 + intensity * 3)) * (.55 + intensity * .45);
            win.c[i] = map(win.c[i], [&](double v) { return posterize(v, levels) * (1 - ink); });
        }
    });
}
Result<Image> roughPastels(ImageView image, const GalleryRoughPastelsOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto detail = (o.strokeDetail - 1) / 19;
        if (o.strokeLength >= 1) {
            auto src = win;
            blur3(src.c, src, .6 + (1 - detail) * 1.2);
            for (std::size_t i = 0; i < win.c.size(); ++i) {
                const auto [x, y] = win.xy(i);
                const auto [c, t] =
                    paintStroke(src, x, y, {1, -1}, 2 + o.strokeLength, 1.5, 51 ^ o.seed, win.c[i]);
                const auto n =
                    (strokes(x, y, {1, -1}, 2 + o.strokeLength * .5, .8, 52 ^ o.seed) - .5) * .12 +
                    (.5 - std::abs(t - .5)) * .06;
                win.c[i] = map(mix(c, win.c[i], detail * .6), [&](double v) { return v + n; });
            }
        }
        texturizeBy(win, o.texture, o.scaling, o.relief, o.light, o.invert, o.seed,
                    [](Px p) { return .15 + .85 * (1 - clamp01(lum(p))); });
    });
}
Result<Image> smudgeStick(ImageView image, const GallerySmudgeStickOptions &o) {
    return process(image, o, [&](Win &win) {
        lineBlur3(win.c, win, {1, 1}, std::size_t(o.strokeLength * 1.5));
        for (auto &p : win.c) {
            const auto l = lum(p);
            p = highlights(map(p, [&](double v) { return v * (.7 + .3 * l); }), l, o.highlightArea,
                           o.intensity);
        }
    });
}
Result<Image> sponge(ImageView image, const GallerySpongeOptions &o) {
    return process(image, o, [&](Win &win) {
        kuwahara(win, 1 + std::size_t(o.brushSize));
        const auto definition = o.definition / 25, scale = 1.5 + o.smoothness * .5;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto hole = smoothstep(.58, .75, fbm(x / scale, y / scale, 61 ^ o.seed));
            win.c[i] = map(win.c[i], [&](double v) {
                return clamp01((v - .5) * (1 + definition * .6) + .5) *
                       (1 - hole * definition * .4);
            });
        }
    });
}
Result<Image> underpainting(ImageView image, const GalleryUnderpaintingOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto original = win.c;
        blur3(win.c, win, .5 + o.brushSize * .4);
        for (std::size_t i = 0; i < win.c.size(); ++i)
            win.c[i] = saturate(mix(original[i], win.c[i], .8), 1.15);
        texturize(win, o.texture, o.scaling, o.relief * (.5 + o.textureCoverage / 40 * 1.5),
                  o.light, o.invert, o.seed);
    });
}
Result<Image> watercolor(ImageView image, const GalleryWatercolorOptions &o) {
    return process(image, o, [&](Win &win) {
        kuwahara(win, 1 + std::size_t((15 - o.brushDetail) / 2.5));
        const auto shadow = o.shadowIntensity / 10, amp = (o.texture - 1) * .04;
        const auto edges = gradMag(win.luminance(), win);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto n = (fbm(x / 3., y / 3., 71 ^ o.seed) - .5) * amp,
                       k = 1 - clamp01(edges[i] * (3 + shadow * 8)) * .6;
            win.c[i] = map(saturate(win.c[i], 1.2), [&](double v) {
                return std::pow(clamp01(v * k + n), 1.1 + shadow * .6);
            });
        }
    });
}
Result<Image> accentedEdges(ImageView image, const GalleryAccentedEdgesOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, .4 + o.smoothness * .3);
        auto edge = gradMag(l, win);
        blur1(edge, win, o.edgeWidth * .5);
        const auto brightness = o.edgeBrightness / 50;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto k = clamp01(edge[i] * 6);
            win.c[i] = map(win.c[i], [&](double v) {
                return brightness >= .5 ? v + (1 - v) * k * (brightness - .5) * 2
                                        : v * (1 - k * (.5 - brightness) * 2);
            });
        }
    });
}
Result<Image> angledStrokes(ImageView image, const GalleryAngledStrokesOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto len = std::max(3., o.strokeLength), balance = o.directionBalance / 100;
        auto src = win;
        blur3(src.c, src, .8);
        const auto l = src.luminance();
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto k = smoothstep(1 - balance - .05, 1 - balance + .05, l[i]);
            const auto [down, td] = paintStroke(src, x, y, {1, 1}, len, 2, 83 ^ o.seed, win.c[i]);
            const auto [up, tu] = paintStroke(src, x, y, {1, -1}, len, 2, 84 ^ o.seed, win.c[i]);
            const auto n = (strokes(x, y, {1, 1}, len * .5, .7, 85 ^ o.seed) - .5) * (1 - k) +
                           (strokes(x, y, {1, -1}, len * .5, .7, 86 ^ o.seed) - .5) * k;
            win.c[i] = map(mix(down, up, k), [&](double v) { return v + n * .1; });
        }
        unsharp(win, 1, .2 + o.sharpness / 10 * 2.3);
    });
}
Result<Image> crosshatch(ImageView image, const GalleryCrosshatchOptions &o) {
    return process(image, o, [&](Win &win) {
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto n = (hash01(x, y, 0, 81 ^ o.seed) - .5) * .12 * o.strength;
            win.c[i] = map(win.c[i], [&](double v) { return v + n; });
        }
        auto a = win.c;
        lineBlur3(a, win, {1, 1}, std::size_t(o.strokeLength / 3));
        lineBlur3(win.c, win, {1, -1}, std::size_t(o.strokeLength / 3));
        for (std::size_t i = 0; i < win.c.size(); ++i)
            win.c[i] = mix(a[i], win.c[i], .5);
        unsharp(win, 1, o.sharpness / 20 * 3);
    });
}
Result<Image> darkStrokes(ImageView image, const GalleryDarkStrokesOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto l = win.luminance();
        auto a = win.c;
        lineBlur3(a, win, {1, 1}, 3);
        lineBlur3(win.c, win, {1, -1}, 8);
        const auto t = .25 + o.balance / 10 * .5;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto dark = map(a[i],
                                  [&](double v) { return v * (1 - o.blackIntensity / 10 * .85); }),
                       light = map(win.c[i], [&](double v) {
                           return v + (1 - v) * o.whiteIntensity / 10 * .5;
                       });
            win.c[i] = mix(dark, light, smoothstep(t - .1, t + .1, l[i]));
        }
    });
}
Result<Image> inkOutlines(ImageView image, const GalleryInkOutlinesOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto l = win.luminance(), edge = gradMag(l, win);
        lineBlur3(win.c, win, {1, 1}, std::size_t(o.strokeLength / 2));
        const auto dark = o.darkIntensity / 50, light = o.lightIntensity / 50;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto ink = clamp01(edge[i] * 5) * dark * 1.5,
                       lt = smoothstep(.5, 1, l[i]) * light,
                       dk = smoothstep(.5, 0, l[i]) * dark * .8;
            win.c[i] = map(win.c[i], [&](double v) {
                return (v + (1 - v) * lt) * (1 - dk) * (1 - clamp01(ink));
            });
        }
    });
}
Result<Image> spatter(ImageView image, const GallerySpatterOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto src = win;
        const auto scale = .6 + o.smoothness * .25;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto dx = (vnoise(x / scale, y / scale, 91 ^ o.seed) - .5) * 2 * o.sprayRadius,
                       dy = (vnoise(x / scale, y / scale, 92 ^ o.seed) - .5) * 2 * o.sprayRadius;
            win.c[i] = src.sample(x + .5 + dx, y + .5 + dy, win.c[i]);
        }
    });
}
Result<Image> sprayedStrokes(ImageView image, const GallerySprayedStrokesOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto src = win;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto dx = (vnoise(x / 1.2, y / 1.2, 91 ^ o.seed) - .5) * 2 * o.sprayRadius,
                       dy = (vnoise(x / 1.2, y / 1.2, 92 ^ o.seed) - .5) * 2 * o.sprayRadius;
            win.c[i] = src.sample(x + .5 + dx, y + .5 + dy, win.c[i]);
        }
        lineBlur3(win.c, win, direction(o.strokeDirection), std::size_t(o.strokeLength / 2));
    });
}
Result<Image> sumiE(ImageView image, const GallerySumiEOptions &o) {
    return process(image, o, [&](Win &win) {
        blur3(win.c, win, o.strokeWidth * .2);
        lineBlur3(win.c, win, {1, 1}, std::size_t(o.strokeWidth / 2));
        const auto pressure = o.strokePressure / 15, contrast = o.contrast / 40;
        for (auto &p : win.c) {
            const auto ink = std::pow(smoothstep(.65, .15, lum(p)), 1 - pressure * .5);
            p = map(p, [&](double v) {
                return clamp01((v - .5) * (1 + contrast) + .5) * (1 - ink * .9 * (.5 + contrast));
            });
        }
    });
}
Result<Image> diffuseGlow(ImageView image, const GalleryDiffuseGlowOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto threshold = .2 + o.clearAmount / 20 * .7;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            auto m =
                smoothstep(threshold - .3, threshold + .1, lum(win.c[i])) * o.glowAmount / 20 * 1.4;
            if (hash01(x, y, 0, 101 ^ o.seed) < o.graininess / 10 * .3 * (m + .15))
                m += .5;
            win.c[i] = mix(win.c[i], o.background, clamp01(m));
        }
    });
}
Result<Image> glass(ImageView image, const GalleryGlassOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto src = win;
        const auto step = .5 + o.smoothness * .5, scale = o.scaling / 100,
                   sign = o.invert ? -1. : 1., limit = o.distortion * 2;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto gx = glassTexture(o.texture, x + step, y, scale, o.seed) -
                            glassTexture(o.texture, x - step, y, scale, o.seed),
                       gy = glassTexture(o.texture, x, y + step, scale, o.seed) -
                            glassTexture(o.texture, x, y - step, scale, o.seed),
                       dx = std::clamp(gx * sign * o.distortion * 3, -limit, limit),
                       dy = std::clamp(gy * sign * o.distortion * 3, -limit, limit);
            win.c[i] = src.sample(x + .5 + dx, y + .5 + dy, win.c[i]);
        }
    });
}
Result<Image> oceanRipple(ImageView image, const GalleryOceanRippleOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto src = win;
        const auto scale = 2 + o.rippleSize * 1.2, mag = o.rippleMagnitude * .9;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto dx = (vnoise(x / scale, y / scale, 111 ^ o.seed) - .5) * 2 * mag,
                       dy = (vnoise(x / scale, y / scale, 112 ^ o.seed) - .5) * 2 * mag;
            win.c[i] = src.sample(x + .5 + dx, y + .5 + dy, win.c[i]);
        }
    });
}
Result<Image> basRelief(ImageView image, const GalleryBasReliefOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        const auto original = l;
        blur1(l, win, .3 + o.smoothness * .5 + (15 - o.detail) * .12);
        const auto [gx, gy] = sobel(l, win);
        const auto light = lightDir(o.light);
        const auto k = 3 + o.detail * .4;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto t =
                clamp01(original[i] * .5 + .25 - k * (gx[i] * light.first + gy[i] * light.second));
            win.c[i] = mix(o.foreground, o.background, t);
        }
    });
}
Result<Image> chalkCharcoal(ImageView image, const GalleryChalkCharcoalOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto mid = mix(o.foreground, o.background, .5);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto l = lum(win.c[i]), cn = strokes(x, y, {1, 1}, 9, .9, 121 ^ o.seed) - .5,
                       kn = strokes(x, y, {1, -1}, 9, .9, 122 ^ o.seed) - .5,
                       cc = smoothstep(.25, .6,
                                       (1 - l) * (.4 + o.charcoalArea / 20 * 1.2) +
                                           cn * (.6 + o.strokePressure / 5 * .4) - .15),
                       kc = smoothstep(.25, .6,
                                       l * (.4 + o.chalkArea / 20 * 1.2) +
                                           kn * (.6 + o.strokePressure / 5 * .4) - .15);
            win.c[i] = mix(mix(mid, o.foreground, cc), o.background, kc * (1 - cc));
        }
    });
}
Result<Image> charcoal(ImageView image, const GalleryCharcoalOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, 1);
        const auto edge = gradMag(l, win);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto sn = strokes(x, y, {1, 1}, 8 + o.charcoalThickness * 3,
                                    .6 + o.charcoalThickness * .5, 131 ^ o.seed) -
                            .5,
                       coverage =
                           smoothstep(1 - o.lightDarkBalance / 100 - .2,
                                      1 - o.lightDarkBalance / 100 + .2, 1 - l[i] + sn * .6) +
                           clamp01(edge[i] * o.detail / 5 * 6);
            win.c[i] = mix(o.background, o.foreground, clamp01(coverage));
        }
    });
}
Result<Image> chrome(ImageView image, const GalleryChromeOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, 1 + o.smoothness / 10 * 4);
        const auto edge = gradMag(l, win);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto v =
                .5 + .5 * std::sin((l[i] * (2.5 + o.detail / 10 * 3) + edge[i] * 6) * 2 * pi);
            win.c[i] = grey(smoothstep(.05, .95, v));
        }
    });
}
Result<Image> conteCrayon(ImageView image, const GalleryConteCrayonOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto mid = mix(o.foreground, o.background, .5);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto l = lum(win.c[i]), n = strokes(x, y, {1, -1}, 7, 1, 141 ^ o.seed) - .5,
                       foreground = clamp01((.6 - l) * 2 * o.foregroundLevel / 15 * 1.4 + n * .5),
                       background = clamp01((l - .4) * 2 * o.backgroundLevel / 15 * 1.4 - n * .5);
            win.c[i] = mix(mix(mid, o.foreground, foreground), o.background,
                           background * (1 - foreground));
        }
        texturize(win, o.texture, o.scaling, o.relief, o.light, o.invert, o.seed);
    });
}
Result<Image> graphicPen(ImageView image, const GalleryGraphicPenOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto radius = std::size_t(o.strokeLength / 2);
        const auto l = win.luminance();
        Plane noise(win.c.size());
        for (std::size_t i = 0; i < noise.size(); ++i) {
            const auto [x, y] = win.xy(i);
            noise[i] = hash01(x, y, 0, 151 ^ o.seed);
        }
        lineBlur(noise, win, direction(o.strokeDirection), radius);
        const auto sd = .2887 / std::sqrt(double(2 * radius + 1));
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto t = l[i] + (noise[i] - .5) / sd * .16;
            win.c[i] =
                mix(o.background, o.foreground,
                    smoothstep(o.lightDarkBalance / 100 + .04, o.lightDarkBalance / 100 - .04, t));
        }
    });
}
Result<Image> halftonePattern(ImageView image, const GalleryHalftonePatternOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto cell = 3 + o.size * 2, contrast = o.contrast / 50, cx = win.w / 2.,
                   cy = win.h / 2.;
        auto l = win.luminance();
        blur1(l, win, .5 + o.size * .3);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto fx = x + .5, fy = y + .5,
                       d = clamp01((1 - l[i] - .5) * (1 + contrast * 3) + .5), aa = 1 / cell;
            double ink;
            if (o.patternType == GalleryPatternType::Circle) {
                const auto r = std::hypot(fx - cx, fy - cy) / cell,
                           q = std::abs(mod(r, 1) - .5) * 2;
                ink = smoothstep(d + aa, d - aa, q);
            } else if (o.patternType == GalleryPatternType::Line) {
                const auto q = std::abs(mod(fy / cell, 1) - .5) * 2;
                ink = smoothstep(d + aa, d - aa, q);
            } else {
                const auto u = mod(fx / cell, 1) - .5, v = mod(fy / cell, 1) - .5,
                           q = std::hypot(u, v) * 2, r = std::sqrt(d) * 1.15;
                ink = smoothstep(r + aa, r - aa, q);
            }
            win.c[i] = mix(o.background, o.foreground, ink);
        }
    });
}
Result<Image> notePaper(ImageView image, const GalleryNotePaperOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, 1);
        auto mask = l;
        for (auto &v : mask)
            v = smoothstep(o.imageBalance / 50 - .05, o.imageBalance / 50 + .05, v);
        blur1(mask, win, 1.2);
        const auto [gx, gy] = sobel(mask, win);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto n = (hash01(x, y, 0, 161 ^ o.seed) - .5) * o.graininess / 20 * .3,
                       f = std::clamp(shade(gx[i], gy[i], lightDir(GalleryLightDirection::TopLeft),
                                            o.relief / 25 * 6),
                                      0., 2.);
            win.c[i] = map(mix(o.foreground, o.background, mask[i]),
                           [&](double v) { return (v + n) * f; });
        }
    });
}
Result<Image> photocopy(ImageView image, const GalleryPhotocopyOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto l = win.luminance();
        auto b = l;
        blur1(b, win, 1 + o.detail * .5);
        const auto darkness = o.darkness / 50;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto highpass = l[i] - b[i],
                       ink = smoothstep(0, .06, -highpass * (1 + darkness * 8)) +
                             smoothstep(.35, .05, l[i]) * darkness * 1.5;
            win.c[i] = mix(o.background, o.foreground, clamp01(ink));
        }
    });
}
Result<Image> plaster(ImageView image, const GalleryPlasterOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, .5 + o.smoothness * .8);
        auto mask = l;
        for (auto &v : mask)
            v = smoothstep(o.imageBalance / 50 + .1, o.imageBalance / 50 - .1, v);
        blur1(mask, win, 1.5);
        const auto [gx, gy] = sobel(mask, win);
        const auto light = lightDir(o.light);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto f = std::clamp(shade(gx[i], gy[i], light, 6), 0., 2.);
            win.c[i] =
                map(mix(o.background, o.foreground, mask[i] * .6), [&](double v) { return v * f; });
        }
    });
}
Result<Image> reticulation(ImageView image, const GalleryReticulationOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto ink = mix(o.background, o.foreground, .3 + o.foregroundLevel / 50 * .7),
                   paper = mix(o.background, o.foreground, o.backgroundLevel / 50 * .6);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto n = .5 * hash01(x, y, 0, 171 ^ o.seed) +
                           .5 * vnoise(x / 1.8, y / 1.8, 172 ^ o.seed),
                       t = 1 - lum(win.c[i]) + (n - .5) * (.4 + o.density / 50 * 1.2);
            win.c[i] = mix(paper, ink, smoothstep(.45, .55, t));
        }
    });
}
Result<Image> stamp(ImageView image, const GalleryStampOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, .3 + o.smoothness * .3);
        for (std::size_t i = 0; i < win.c.size(); ++i)
            win.c[i] =
                mix(o.background, o.foreground,
                    smoothstep(o.lightDarkBalance / 50 + .02, o.lightDarkBalance / 50 - .02, l[i]));
    });
}
Result<Image> tornEdges(ImageView image, const GalleryTornEdgesOptions &o) {
    return process(image, o, [&](Win &win) {
        auto l = win.luminance();
        blur1(l, win, 1);
        const auto rough = (16 - o.smoothness) / 15 * .35, aa = .25 / o.contrast;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto t = l[i] + (fbm(x / 2., y / 2., 181 ^ o.seed) - .5) * rough;
            win.c[i] = mix(o.background, o.foreground,
                           smoothstep(o.imageBalance / 50 + aa, o.imageBalance / 50 - aa, t));
        }
    });
}
Result<Image> waterPaper(ImageView image, const GalleryWaterPaperOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto radius = std::size_t(o.fiberLength / 6);
        lineBlur3(win.c, win, {1, 0}, radius);
        lineBlur3(win.c, win, {0, 1}, radius);
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto fiber = std::max(strokes(x, y, {1, 0}, o.fiberLength, 1, 191 ^ o.seed),
                                        strokes(x, y, {0, 1}, o.fiberLength, 1, 192 ^ o.seed));
            win.c[i] = map(win.c[i], [&](double v) {
                return (v * (.85 + .25 * fiber) - .5) * (o.contrast / 100 * 1.6 + .2) + .5 +
                       (o.brightness / 100 - .5) * .8;
            });
        }
    });
}
Result<Image> glowingEdges(ImageView image, const GalleryGlowingEdgesOptions &o) {
    return process(image, o, [&](Win &win) {
        blur3(win.c, win, .3 + o.smoothness * .25);
        auto edges = win.c;
        for (int ch = 0; ch < 3; ++ch) {
            Plane p(win.c.size());
            for (std::size_t i = 0; i < p.size(); ++i)
                p[i] = win.c[i][ch];
            const auto gradient = gradMag(p, win);
            for (std::size_t i = 0; i < p.size(); ++i)
                edges[i][ch] = gradient[i];
        }
        blur3(edges, win, o.edgeWidth * .4);
        const auto k = 2 + o.edgeBrightness * 1.2;
        for (std::size_t i = 0; i < win.c.size(); ++i)
            win.c[i] = map(edges[i], [&](double v) { return clamp01(v * k); });
    });
}
Result<Image> craquelure(ImageView image, const GalleryCraquelureOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto depth = o.crackDepth / 10, brightness = o.crackBrightness / 10,
                   cell = o.crackSpacing * 1.3 + 1, width = .8 + depth * .8;
        const auto crack = [&](double x, double y) {
            const auto [d1, d2, p] = voronoi(x, y, cell, 201 ^ o.seed);
            return 1 - smoothstep(0, width, d2 - d1);
        };
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto fx = x + .5, fy = y + .5, c = crack(fx, fy), gx = crack(fx + 1, fy) - c,
                       gy = crack(fx, fy + 1) - c,
                       f = std::clamp(
                           shade(-gx, -gy, lightDir(GalleryLightDirection::TopLeft), depth * 2), 0.,
                           2.);
            win.c[i] = map(win.c[i], [&](double v) {
                return v * (.55 + brightness * .5) * (1 - c * depth * .6) * f;
            });
        }
    });
}
Result<Image> grain(ImageView image, const GalleryGrainOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto intensity = o.intensity / 100, contrast = o.contrast / 100;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto noise = [&](std::uint32_t c) { return hash01(x, y, c, 211 ^ o.seed) - .5; };
            const auto p = win.c[i];
            Px q = p;
            switch (o.grainType) {
            case GalleryGrainType::Soft:
                for (int ch = 0; ch < 3; ++ch)
                    q[ch] = p[ch] + noise(ch) * intensity * .4;
                break;
            case GalleryGrainType::Sprinkles:
                if (hash01(x, y, 3, 211 ^ o.seed) < intensity * .15)
                    q = o.background;
                break;
            case GalleryGrainType::Clumped:
                q = map(p, [&](double v) {
                    return v + (vnoise(x / 2., y / 2., 212 ^ o.seed) - .5) * intensity * 1.2;
                });
                break;
            case GalleryGrainType::Contrasty:
                q = map(p, [&](double v) {
                    return (v - .5) * (1 + intensity * 2) + .5 + noise(4) * intensity * .5;
                });
                break;
            case GalleryGrainType::Enlarged:
                for (int ch = 0; ch < 3; ++ch)
                    q[ch] =
                        p[ch] +
                        (vnoise(x / 3., y / 3., std::uint32_t(213 + ch) ^ o.seed) - .5) * intensity;
                break;
            case GalleryGrainType::Stippled:
                q = mix(p, lum(p) > hash01(x, y, 5, 211 ^ o.seed) ? o.background : o.foreground,
                        .3 + intensity * .7);
                break;
            case GalleryGrainType::Horizontal:
                q = map(p, [&](double v) {
                    return v + (vnoise(x / 25., double(y), 216 ^ o.seed) - .5) * intensity;
                });
                break;
            case GalleryGrainType::Vertical:
                q = map(p, [&](double v) {
                    return v + (vnoise(double(x), y / 25., 217 ^ o.seed) - .5) * intensity;
                });
                break;
            case GalleryGrainType::Speckle:
                if (hash01(x, y, 6, 211 ^ o.seed) < intensity * .1)
                    q = mix(p, o.foreground, .85);
                break;
            default:
                for (int ch = 0; ch < 3; ++ch)
                    q[ch] = p[ch] + noise(ch) * intensity * .8;
                break;
            }
            win.c[i] = map(q, [&](double v) { return (v - .5) * (.5 + contrast) + .5; });
        }
    });
}
Result<Image> mosaicTiles(ImageView image, const GalleryMosaicTilesOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto tile = o.tileSize, width = o.groutWidth, light = o.lightenGrout / 10;
        const auto edge = [&](double x, double y) {
            const auto jx = x +
                            (vnoise(x / tile * .7, y / tile * .7, 221 ^ o.seed) - .5) * tile * .3,
                       jy = y +
                            (vnoise(x / tile * .7, y / tile * .7, 222 ^ o.seed) - .5) * tile * .3,
                       u = mod(jx, tile), v = mod(jy, tile);
            return std::min({u, tile - u, v, tile - v});
        };
        const auto height = [&](double x, double y) {
            return smoothstep(width * .5, width * .5 + 2.5, edge(x, y));
        };
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto fx = x + .5, fy = y + .5,
                       grout = smoothstep(width * .5 + .7, width * .5 - .3, edge(fx, fy)),
                       shading = (height(fx - 1, fy - 1) - height(fx + 1, fy + 1)) * .35;
            const auto gc = grey(clamp01(lum(win.c[i]) * .5 + light * .45));
            win.c[i] = mix(map(win.c[i], [&](double v) { return v * (1 + shading); }), gc, grout);
        }
    });
}
Result<Image> patchwork(ImageView image, const GalleryPatchworkOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto size = 3 + o.squareSize * 1.5, relief = o.relief / 25;
        const auto src = win;
        const auto height = [&](double x, double y) {
            const auto bx = std::int64_t(std::floor(x / size)),
                       by = std::int64_t(std::floor(y / size));
            const auto u = mod(x, size), v = mod(y, size),
                       bevel = smoothstep(0, 1.5, std::min({u, size - u, v, size - v}));
            return hash01(bx, by, 0, 231 ^ o.seed) * bevel;
        };
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto fx = x + .5, fy = y + .5, bx = std::floor(fx / size),
                       by = std::floor(fy / size);
            const auto c =
                src.at(std::int64_t((bx + .5) * size), std::int64_t((by + .5) * size), win.c[i]);
            const auto shading = (height(fx - 1, fy - 1) - height(fx + 1, fy + 1)) * relief * 1.5;
            win.c[i] = map(c, [&](double v) { return v * (1 + shading); });
        }
    });
}
Result<Image> stainedGlass(ImageView image, const GalleryStainedGlassOptions &o) {
    return process(image, o, [&](Win &win) {
        const auto cell = o.cellSize * 1.6 + 1, light = o.lightIntensity / 10, cx = win.w / 2.,
                   cy = win.h / 2., half = std::max(1., double(std::max(win.w, win.h)) / 2);
        const auto src = win;
        for (std::size_t i = 0; i < win.c.size(); ++i) {
            const auto [x, y] = win.xy(i);
            const auto fx = x + .5, fy = y + .5;
            const auto [d1, d2, point] = voronoi(fx, fy, cell, 241 ^ o.seed);
            const auto border = smoothstep(o.borderThickness * .5 + .6, o.borderThickness * .5 - .4,
                                           (d2 - d1) * .5),
                       dn = std::min(1., std::hypot(fx - cx, fy - cy) / half);
            const auto color =
                map(src.at(std::int64_t(point.first), std::int64_t(point.second), win.c[i]),
                    [&](double v) { return v * (1 + light * .6 * (1 - dn)); });
            win.c[i] = mix(color, o.foreground, border);
        }
    });
}
Result<Image> texturizer(ImageView image, const GalleryTexturizerOptions &o) {
    return process(image, o, [&](Win &win) {
        texturize(win, o.texture, o.scaling, o.relief, o.light, o.invert, o.seed);
    });
}

Result<Image> filterGallery(ImageView image, std::span<const GalleryEffect> effects) {
    if (auto checked = detail::validate(image); !checked)
        return std::unexpected(checked.error());
    if (effects.size() > 64)
        return std::unexpected(
            Error{ErrorCode::InvalidParameter, "Gallery stack accepts at most 64 effects"});
    auto guarded = detail::allocated([&]() -> Result<Image> {
        auto current = detail::copy(image);
        for (const auto &effect : effects) {
            auto result = std::visit(
                [&](const auto &options) -> Result<Image> {
                    using O = std::decay_t<decltype(options)>;
                    if constexpr (std::is_same_v<O, GalleryColoredPencilOptions>)
                        return coloredPencil(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryCutoutOptions>)
                        return cutout(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryDryBrushOptions>)
                        return dryBrush(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryFilmGrainOptions>)
                        return galleryFilmGrain(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryFrescoOptions>)
                        return fresco(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryNeonGlowOptions>)
                        return neonGlow(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryPaintDaubsOptions>)
                        return paintDaubs(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryPaletteKnifeOptions>)
                        return paletteKnife(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryPlasticWrapOptions>)
                        return plasticWrap(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryPosterEdgesOptions>)
                        return posterEdges(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryRoughPastelsOptions>)
                        return roughPastels(current.view(), options);
                    else if constexpr (std::is_same_v<O, GallerySmudgeStickOptions>)
                        return smudgeStick(current.view(), options);
                    else if constexpr (std::is_same_v<O, GallerySpongeOptions>)
                        return sponge(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryUnderpaintingOptions>)
                        return underpainting(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryWatercolorOptions>)
                        return watercolor(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryAccentedEdgesOptions>)
                        return accentedEdges(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryAngledStrokesOptions>)
                        return angledStrokes(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryCrosshatchOptions>)
                        return crosshatch(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryDarkStrokesOptions>)
                        return darkStrokes(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryInkOutlinesOptions>)
                        return inkOutlines(current.view(), options);
                    else if constexpr (std::is_same_v<O, GallerySpatterOptions>)
                        return spatter(current.view(), options);
                    else if constexpr (std::is_same_v<O, GallerySprayedStrokesOptions>)
                        return sprayedStrokes(current.view(), options);
                    else if constexpr (std::is_same_v<O, GallerySumiEOptions>)
                        return sumiE(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryDiffuseGlowOptions>)
                        return diffuseGlow(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryGlassOptions>)
                        return glass(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryOceanRippleOptions>)
                        return oceanRipple(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryBasReliefOptions>)
                        return basRelief(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryChalkCharcoalOptions>)
                        return chalkCharcoal(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryCharcoalOptions>)
                        return charcoal(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryChromeOptions>)
                        return chrome(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryConteCrayonOptions>)
                        return conteCrayon(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryGraphicPenOptions>)
                        return graphicPen(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryHalftonePatternOptions>)
                        return halftonePattern(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryNotePaperOptions>)
                        return notePaper(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryPhotocopyOptions>)
                        return photocopy(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryPlasterOptions>)
                        return plaster(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryReticulationOptions>)
                        return reticulation(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryStampOptions>)
                        return stamp(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryTornEdgesOptions>)
                        return tornEdges(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryWaterPaperOptions>)
                        return waterPaper(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryGlowingEdgesOptions>)
                        return glowingEdges(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryCraquelureOptions>)
                        return craquelure(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryGrainOptions>)
                        return grain(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryMosaicTilesOptions>)
                        return mosaicTiles(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryPatchworkOptions>)
                        return patchwork(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryStainedGlassOptions>)
                        return stainedGlass(current.view(), options);
                    else if constexpr (std::is_same_v<O, GalleryTexturizerOptions>)
                        return texturizer(current.view(), options);
                },
                effect);
            if (!result)
                return std::unexpected(result.error());
            current = std::move(*result);
        }
        return current;
    });
    if (!guarded)
        return std::unexpected(guarded.error());
    return std::move(*guarded);
}
} // namespace iiGraphicAdjust
