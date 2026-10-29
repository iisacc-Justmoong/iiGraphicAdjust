#include "PixelRenderInternal.h"
#include <functional>
#include <iiGraphicAdjust/RenderFilters.h>
#include <tuple>

namespace iiGraphicAdjust {
namespace {
using namespace pixelrender;
using Point = std::array<double, 2>;
template <class F> Result<Image> render(ImageView in, F f) {
    if (auto r = pixelrender::validate(in); !r)
        return std::unexpected(r.error());
    return detail::allocated([&] { return f(); });
}
Rgba preserve(Color c, Rgba p) {
    return p.a > 0 ? color(c, p.a) : p;
}
double cloudHash(std::int64_t x, std::int64_t y, std::uint32_t seed) {
    std::uint32_t h =
        std::uint32_t(x) * 0x8da6b343u ^ std::uint32_t(y) * 0xd8163841u ^ seed * 0xcb1ab31fu;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return double(h & 0xffffffu) / double(0xffffffu);
}
double cloudValue(double x, double y, double base, std::uint32_t seed) {
    double sum = 0, amp = 1, freq = 1 / base, mass = 0;
    for (std::uint32_t k = 0; k < 8; ++k) {
        double fx = x * freq, fy = y * freq;
        auto ix = std::int64_t(std::floor(fx)), iy = std::int64_t(std::floor(fy));
        double sx = smooth(0, 1, fx - ix), sy = smooth(0, 1, fy - iy);
        auto s = seed + k * 7919u;
        double a = cloudHash(ix, iy, s), b = cloudHash(ix + 1, iy, s), c = cloudHash(ix, iy + 1, s),
               d = cloudHash(ix + 1, iy + 1, s);
        sum += ((a + (b - a) * sx) * (1 - sy) + (c + (d - c) * sx) * sy) * amp;
        mass += amp;
        amp *= .5;
        freq *= 2;
    }
    return detail::clamp((sum / mass - .5) * 2.2 + .5);
}
Rgba between(Rgba a, Rgba b, double t) {
    Average avg;
    avg.add(a, 1 - t);
    avg.add(b, t);
    return avg.result();
}
} // namespace
Result<Image> clouds(ImageView in, const CloudsOptions &o) {
    if (!validColor(o.foreground) || !validColor(o.background))
        return bad();
    return render(in, [&] {
        auto out = detail::output(in);
        double base = std::clamp(double(std::max(in.width, in.height)) / 4, 16., 512.);
        for (std::size_t i = 0; i < in.pixels.size(); ++i)
            out.pixels[i] = between(o.foreground, o.background,
                                    cloudValue(i % in.width, i / in.width, base, o.seed));
        return out;
    });
}
Result<Image> differenceClouds(ImageView in, const CloudsOptions &o) {
    if (!validColor(o.foreground) || !validColor(o.background))
        return bad();
    return render(in, [&] {
        auto out = detail::copy(in);
        double base = std::clamp(double(std::max(in.width, in.height)) / 4, 16., 512.);
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            auto p = in.pixels[i];
            if (p.a <= 0)
                continue;
            auto c = between(o.foreground, o.background,
                             cloudValue(i % in.width, i / in.width, base, o.seed));
            auto rgbp = rgb(p), rgbc = rgb(c);
            for (int ch = 0; ch < 3; ++ch)
                rgbp[ch] = std::abs(rgbp[ch] - rgbc[ch]);
            out.pixels[i] = preserve(rgbp, p);
        }
        return out;
    });
}
Result<Image> fibers(ImageView in, const FibersOptions &o) {
    if (!detail::inRange(o.variance, 1, 64) || !detail::inRange(o.strength, 1, 64) ||
        !validColor(o.foreground) || !validColor(o.background))
        return bad();
    return render(in, [&] {
        auto out = detail::output(in);
        double fx = .08 + o.variance / 64 * .6,
               fy = fx / (2 + o.strength * 1.5) * (.5 + o.variance / 32);
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            double x = i % in.width, y = i / in.width, sum = 0, amp = 1, mass = 0, ax = fx, ay = fy;
            for (std::uint32_t k = 0; k < 4; ++k) {
                sum += noise(x * ax, y * ay, o.seed + k * 977u) * amp;
                mass += amp;
                amp *= .55;
                ax *= 2;
                ay *= 2;
            }
            double grain = noise(x * .9, y * fy * .25, o.seed ^ 0x5bd1u) - .5,
                   t = detail::clamp((sum / mass - .5) * (1.6 + o.variance / 40) + .5 +
                                     grain * .25);
            out.pixels[i] = between(o.foreground, o.background, t);
        }
        return out;
    });
}
Result<Image> lensFlare(ImageView in, const LensFlareOptions &o) {
    if (!detail::inRange(o.brightness, 0, 300) || !detail::inRange(o.centerX, -.5, 1.5) ||
        !detail::inRange(o.centerY, -.5, 1.5) || o.lens < FlareLens::Zoom ||
        o.lens > FlareLens::MoviePrime)
        return bad();
    return render(in, [&] {
        auto out = detail::copy(in);
        if (o.brightness == 0)
            return out;
        double cx = o.centerX * in.width, cy = o.centerY * in.height, mx = in.width / 2.,
               my = in.height / 2.,
               scale = std::max(1., std::hypot(double(in.width), double(in.height))) / 1000,
               bright = o.brightness / 100;
        struct Ghost {
            double t, r;
            Color tint;
            double k;
        };
        std::vector<Ghost> ghosts;
        double core = 14, glow = 90, ring = 0, ringWidth = 0, rays = .5, streak = 0;
        switch (o.lens) {
        case FlareLens::Zoom:
            ghosts = {{.35, 10, {.6, .8, 1}, .18}, {.6, 26, {.5, 1, .6}, .10},
                      {.85, 6, {1, .7, .4}, .25},  {1.25, 40, {.7, .5, 1}, .08},
                      {1.5, 16, {1, .6, .5}, .15}, {1.9, 70, {.5, .7, 1}, .06}};
            break;
        case FlareLens::Prime35:
            core = 12;
            glow = 70;
            ring = 90;
            ringWidth = 6;
            rays = .3;
            ghosts = {{.7, 18, {.6, 1, .7}, .12}, {1.4, 30, {1, .7, .5}, .09}};
            break;
        case FlareLens::Prime105:
            core = 22;
            glow = 130;
            rays = .8;
            ghosts = {{.5, 14, {1, .8, .5}, .12}, {1.6, 50, {.6, .7, 1}, .07}};
            break;
        case FlareLens::MoviePrime:
            core = 10;
            glow = 60;
            rays = .2;
            streak = 1;
            ghosts = {
                {.4, 8, {.5, .7, 1}, .2}, {1, 20, {.4, .6, 1}, .12}, {1.6, 12, {.5, .8, 1}, .16}};
            break;
        }
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            auto p = in.pixels[i];
            if (p.a <= 0)
                continue;
            double fx = i % in.width + .5, fy = i / in.width + .5, dx = fx - cx, dy = fy - cy,
                   d = std::hypot(dx, dy),
                   g = std::exp(-std::pow(d / (core * scale), 2)) +
                       .35 * std::exp(-d / (glow * scale));
            Color f{g, g * .95, g * .85};
            double th = std::atan2(dy, dx), m = std::pow(std::sin(th * 12) * .5 + .5, 8) +
                                                std::pow(std::sin(th * 7 + 1.3) * .5 + .5, 12) * .6;
            double rr = rays * m * std::exp(-d / (glow * 1.6 * scale));
            for (auto &v : f)
                v += rr * .6;
            if (streak > 0) {
                double st = std::exp(-std::abs(dy) / (2.5 * scale)) *
                            std::exp(-std::abs(dx) / (450 * scale));
                f[0] += st * .5;
                f[1] += st * .7;
                f[2] += st;
            }
            if (ring > 0)
                for (int ch = 0; ch < 3; ++ch) {
                    double rd = (d - ring * scale * std::array<double, 3>{1.04, 1, .96}[ch]) /
                                (ringWidth * scale);
                    f[ch] += .18 * std::exp(-rd * rd);
                }
            for (const auto &gh : ghosts) {
                double gx = cx + (mx - cx) * gh.t * 2, gy = cy + (my - cy) * gh.t * 2,
                       gd = std::hypot(fx - gx, fy - gy) / (gh.r * scale);
                if (gd < 1.2) {
                    double e = smooth(1, .85, gd) * (.6 + .4 * gd);
                    for (int ch = 0; ch < 3; ++ch)
                        f[ch] += e * gh.k * gh.tint[ch];
                }
            }
            auto c = rgb(p);
            for (int ch = 0; ch < 3; ++ch)
                c[ch] = 1 - (1 - c[ch]) * (1 - detail::clamp(f[ch] * bright));
            out.pixels[i] = preserve(c, p);
        }
        return out;
    });
}
Result<Image> lightingEffects(ImageView in, const LightingEffectsOptions &o) {
    if (o.lights.size() > 64 || !detail::inRange(o.gloss, -100, 100) ||
        !detail::inRange(o.metallic, -100, 100) || !detail::inRange(o.exposure, -100, 100) ||
        !detail::inRange(o.ambience, -100, 100) || !detail::inRange(o.height, 0, 100) ||
        o.texture < TextureChannel::None || o.texture > TextureChannel::Luminance)
        return bad();
    for (const auto &l : o.lights) {
        if (l.kind < LightKind::Spot || l.kind > LightKind::Infinite ||
            !detail::inRange(l.x, -2, 3) || !detail::inRange(l.y, -2, 3) ||
            !detail::inRange(l.z, 0, 10) || !detail::inRange(l.targetX, -2, 3) ||
            !detail::inRange(l.targetY, -2, 3) || !detail::inRange(l.angle, -180, 180) ||
            !detail::inRange(l.elevation, 0, 90) || !detail::inRange(l.intensity, -100, 100) ||
            !detail::inRange(l.cone, 1, 89) || !detail::inRange(l.hotspot, 0, 100) ||
            !detail::inRange(l.radius, .01, 10))
            return bad();
        for (auto c : l.color)
            if (!detail::inRange(c, 0, 1))
                return bad();
    }
    return render(in, [&] {
        auto out = detail::copy(in);
        double shortSide = std::min(in.width, in.height), hscale = o.height / 100 * shortSide * .06,
               ambient = o.ambience / 100 * .5, metal = (o.metallic + 100) / 200,
               shininess = std::pow(2., 1 + (o.gloss + 100) / 200 * 7),
               specK = (o.gloss + 100) / 200 * .7, expo = std::pow(2., o.exposure / 50);
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            auto p = in.pixels[i];
            if (p.a <= 0)
                continue;
            auto x = std::int64_t(i % in.width), y = std::int64_t(i / in.width);
            auto tex = [&](std::int64_t sx, std::int64_t sy) {
                auto q = visibleAt(in, sx, sy, p);
                double value = 0;
                switch (o.texture) {
                case TextureChannel::Red:
                    value = q.r;
                    break;
                case TextureChannel::Green:
                    value = q.g;
                    break;
                case TextureChannel::Blue:
                    value = q.b;
                    break;
                case TextureChannel::Alpha:
                    value = at(in, sx, sy).a;
                    break;
                case TextureChannel::Luminance:
                    value = lum(q);
                    break;
                default:
                    break;
                }
                return o.whiteIsHigh ? value : 1 - value;
            };
            Color normal{0, 0, 1};
            if (o.texture != TextureChannel::None && o.height > 0)
                normal = norm({-(tex(x + 1, y) - tex(x - 1, y)) * .5 * hscale,
                               -(tex(x, y + 1) - tex(x, y - 1)) * .5 * hscale, 1});
            Color base = rgb(p), lit{ambient, ambient, ambient}, spc{};
            for (const auto &l : o.lights) {
                Color dir{};
                double atten = 1;
                Color pos{l.x * in.width, l.y * in.height, l.z * shortSide};
                Color v{pos[0] - x - .5, pos[1] - y - .5, pos[2]};
                if (l.kind == LightKind::Infinite) {
                    double a = radians(l.angle), e = radians(l.elevation);
                    dir =
                        norm({std::cos(a) * std::cos(e), -std::sin(a) * std::cos(e), std::sin(e)});
                } else {
                    dir = norm(v);
                    if (l.kind == LightKind::Point) {
                        double dist = std::hypot(v[0], v[1]),
                               reach = std::max(1., l.radius * shortSide);
                        atten = std::pow(detail::clamp(1 - dist / reach), 2);
                    } else {
                        auto axis = norm({l.targetX * in.width - pos[0],
                                          l.targetY * in.height - pos[1], -pos[2]});
                        double outer = std::cos(radians(l.cone)),
                               inner = std::cos(radians(l.cone * l.hotspot / 100));
                        atten = smooth(outer, std::max(outer + 1e-4, inner), -dot(dir, axis));
                    }
                }
                double intensity = l.intensity / 50 * atten, ndl = std::max(0., dot(normal, dir));
                auto half = norm({dir[0], dir[1], dir[2] + 1});
                double s = std::pow(std::max(0., dot(normal, half)), shininess) * specK;
                for (int ch = 0; ch < 3; ++ch) {
                    lit[ch] += ndl * intensity * l.color[ch];
                    spc[ch] += s * std::max(0., intensity) *
                               (l.color[ch] * (1 - metal) + base[ch] * metal);
                }
            }
            for (int ch = 0; ch < 3; ++ch)
                base[ch] = (base[ch] * lit[ch] + spc[ch]) * expo;
            out.pixels[i] = preserve(base, p);
        }
        return out;
    });
}
Result<Image> relight(ImageView in, const RelightOptions &o) {
    if (!detail::inRange(o.angle, -180, 180) || !detail::inRange(o.elevation, 0, 90) ||
        !detail::inRange(o.intensity, 0, 100) || !detail::inRange(o.ambient, 0, 100) ||
        !detail::inRange(o.warmth, -100, 100) || !detail::inRange(o.softness, 1, 100))
        return bad();
    return render(in, [&] {
        auto out = detail::copy(in);
        if (o.intensity == 0)
            return out;
        double sigma = std::clamp(o.softness / 100 * .08 * std::min(in.width, in.height), 2., 96.);
        auto step = std::int64_t(std::clamp(std::lround(sigma), 1L, 32L));
        std::vector<double> field(in.pixels.size());
        for (std::size_t i = 0; i < field.size(); ++i) {
            auto p = in.pixels[i];
            field[i] = .2126 * p.r + .7152 * p.g + .0722 * p.b;
        }
        field = blurredVisible(in, std::move(field), sigma);
        auto sampleField = [&](std::int64_t x, std::int64_t y) {
            x = std::clamp<std::int64_t>(x, 0, in.width - 1);
            y = std::clamp<std::int64_t>(y, 0, in.height - 1);
            return field[std::size_t(y) * in.width + x];
        };
        double a = radians(o.angle), e = radians(o.elevation), w = o.warmth / 100;
        Color dir{std::cos(a) * std::cos(e), -std::sin(a) * std::cos(e), std::sin(e)},
            tint = w >= 0 ? Color{1 + .15 * w, 1 - .05 * w, 1 - .25 * w}
                          : Color{1 + .20 * w, 1 + .10 * w, 1 - .15 * w};
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            auto p = in.pixels[i];
            if (p.a <= 0)
                continue;
            auto x = std::int64_t(i % in.width), y = std::int64_t(i / in.width);
            double nx = (sampleField(x - step, y) - sampleField(x + step, y)) * 2,
                   ny = (sampleField(x, y - step) - sampleField(x, y + step)) * 2,
                   shade = o.ambient / 100 +
                           o.intensity / 100 * std::max(0., dot(norm({nx, ny, 1}), dir)),
                   base = std::max(.04, field[i]);
            auto c = rgb(p);
            for (int ch = 0; ch < 3; ++ch)
                c[ch] = c[ch] / base * shade * tint[ch];
            out.pixels[i] = preserve(c, p);
        }
        return out;
    });
}
namespace {
struct Rng {
    std::uint64_t state;
    static Rng seeded(std::uint32_t seed, std::uint64_t salt) {
        return {(std::uint64_t(seed) << 32) ^ (salt * 0x9e3779b97f4a7c15ull) ^
                0x2545f4914f6cdd1dull};
    }
    std::uint64_t next() {
        state += 0x9e3779b97f4a7c15ull;
        auto z = state;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        return z ^ (z >> 31);
    }
    double f() {
        return double(next() >> 40) / 16777216.;
    }
    double sym() {
        return f() * 2 - 1;
    }
    double range(double a, double b) {
        return a + (b - a) * f();
    }
};
double noise1(double x, std::uint64_t seed) {
    auto i = std::int64_t(std::floor(x));
    auto h = [&](std::int64_t k) {
        Rng r{std::uint64_t(k) * 0xd6e8feb86659fd93ull ^ seed};
        return r.sym();
    };
    double t = smooth(0, 1, x - i);
    return h(i) + (h(i + 1) - h(i)) * t;
}
double fbm1(double x, std::uint64_t seed) {
    return (noise1(x, seed) + .5 * noise1(x * 2.13 + 7.1, seed ^ 0xa5a5)) / 1.5;
}
enum class Blend { Normal, Add, Lighten };
struct Primitive {
    bool ellipse = false;
    Point a{}, b{};
    double r0 = 1, r1 = 1, angle = 0, soft = 1;
    Rgba c0{}, c1{};
    Blend blend = Blend::Normal;
};
Primitive capsule(Point a, Point b, double ra, double rb, Rgba c0, Rgba c1) {
    return {false, a, b, ra, rb, 0, 1, c0, c1};
}
Primitive ellipse(Point p, double rx, double ry, double angle, Rgba c0, Rgba c1) {
    return {true, p, {}, rx, ry, angle, 1, c0, c1};
}
Primitive disc(Point p, double r, Rgba c) {
    return ellipse(p, r, r, 0, c, c);
}
Rgba shade(Rgba c, double k) {
    auto values = rgb(c);
    for (auto &v : values)
        v *= k;
    return color(values, c.a);
}
Rgba lerp(Rgba a, Rgba b, double t) {
    auto c = mix(rgb(a), rgb(b), t);
    return color(c, a.a + (b.a - a.a) * t);
}
Image composite(ImageView in, const std::vector<Primitive> &prims) {
    std::vector<std::array<double, 4>> paint(in.pixels.size());
    for (const auto &p : prims) {
        double softness = std::max(1., p.soft), s = softness + 1, x0, y0, x1, y1;
        if (p.ellipse) {
            double co = std::cos(p.angle), si = std::sin(p.angle),
                   ex = std::hypot(p.r0 * co, p.r1 * si) + s,
                   ey = std::hypot(p.r0 * si, p.r1 * co) + s;
            x0 = p.a[0] - ex;
            y0 = p.a[1] - ey;
            x1 = p.a[0] + ex;
            y1 = p.a[1] + ey;
        } else {
            double r = std::max(p.r0, p.r1) + s;
            x0 = std::min(p.a[0], p.b[0]) - r;
            y0 = std::min(p.a[1], p.b[1]) - r;
            x1 = std::max(p.a[0], p.b[0]) + r;
            y1 = std::max(p.a[1], p.b[1]) + r;
        }
        auto ax = std::size_t(std::clamp(std::floor(x0), 0., double(in.width))),
             ay = std::size_t(std::clamp(std::floor(y0), 0., double(in.height))),
             bx = std::size_t(std::clamp(std::ceil(x1), 0., double(in.width))),
             by = std::size_t(std::clamp(std::ceil(y1), 0., double(in.height)));
        for (auto y = ay; y < by; ++y)
            for (auto x = ax; x < bx; ++x) {
                double fx = x + .5, fy = y + .5, cov = 0, t = 0;
                if (p.ellipse) {
                    double si = std::sin(p.angle), co = std::cos(p.angle), dx = fx - p.a[0],
                           dy = fy - p.a[1], u = dx * co + dy * si, v = -dx * si + dy * co,
                           rx = std::max(.05, p.r0), ry = std::max(.05, p.r1),
                           e = std::hypot(u / rx, v / ry);
                    cov = detail::clamp((1 - e) * std::min(rx, ry) / softness + .5) *
                          detail::clamp(2 * std::min(rx, ry));
                    t = std::min(1., e);
                } else {
                    double dx = p.b[0] - p.a[0], dy = p.b[1] - p.a[1], l2 = dx * dx + dy * dy;
                    t = l2 > 1e-9 ? detail::clamp(((fx - p.a[0]) * dx + (fy - p.a[1]) * dy) / l2)
                                  : 0;
                    double d = std::hypot(p.a[0] + dx * t - fx, p.a[1] + dy * t - fy),
                           r = p.r0 + (p.r1 - p.r0) * t;
                    cov =
                        detail::clamp((std::max(.5, r) - d) / softness + .5) * detail::clamp(2 * r);
                }
                if (cov <= 0)
                    continue;
                auto c = lerp(p.c0, p.c1, t);
                double alpha = detail::clamp(c.a * cov);
                if (alpha <= 0)
                    continue;
                auto &dest = paint[y * in.width + x];
                auto values = rgb(c);
                for (int ch = 0; ch < 3; ++ch) {
                    double value = values[ch] * alpha;
                    if (p.blend == Blend::Normal)
                        dest[ch] = value + dest[ch] * (1 - alpha);
                    else if (p.blend == Blend::Add)
                        dest[ch] += value;
                    else
                        dest[ch] = std::max(dest[ch], value);
                }
                dest[3] = p.blend == Blend::Lighten ? std::max(dest[3], alpha)
                                                    : alpha + dest[3] * (1 - alpha);
            }
    }
    auto out = detail::copy(in);
    for (std::size_t i = 0; i < paint.size(); ++i) {
        auto p = in.pixels[i];
        auto t = paint[i];
        if (t[3] <= 0)
            continue;
        double a = t[3] + p.a * (1 - t[3]);
        Color c{};
        auto original = rgb(p);
        for (int ch = 0; ch < 3; ++ch)
            c[ch] = a > 0 ? (t[ch] + original[ch] * p.a * (1 - t[3])) / a : 0;
        out.pixels[i] = clean(color(c, a));
    }
    return out;
}
struct Spine {
    std::vector<Point> points;
    std::vector<double> cum;
    explicit Spine(std::vector<Point> pts) : points(std::move(pts)), cum(points.size()) {
        for (std::size_t i = 1; i < points.size(); ++i)
            cum[i] = cum[i - 1] +
                     std::hypot(points[i][0] - points[i - 1][0], points[i][1] - points[i - 1][1]);
    }
    double length() const {
        return cum.empty() ? 0 : cum.back();
    }
    std::pair<Point, Point> at(double s) const {
        if (points.empty())
            return {{0, 0}, {1, 0}};
        if (points.size() == 1)
            return {points[0], {0, -1}};
        s = std::clamp(s, 0., length());
        auto it = std::upper_bound(cum.begin(), cum.end(), s);
        auto k = std::min(points.size() - 2,
                          std::size_t(std::max(std::ptrdiff_t{0}, it - cum.begin() - 1)));
        auto a = points[k], b = points[k + 1];
        double seg = std::max(1e-6, cum[k + 1] - cum[k]), t = detail::clamp((s - cum[k]) / seg),
               dx = b[0] - a[0], dy = b[1] - a[1], m = std::max(1e-6, std::hypot(dx, dy));
        return {{a[0] + dx * t, a[1] + dy * t}, {dx / m, dy / m}};
    }
};
Rgba flameColor(double t, const std::optional<Rgba> &custom) {
    double a = std::pow(1 - t, .7) * .9 + .1 * (1 - t);
    if (custom) {
        auto c = rgb(*custom), core = mix(c, {1, 1, 1}, .6), rim = c;
        for (auto &v : rim)
            v *= .6;
        return color(t < .35 ? mix(core, c, t / .35) : mix(c, rim, (t - .35) / .65), a * custom->a);
    }
    constexpr std::array<double, 5> positions{0, .2, .45, .75, 1};
    constexpr std::array<Color, 5> colors{
        {{1, .98, .85}, {1, .86, .35}, {1, .55, .08}, {.9, .25, .02}, {.6, .08, 0}}};
    for (std::size_t i = 1; i < 5; ++i)
        if (t <= positions[i])
            return color(mix(colors[i - 1], colors[i],
                             (t - positions[i - 1]) / (positions[i] - positions[i - 1])),
                         a);
    return color(colors.back(), a);
}
void drawFlame(std::vector<Primitive> &out, const Spine &spine, double width, const FlameOptions &o,
               Rng &rng) {
    double len = spine.length();
    if (len < 1)
        return;
    auto lines = std::size_t(std::lround(o.flameLines * .6 + 3));
    auto segs = std::min(
        std::size_t{160},
        std::max(std::array<std::size_t, 5>{6, 10, 16, 24, 36}[o.quality], std::size_t(len / 12)));
    double turb = o.turbulence / 100, jag = o.jaggedness / 100;
    if (o.style == FlameStyle::Violent) {
        turb = std::min(2., turb * 1.8 + .2);
        jag = std::min(2., jag * 1.8 + .15);
    } else if (o.style == FlameStyle::Flat) {
        turb *= .35;
        jag *= .35;
    }
    auto shape = o.randomizeShapes ? FlameShape(rng.next() % 5) : o.shape;
    double opacity = o.opacity / 100, half = std::max(1., width) / 2;
    auto seed = rng.next();
    auto sway = [&](double u) {
        return turb * half * 1.6 * fbm1(u * 1.7, seed ^ 0x5a5a) * std::pow(u, 1.4);
    };
    for (int k = 0; k < 6; ++k) {
        double u = double(k) / 6 * .75 + .05;
        auto [p, d] = spine.at(u * len);
        double off = sway(u), r = half * (1.25 - u) + 2;
        auto c = flameColor(u, o.color);
        auto glow =
            ellipse({p[0] - d[1] * off, p[1] + d[0] * off}, r, r * 1.2, std::atan2(d[1], d[0]),
                    color(rgb(c), .16 * opacity * (o.color ? o.color->a : 1)),
                    color({c.r, c.g * .6, c.b * .3}, 0));
        glow.soft = std::max(2., r);
        glow.blend = Blend::Add;
        out.push_back(glow);
    }
    double lineRadius = std::clamp(half / lines * 2.2, .8, 10.);
    for (std::size_t li = 0; li < lines; ++li) {
        double lane = lines == 1 ? 0 : double(li) / (lines - 1) * 2 - 1;
        auto lseed = seed ^ (std::uint64_t(li) * 0x517cc1b727220aull);
        double start = o.bottomAlignment / 100 * rng.f() * .4,
               reach = (1 - .55 * std::pow(std::abs(lane), 1.5)) * rng.range(.7, 1),
               end = std::clamp(start + reach * (1 - start), start + .05, 1.),
               lr = lineRadius * rng.range(.6, 1.3);
        Point prev{};
        double prevR = 0, prevU = 0;
        for (std::size_t k = 0; k <= segs; ++k) {
            double u = start + (end - start) * k / segs, s = u * len;
            auto [p, d] = spine.at(s);
            double spread = 1;
            switch (shape) {
            case FlameShape::Parallel:
                spread = 1 - .6 * u;
                break;
            case FlameShape::ToCenter:
                spread = 1 - u;
                break;
            case FlameShape::Spread:
                spread = 1 + u;
                break;
            case FlameShape::Oval:
                spread = std::sin(pi * (.15 + .85 * u)) * 1.3;
                break;
            case FlameShape::Pointed:
                spread = std::pow(1 - u, 2);
                break;
            }
            double flick = turb * half * .5 * fbm1(s / (len * .2 + 6) + li * .37, lseed) * u +
                           jag * half * .25 * noise1(s / 5, lseed ^ 0x77) * u,
                   off = lane * half * spread + sway(u) + flick;
            Point q{p[0] - d[1] * off, p[1] + d[0] * off};
            double r = lr * (1 - u * .75),
                   fade = detail::clamp((u - start) / std::max(1e-3, end - start));
            if (k > 0) {
                auto faded = [&](double uu) {
                    auto c = flameColor(uu, o.color);
                    c.a *= float(opacity * (1 - std::pow(fade, 3)) *
                                 std::sqrt(detail::clamp((uu - start) / .08)));
                    return c;
                };
                auto prim = capsule(prev, q, prevR, r, faded(prevU), faded(u));
                prim.soft = 1 + lr * .6;
                prim.blend = Blend::Lighten;
                out.push_back(prim);
            }
            prev = q;
            prevR = r;
            prevU = u;
        }
    }
}
} // namespace
Result<Image> flame(ImageView in, const FlameOptions &o) {
    if (o.type < FlameType::OneAlongPath || o.type > FlameType::OneDirection ||
        o.style < FlameStyle::Normal || o.style > FlameStyle::Flat ||
        o.shape < FlameShape::Parallel || o.shape > FlameShape::Pointed ||
        !detail::inRange(o.length, 1, 1000) || !detail::inRange(o.width, 1, 1000) ||
        !detail::inRange(o.angle, -180, 180) || !detail::inRange(o.interval, 1, 1000) ||
        !detail::inRange(o.turbulence, 0, 100) || !detail::inRange(o.jaggedness, 0, 100) ||
        !detail::inRange(o.opacity, 0, 100) || !detail::inRange(o.flameLines, 1, 100) ||
        !detail::inRange(o.bottomAlignment, 0, 100) || o.quality > 4 ||
        (o.color && !validColor(*o.color)) || o.paths.size() > 256)
        return bad();
    std::size_t count = 0;
    double totalLength = 0;
    for (const auto &path : o.paths) {
        count += path.size();
        if (count > 4096)
            return bad();
        for (std::size_t i = 0; i < path.size(); ++i) {
            for (auto v : path[i])
                if (!detail::inRange(v, -1000000, 1000000))
                    return bad();
            if (i)
                totalLength += std::hypot(path[i][0] - path[i - 1][0], path[i][1] - path[i - 1][1]);
        }
    }
    const double costPerFlame = 7 + std::lround(o.flameLines * .6 + 3) * 160;
    const double estimatedFlames =
        o.type == FlameType::OneAlongPath || o.type == FlameType::CandleLight
            ? double(o.paths.size())
            : totalLength / o.interval + o.paths.size();
    if (estimatedFlames * costPerFlame > 500000)
        return bad();
    return render(in, [&] {
        if (o.opacity == 0)
            return detail::copy(in);
        auto paths = o.paths;
        if (paths.empty()) {
            if (o.type == FlameType::CandleLight)
                paths = {{{in.width * .5, in.height * .75}}};
            else {
                std::vector<Point> path;
                for (int k = 0; k <= 32; ++k) {
                    double t = k / 32.;
                    path.push_back(
                        {in.width * (.15 + .7 * t), in.height * (.8 - .08 * std::sin(pi * t))});
                }
                paths.push_back(std::move(path));
            }
        }
        std::vector<Primitive> prims;
        auto rng = Rng::seeded(o.seed, 0xf1a3);
        double runtimeFlames = 0;
        for (const auto &path : paths) {
            if (!path.empty())
                runtimeFlames +=
                    o.type == FlameType::OneAlongPath || o.type == FlameType::CandleLight
                        ? 1
                        : Spine(path).length() / o.interval + 1;
        }
        if (runtimeFlames * costPerFlame > 500000)
            throw std::length_error("Flame primitive budget exceeded");
        for (const auto &path : paths) {
            if (path.empty())
                continue;
            Spine spine(path);
            double total = spine.length();
            if (o.type == FlameType::OneAlongPath && path.size() >= 2 && total >= 1)
                drawFlame(prims, spine, o.width, o, rng);
            else if (o.type == FlameType::CandleLight || o.type == FlameType::OneAlongPath) {
                auto base = path[0];
                double h = std::min(400., o.length);
                auto copy = o;
                copy.shape = FlameShape::Oval;
                copy.turbulence *= .3;
                copy.jaggedness *= .2;
                drawFlame(prims, Spine({base, {base[0], base[1] - h}}), std::max(4., o.width * .6),
                          copy, rng);
                auto core = flameColor(.05, o.color);
                auto p =
                    ellipse({base[0], base[1] - h * .28}, std::max(1.5, o.width * .12), h * .22, 0,
                            color(rgb(core), .9 * o.opacity / 100 * (o.color ? o.color->a : 1)),
                            color(rgb(core), 0));
                p.soft = 2;
                p.blend = Blend::Add;
                prims.push_back(p);
            } else {
                bool closed = path.size() > 2 && std::abs(path.front()[0] - path.back()[0]) < 1 &&
                              std::abs(path.front()[1] - path.back()[1]) < 1;
                auto number = std::size_t(o.adjustIntervalForLoops && closed
                                              ? std::max(1., std::round(total / o.interval))
                                              : std::floor(total / o.interval) + 1);
                double step = o.adjustIntervalForLoops && closed ? total / number : o.interval;
                if (number * std::lround(o.flameLines * .6 + 3) * 160 > 500000)
                    throw std::length_error("Flame primitive budget exceeded");
                for (std::size_t k = 0; k < number; ++k) {
                    auto [p, d] = spine.at(k * step);
                    Point dir{d[1], -d[0]};
                    if (o.type == FlameType::PathDirections)
                        dir = d;
                    else if (o.type == FlameType::OneDirection)
                        dir = {std::sin(radians(o.angle)), -std::cos(radians(o.angle))};
                    double var = o.type == FlameType::VariousLengths ? .7 : 0, len = o.length;
                    if (o.randomizeLength || var > 0)
                        len *= rng.range(1 - std::max(.4, var), 1 + std::max(.4, var) * .5);
                    drawFlame(prims, Spine({p, {p[0] + dir[0] * len, p[1] + dir[1] * len}}),
                              o.width, o, rng);
                }
            }
        }
        return composite(in, prims);
    });
}
namespace {
enum class Leaf { None, Round, Oval, Needle, Heart, Blossom, Long, Frond };
enum class Habit { Branching, Conifer, Palm, Bamboo };
struct TreeKind {
    const char *name;
    Habit habit;
    unsigned levels, children;
    double spread, ratio, trunk, droop, wiggle, thick;
    Leaf leaf;
    Color leafColor, bark;
    std::optional<Color> accent;
};
constexpr Color brown{.36, .25, .16}, greyBark{.45, .42, .38}, green{.24, .52, .16},
    darkGreen{.12, .36, .14};
const std::array<TreeKind, 34> treeKinds{{
    {"Oak", Habit::Branching, 5, 3, 38.0, 0.72, 0.35, 0.05, 0.35, 0.035, Leaf::Round, green, brown,
     std::nullopt},
    {"Maple", Habit::Branching, 5, 3, 34.0, 0.74, 0.32, 0.0, 0.25, 0.03, Leaf::Heart,
     Color{0.3, 0.55, 0.15}, brown, std::nullopt},
    {"Pine", Habit::Conifer, 3, 2, 70.0, 0.5, 0.15, 0.25, 0.1, 0.025, Leaf::Needle, darkGreen,
     brown, std::nullopt},
    {"Palm", Habit::Palm, 1, 9, 0.0, 0.0, 0.85, 0.0, 0.15, 0.022, Leaf::Frond,
     Color{0.25, 0.5, 0.18}, Color{0.55, 0.45, 0.32}, std::nullopt},
    {"Birch", Habit::Branching, 5, 2, 28.0, 0.75, 0.4, 0.1, 0.3, 0.02, Leaf::Oval,
     Color{0.45, 0.65, 0.2}, Color{0.88, 0.86, 0.82}, std::nullopt},
    {"Weeping Willow", Habit::Branching, 5, 3, 40.0, 0.78, 0.3, 0.9, 0.2, 0.035, Leaf::Long,
     Color{0.45, 0.62, 0.22}, greyBark, std::nullopt},
    {"Cherry Blossom", Habit::Branching, 5, 3, 42.0, 0.7, 0.3, 0.0, 0.45, 0.03, Leaf::Blossom,
     Color{0.98, 0.75, 0.82}, Color{0.3, 0.2, 0.17}, std::nullopt},
    {"Bamboo", Habit::Bamboo, 1, 5, 6.0, 0.0, 1.0, 0.0, 0.05, 0.012, Leaf::Long,
     Color{0.35, 0.6, 0.2}, Color{0.55, 0.65, 0.3}, std::nullopt},
    {"Cypress", Habit::Branching, 5, 3, 10.0, 0.8, 0.1, -0.1, 0.1, 0.02, Leaf::Needle, darkGreen,
     brown, std::nullopt},
    {"Baobab", Habit::Branching, 3, 4, 50.0, 0.55, 0.65, 0.0, 0.3, 0.09, Leaf::Round,
     Color{0.35, 0.55, 0.2}, Color{0.55, 0.47, 0.4}, std::nullopt},
    {"Ginkgo", Habit::Branching, 4, 3, 30.0, 0.72, 0.35, 0.0, 0.2, 0.025, Leaf::Heart,
     Color{0.75, 0.75, 0.2}, greyBark, std::nullopt},
    {"Spruce", Habit::Conifer, 3, 2, 80.0, 0.45, 0.1, 0.45, 0.08, 0.025, Leaf::Needle,
     Color{0.1, 0.3, 0.2}, brown, std::nullopt},
    {"Bare Winter Tree", Habit::Branching, 6, 2, 34.0, 0.74, 0.3, 0.05, 0.35, 0.03, Leaf::None,
     green, Color{0.3, 0.25, 0.22}, std::nullopt},
    {"Apple", Habit::Branching, 5, 3, 46.0, 0.68, 0.3, 0.1, 0.4, 0.03, Leaf::Round, green, brown,
     Color{0.85, 0.1, 0.1}},
    {"Acacia", Habit::Branching, 4, 3, 55.0, 0.75, 0.45, -0.25, 0.25, 0.03, Leaf::Round,
     Color{0.35, 0.5, 0.18}, brown, std::nullopt},
    {"Poplar", Habit::Branching, 5, 3, 12.0, 0.8, 0.15, -0.15, 0.1, 0.025, Leaf::Oval,
     Color{0.3, 0.55, 0.15}, greyBark, std::nullopt},
    {"Elm", Habit::Branching, 5, 2, 24.0, 0.8, 0.3, 0.15, 0.25, 0.032, Leaf::Oval, green, brown,
     std::nullopt},
    {"Magnolia", Habit::Branching, 4, 3, 40.0, 0.7, 0.3, 0.0, 0.3, 0.03, Leaf::Blossom,
     Color{0.98, 0.9, 0.92}, Color{0.35, 0.28, 0.25}, Color{0.85, 0.5, 0.65}},
    {"Banana", Habit::Palm, 1, 7, 0.0, 0.0, 0.6, 0.0, 0.05, 0.04, Leaf::Frond,
     Color{0.45, 0.7, 0.2}, Color{0.5, 0.6, 0.3}, std::nullopt},
    {"Bonsai", Habit::Branching, 4, 3, 60.0, 0.6, 0.45, 0.2, 0.7, 0.05, Leaf::Round, darkGreen,
     Color{0.4, 0.3, 0.22}, std::nullopt},
    {"Jacaranda", Habit::Branching, 5, 3, 44.0, 0.72, 0.3, 0.0, 0.35, 0.03, Leaf::Blossom,
     Color{0.6, 0.45, 0.85}, brown, std::nullopt},
    {"Autumn Maple", Habit::Branching, 5, 3, 34.0, 0.74, 0.32, 0.0, 0.25, 0.03, Leaf::Heart,
     Color{0.85, 0.3, 0.1}, brown, std::nullopt},
    {"Golden Autumn", Habit::Branching, 5, 3, 36.0, 0.72, 0.33, 0.05, 0.3, 0.03, Leaf::Round,
     Color{0.9, 0.7, 0.15}, brown, std::nullopt},
    {"Olive", Habit::Branching, 4, 3, 48.0, 0.68, 0.35, 0.1, 0.6, 0.04, Leaf::Long,
     Color{0.5, 0.58, 0.42}, greyBark, Color{0.25, 0.28, 0.12}},
    {"Fir", Habit::Conifer, 3, 2, 75.0, 0.5, 0.12, 0.15, 0.05, 0.025, Leaf::Needle,
     Color{0.12, 0.38, 0.22}, brown, std::nullopt},
    {"Cedar", Habit::Conifer, 3, 3, 88.0, 0.6, 0.2, 0.0, 0.15, 0.03, Leaf::Needle,
     Color{0.2, 0.4, 0.25}, brown, std::nullopt},
    {"Lemon", Habit::Branching, 4, 3, 45.0, 0.68, 0.3, 0.05, 0.35, 0.03, Leaf::Oval, darkGreen,
     brown, Color{0.98, 0.88, 0.15}},
    {"Wisteria", Habit::Branching, 5, 3, 45.0, 0.72, 0.3, 0.7, 0.3, 0.03, Leaf::Blossom,
     Color{0.7, 0.55, 0.9}, Color{0.35, 0.3, 0.27}, std::nullopt},
    {"Young Sapling", Habit::Branching, 3, 2, 30.0, 0.7, 0.45, 0.0, 0.2, 0.015, Leaf::Oval,
     Color{0.4, 0.7, 0.2}, Color{0.45, 0.35, 0.22}, std::nullopt},
    {"Dead Tree", Habit::Branching, 5, 2, 45.0, 0.68, 0.4, 0.1, 0.8, 0.035, Leaf::None, green,
     Color{0.32, 0.29, 0.26}, std::nullopt},
    {"Coconut Palm", Habit::Palm, 1, 11, 0.0, 0.0, 0.88, 0.0, 0.35, 0.018, Leaf::Frond,
     Color{0.3, 0.55, 0.15}, Color{0.6, 0.5, 0.36}, Color{0.4, 0.3, 0.15}},
    {"Shrub", Habit::Branching, 4, 4, 55.0, 0.7, 0.05, 0.0, 0.35, 0.02, Leaf::Round, green, brown,
     std::nullopt},
    {"Plum Blossom", Habit::Branching, 5, 2, 48.0, 0.7, 0.3, 0.1, 0.6, 0.03, Leaf::Blossom,
     Color{0.95, 0.6, 0.7}, Color{0.25, 0.18, 0.15}, std::nullopt},
    {"Cloud Pine", Habit::Branching, 4, 3, 70.0, 0.65, 0.35, 0.2, 0.5, 0.035, Leaf::Needle,
     Color{0.15, 0.4, 0.2}, Color{0.35, 0.27, 0.2}, std::nullopt},
}};
struct TreeDrawing {
    const TreeKind &kind;
    const TreeOptions &options;
    Rng rng;
    std::vector<Primitive> branches;
    std::vector<std::pair<double, Primitive>> leaves;
    Rgba bark, leaf;
    Point light, centre;
    double leafPixels;
    double lightAt(Point p) {
        if (options.flatShading)
            return 1;
        double dx = p[0] - centre[0], dy = p[1] - centre[1], m = std::max(1., std::hypot(dx, dy));
        return .78 + .32 * (dx * light[0] + dy * light[1]) / m + .12 * rng.sym();
    }
    void addLeaf(Point p, double dir) {
        if (kind.leaf == Leaf::None || options.leavesSize == 0)
            return;
        double s = leafPixels * rng.range(.7, 1.2), a = dir + rng.sym() * 1.2, k = lightAt(p);
        Primitive prim;
        switch (kind.leaf) {
        case Leaf::Round:
            prim = ellipse(p, s, s * .85, a, shade(leaf, k * 1.1), shade(leaf, k * .8));
            break;
        case Leaf::Oval:
            prim = ellipse(p, s * 1.2, s * .55, a, shade(leaf, k * 1.1), shade(leaf, k * .8));
            break;
        case Leaf::Long:
            prim = ellipse(p, s * 1.6, s * .28, a, shade(leaf, k * 1.1), shade(leaf, k * .8));
            break;
        case Leaf::Heart:
            prim = ellipse(p, s * .9, s * 1.1, a, shade(leaf, k * 1.1), shade(leaf, k * .75));
            break;
        case Leaf::Needle:
            for (int j = 0; j < 7; ++j) {
                double angle = a + (j / 6. - .5) * 2.4 + rng.sym() * .15,
                       len = s * rng.range(1.1, 1.7), kk = k + .1 * rng.sym();
                Point q{p[0] + std::cos(angle) * len, p[1] + std::sin(angle) * len};
                leaves.emplace_back(kk, capsule(p, q, std::max(.45, s * .1), std::max(.3, s * .05),
                                                shade(leaf, kk * .85), shade(leaf, kk * 1.1)));
            }
            return;
        case Leaf::Blossom:
            prim = ellipse(p, s * .75, s * .75, 0, shade(leaf, k * 1.08), shade(leaf, k * .85));
            break;
        case Leaf::Frond:
            prim = ellipse(p, s * 1.6, s * .3, a, shade(leaf, k), shade(leaf, k * .8));
            break;
        case Leaf::None:
            return;
        }
        leaves.emplace_back(k + rng.f() * .05, prim);
        if (kind.accent && rng.f() < .12) {
            Point q{p[0] + rng.sym() * s, p[1] + rng.sym() * s};
            double r = std::max(1., s * .45);
            leaves.emplace_back(k + 1, ellipse(q, r, r, 0, color(*kind.accent, leaf.a),
                                               shade(color(*kind.accent, leaf.a), k * .75)));
        }
    }
    std::pair<Point, double> limb(Point p, double angle, double len, double r0, double r1,
                                  double depth) {
        auto segs = std::clamp(std::size_t(len / 6), std::size_t{2}, std::size_t{10});
        double droop = kind.droop * depth;
        auto col = shade(bark, .9 + .2 * depth);
        Point q = p;
        for (std::size_t k = 0; k < segs; ++k) {
            double t0 = double(k) / segs, t1 = double(k + 1) / segs;
            angle += kind.wiggle * rng.sym() * .25;
            double diff = std::atan2(std::sin(pi / 2 - angle), std::cos(pi / 2 - angle));
            angle += diff * droop * .12;
            double step = len / segs;
            Point nq{q[0] + std::cos(angle) * step, q[1] + std::sin(angle) * step};
            branches.push_back(capsule(q, nq, r0 + (r1 - r0) * t0, r0 + (r1 - r0) * t1, col, col));
            if (depth > .99 && kind.leaf != Leaf::None && k > 0 &&
                rng.f() < options.leavesAmount / 100 * .9)
                addLeaf(nq, angle);
            q = nq;
        }
        return {q, angle};
    }
    void branch(Point p, double a, double len, double r, unsigned level) {
        double depth = double(level) / std::max(1u, kind.levels), rEnd = r * .62;
        auto [end, angle] = limb(p, a, len, r, rEnd, depth);
        if (level >= kind.levels) {
            auto n = std::size_t(std::lround(options.leavesAmount / 100 * 9));
            for (std::size_t i = 0; i < n; ++i) {
                double d = len * .5 * rng.f(), th = rng.f() * 2 * pi;
                addLeaf({end[0] + std::cos(th) * d, end[1] + std::sin(th) * d}, angle);
            }
            return;
        }
        double spread = radians(kind.spread);
        for (unsigned c = 0; c < kind.children; ++c) {
            double side = kind.children == 1 ? 0 : double(c) / (kind.children - 1) * 2 - 1,
                   ca = angle + side * spread + rng.sym() * spread * .35,
                   cl = len * kind.ratio * rng.range(.8, 1.1), cr = rEnd * rng.range(.75, .95);
            branch(end, ca, cl, cr, level + 1);
        }
    }
};
} // namespace
Result<Image> tree(ImageView in, const TreeOptions &o) {
    if (o.baseTreeType < 1 || o.baseTreeType > 34 || o.lightDirection < 1 || o.lightDirection > 5 ||
        !detail::inRange(o.leavesAmount, 0, 100) || !detail::inRange(o.leavesSize, 0, 200) ||
        !detail::inRange(o.branchesHeight, 50, 300) ||
        !detail::inRange(o.branchesThickness, 50, 200) || !detail::inRange(o.x, -2, 3) ||
        !detail::inRange(o.y, -2, 3) || !detail::inRange(o.size, .05, 2) ||
        !validColor(o.leavesColor) || !validColor(o.branchesColor))
        return bad();
    return render(in, [&] {
        const auto &kind = treeKinds[o.baseTreeType - 1];
        double h = in.height * o.size;
        Point base{o.x * in.width, o.y * in.height};
        double a = radians(std::array<double, 5>{180, 225, 270, 315, 0}[o.lightDirection - 1]);
        Rgba leaf = o.defaultLeaves ? color(kind.leafColor, 1) : o.leavesColor,
             bark = o.customBranchColor ? o.branchesColor : color(kind.bark, 1);
        double thick = o.branchesThickness / 100, bh = o.branchesHeight / 100, up = -pi / 2,
               r0 = std::max(.8, h * kind.thick * thick);
        TreeDrawing t{kind,
                      o,
                      Rng::seeded(o.seed, 0x7ee + o.baseTreeType),
                      {},
                      {},
                      bark,
                      leaf,
                      {std::cos(a), std::sin(a)},
                      {base[0], base[1] - h * .65},
                      std::max(.6, h * .012 * o.leavesSize / 100)};
        if (kind.habit == Habit::Branching) {
            double trunk = std::min(h * .92, h * kind.trunk * bh);
            auto [top, angle] = t.limb(base, up, std::max(1., trunk), r0, r0 * .75, 0);
            double geo = 0;
            for (unsigned k = 0; k < kind.levels; ++k)
                geo += std::pow(kind.ratio, k);
            double first = std::max(h * .08, h - trunk) / std::max(.5, geo) * 1.05;
            auto children = std::max(2u, kind.children);
            for (unsigned c = 0; c < children; ++c) {
                double side = double(c) / (children - 1) * 2 - 1,
                       ca = angle + side * radians(kind.spread) * .8 + t.rng.sym() * .15,
                       fl = first * t.rng.range(.85, 1.05);
                t.branch(top, ca, fl, r0 * .7, 1);
            }
        } else if (kind.habit == Habit::Conifer) {
            auto top = t.limb(base, up, h, r0, r0 * .1, 0).first;
            double start = std::min(h * .8, h * kind.trunk * bh);
            auto whorls = std::size_t(14 * std::clamp(std::sqrt(h / 300), .6, 2.5));
            double spread = radians(kind.spread);
            for (std::size_t k = 0; k < whorls; ++k) {
                double f = double(k) / whorls, y = base[1] - start - (h - start) * f,
                       x = base[0] + (top[0] - base[0]) * (base[1] - y) / h,
                       span = (h - start) * .42 * std::pow(1 - f, .9) + h * .03;
                for (double side : std::array<double, 2>{-1, 1}) {
                    double ang = up + side * spread + t.rng.sym() * .1;
                    t.branch({x, y}, ang, span, std::max(.6, r0 * .35 * (1 - f)), kind.levels);
                    if (kind.levels >= 2) {
                        Point mid{x + std::cos(ang + side * .5) * span * .5,
                                  y + std::sin(ang + side * .5) * span * .5};
                        t.branch(mid, ang + side * .6, span * .4, std::max(.5, r0 * .15),
                                 kind.levels);
                    }
                }
            }
        } else if (kind.habit == Habit::Palm) {
            double trunk = std::max(1., h * kind.trunk * std::min(1.15, bh)),
                   lean = t.rng.sym() * kind.wiggle;
            Point p = base;
            for (int k = 0; k < 12; ++k) {
                double f = k / 12., ang = up + lean * f;
                Point q{p[0] + std::cos(ang) * trunk / 12, p[1] + std::sin(ang) * trunk / 12};
                double r = r0 * (1 - .35 * f);
                auto ring = shade(bark, k % 2 == 0 ? 1 : .85);
                t.branches.push_back(capsule(p, q, r, r * .97, ring, ring));
                p = q;
            }
            double flen = h * .38 * std::sqrt(std::max(10., o.leavesSize) / 100);
            auto leafletCount = std::size_t(std::lround(o.leavesAmount / 100 * 3));
            if (o.leavesAmount > 0 && o.leavesSize > 0)
                for (unsigned k = 0; k < kind.children; ++k) {
                    double ang =
                        up + (double(k) / kind.children - .5) * pi * 1.7 + t.rng.sym() * .2;
                    Point q = p;
                    for (int s = 0; s < 10; ++s) {
                        double diff = pi / 2 - ang;
                        ang += .06 * std::atan2(std::sin(diff), std::cos(diff));
                        Point nq{q[0] + std::cos(ang) * flen / 10,
                                 q[1] + std::sin(ang) * flen / 10};
                        double kk = t.lightAt(nq);
                        t.leaves.emplace_back(
                            kk - .1, capsule(q, nq, std::max(.6, r0 * .15), std::max(.4, r0 * .1),
                                             shade(leaf, kk * .8), shade(leaf, kk * .8)));
                        for (std::size_t j = 0; j < leafletCount; ++j) {
                            double along = double(j) / leafletCount;
                            Point at{q[0] + (nq[0] - q[0]) * along, q[1] + (nq[1] - q[1]) * along};
                            for (double side : std::array<double, 2>{-1, 1}) {
                                double la = ang + side * 1.1, ll = flen * .22 * (1 - s / 10. * .7);
                                Point tip{at[0] + std::cos(la) * ll,
                                          at[1] + std::sin(la) * ll + ll * .25};
                                t.leaves.emplace_back(kk, capsule(at, tip, std::max(.5, ll * .08),
                                                                  .3, shade(leaf, kk),
                                                                  shade(leaf, kk * .9)));
                            }
                        }
                        q = nq;
                    }
                }
            if (kind.accent)
                for (int i = 0; i < 4; ++i)
                    t.leaves.emplace_back(
                        2., disc({p[0] + t.rng.sym() * r0 * 2, p[1] + t.rng.f() * r0 * 2}, r0 * 1.1,
                                 color(*kind.accent, 1)));
        } else {
            for (unsigned k = 0; k < kind.children; ++k) {
                double x = base[0] + (double(k) - (kind.children - 1) / 2.) * r0 * 5 +
                           t.rng.sym() * r0,
                       hh = h * t.rng.range(.7, 1),
                       ang = up + t.rng.sym() * radians(kind.spread) * .5;
                Point p{x, base[1]};
                for (int j = 0; j < 8; ++j) {
                    Point q{p[0] + std::cos(ang) * hh / 8, p[1] + std::sin(ang) * hh / 8};
                    t.branches.push_back(capsule(p, q, r0, r0, bark, shade(bark, .85)));
                    t.branches.push_back(capsule({q[0] - r0 * 1.1, q[1]}, {q[0] + r0 * 1.1, q[1]},
                                                 r0 * .25, r0 * .25, shade(bark, .7),
                                                 shade(bark, .7)));
                    if (j >= 8 / 3) {
                        auto n = std::size_t(std::lround(o.leavesAmount / 100 * 4));
                        for (std::size_t l = 0; l < n; ++l) {
                            double side = t.rng.f() < .5 ? -1 : 1,
                                   la = ang + side * t.rng.range(.6, 1.4), ll = t.leafPixels * 2.2;
                            t.addLeaf({q[0] + std::cos(la) * ll, q[1] + std::sin(la) * ll}, la);
                        }
                    }
                    p = q;
                }
            }
        }
        std::stable_sort(t.leaves.begin(), t.leaves.end(),
                         [](const auto &a, const auto &b) { return a.first < b.first; });
        for (auto &p : t.leaves)
            t.branches.push_back(p.second);
        return composite(in, t.branches);
    });
}
namespace {
struct Border {
    double x, y, w, h;
    double length() const {
        return 2 * (w + h);
    }
    std::tuple<Point, Point, Point> at(double s) const {
        s = std::fmod(s, length());
        if (s < 0)
            s += length();
        if (s < w)
            return {{x + s, y}, {1, 0}, {0, 1}};
        if (s < w + h)
            return {{x + w, y + s - w}, {0, 1}, {-1, 0}};
        if (s < 2 * w + h)
            return {{x + w - (s - w - h), y + h}, {-1, 0}, {0, -1}};
        return {{x, y + h - (s - 2 * w - h)}, {0, -1}, {1, 0}};
    }
};
void flowerPrim(std::vector<Primitive> &out, Point c, double r, unsigned petals, Rgba petal,
                Rgba centre, double rotation) {
    for (unsigned k = 0; k < petals; ++k) {
        double a = rotation + double(k) / petals * 2 * pi;
        Point p{c[0] + std::cos(a) * r * .55, c[1] + std::sin(a) * r * .55};
        out.push_back(ellipse(p, r * .5, r * .26, a, petal, shade(petal, .8)));
    }
    out.push_back(ellipse(c, r * .28, r * .28, 0, centre, shade(centre, .75)));
}
void leafPrim(std::vector<Primitive> &out, Point p, double a, double len, Rgba c) {
    Point at{p[0] + std::cos(a) * len * .5, p[1] + std::sin(a) * len * .5};
    out.push_back(ellipse(at, len * .5, len * .22, a, shade(c, 1.12), shade(c, .8)));
}
void starPrim(std::vector<Primitive> &out, Point p, double r, unsigned count, Rgba c,
              double rotation) {
    for (unsigned k = 0; k < count; ++k) {
        double a = rotation + double(k) / count * 2 * pi;
        out.push_back(
            capsule(p, {p[0] + std::cos(a) * r, p[1] + std::sin(a) * r}, r * .28, .4, c, c));
    }
}
} // namespace
Result<Image> pictureFrame(ImageView in, const PictureFrameOptions &o) {
    if (o.style < PictureFrameStyle::VineWithFlowers || o.style > PictureFrameStyle::Hearts ||
        !detail::inRange(o.margin, 0, 30) || !detail::inRange(o.size, 1, 100) ||
        !detail::inRange(o.arrangement, 1, 100) || !detail::inRange(o.thickness, 1, 100) ||
        !detail::inRange(o.fade, 0, 100) || o.lines < 1 || o.lines > 5 ||
        !validColor(o.vineColor) || !validColor(o.flowerColor) || !validColor(o.leafColor))
        return bad();
    if (auto r = pixelrender::validate(in); !r)
        return std::unexpected(r.error());
    double shortSide = std::max(8., double(std::min(in.width, in.height))),
           margin = shortSide * o.margin / 100, unit = shortSide * (.008 + o.size / 100 * .05),
           thick = std::max(.6, unit * .04 + shortSide * .00012 * o.thickness),
           spacing = unit * (4 - 3.2 * o.arrangement / 100), inset = margin + unit;
    Border b{inset, inset, std::max(1., in.width - 2 * inset), std::max(1., in.height - 2 * inset)};
    double total = b.length();
    const auto count = std::uint64_t(std::max(4., std::round(total / spacing))),
               vineSegments = std::uint64_t(std::max(8., std::ceil(total / 3))),
               lineSegments = std::uint64_t(std::max(8., std::ceil(total / 2)));
    // Count the actual style recipe before allocating or generating any geometry.
    // Color opacity and fading do not waive the same work limit used by Flame.
    std::uint64_t primitives = 0;
    switch (o.style) {
    case PictureFrameStyle::VineWithFlowers:
        primitives = vineSegments + count + ((count + 1) / 2) * 6;
        break;
    case PictureFrameStyle::VineWithLeaves:
        primitives = vineSegments + count * 2;
        break;
    case PictureFrameStyle::Ivy:
        primitives = vineSegments + count * 3;
        break;
    case PictureFrameStyle::Roses:
        primitives = vineSegments + count + ((count + 2) / 3) * 4;
        break;
    case PictureFrameStyle::Daisies:
        primitives = vineSegments + count + (count / 2) * 13;
        break;
    case PictureFrameStyle::Berries:
        primitives = vineSegments + count * 4;
        break;
    case PictureFrameStyle::Bamboo:
        for (double len : std::array<double, 4>{b.w, b.h, b.w, b.h}) {
            auto joints = std::uint64_t(std::max(1., std::round(len / (spacing * 2))));
            primitives += joints * 2 + std::max(std::uint64_t{1}, joints / 2);
        }
        break;
    case PictureFrameStyle::Waves:
    case PictureFrameStyle::Zigzag:
        primitives = o.lines * lineSegments;
        break;
    case PictureFrameStyle::Rope:
        primitives = o.lines * (lineSegments / 2 + 1);
        break;
    case PictureFrameStyle::DoubleLine:
        primitives = o.lines * (lineSegments + 4) + 4;
        break;
    case PictureFrameStyle::Dots:
        primitives = count;
        break;
    case PictureFrameStyle::Scallops:
        primitives = count * 8;
        break;
    case PictureFrameStyle::Snowflakes:
        primitives = count * 18;
        break;
    case PictureFrameStyle::Stars:
        primitives = count * 5;
        break;
    case PictureFrameStyle::Hearts:
        primitives = count * 2;
        break;
    }
    if (primitives > 500000)
        return std::unexpected(
            Error{ErrorCode::InvalidParameter, "Picture Frame exceeds 500000 primitives"});
    return detail::allocated([&] {
        std::vector<Primitive> out;
        out.reserve(primitives);
        double step = total / count;
        auto rng = Rng::seeded(o.seed, 0xf8a3e + unsigned(o.style) + 1);
        Rgba vine = o.vineColor, fl = o.flowerColor, lf = o.leafColor, white{1, 1, 1, 1},
             yellow{.98F, .82F, .2F, 1};
        auto wave = [&](double s, double amp, double wavelength) {
            return amp * std::sin(s / wavelength * 2 * pi);
        };
        auto vinePath = [&](double amp, double wavelength, Rgba col, double r) {
            auto n = vineSegments;
            Point prev{};
            for (std::size_t k = 0; k <= n; ++k) {
                double s = double(k) / n * total;
                auto [p, t, normal] = b.at(s);
                double off = wave(s, amp, wavelength);
                Point q{p[0] + normal[0] * off, p[1] + normal[1] * off};
                if (k)
                    out.push_back(capsule(prev, q, r, r, col, col));
                prev = q;
            }
        };
        if (o.style <= PictureFrameStyle::Berries) {
            double amp = unit * .5, wl = step * 2;
            vinePath(amp, wl, vine, thick);
            for (std::size_t k = 0; k < count; ++k) {
                double s = k * step + step * .5;
                auto [p, t, normal] = b.at(s);
                double off = wave(s, amp, wl);
                Point q{p[0] + normal[0] * off, p[1] + normal[1] * off};
                double side = k % 2 == 0 ? 1 : -1, ta = std::atan2(t[1], t[0]),
                       la = ta + side * .9 + rng.sym() * .3, ll = unit * rng.range(.9, 1.2);
                if (o.style == PictureFrameStyle::VineWithFlowers) {
                    leafPrim(out, q, la, ll, lf);
                    if (k % 2 == 0)
                        flowerPrim(out,
                                   {q[0] + normal[0] * unit * .7 * side,
                                    q[1] + normal[1] * unit * .7 * side},
                                   unit * .65, 5, fl, yellow, rng.f() * 6.3);
                } else if (o.style == PictureFrameStyle::VineWithLeaves) {
                    leafPrim(out, q, la, ll, lf);
                    leafPrim(out, q, ta - side * .9, ll * .8, lf);
                } else if (o.style == PictureFrameStyle::Ivy) {
                    for (int j = 0; j < 3; ++j)
                        leafPrim(out, q, la + (j - 1) * .7, ll * .75, shade(lf, .75 + .15 * j));
                } else if (o.style == PictureFrameStyle::Roses) {
                    leafPrim(out, q, la, ll, lf);
                    if (k % 3 == 0) {
                        double r = unit * .7;
                        auto factors = std::array<double, 4>{1, .75, .5, .28};
                        for (std::size_t j = 0; j < 4; ++j)
                            out.push_back(ellipse(q, r * factors[j], r * factors[j] * .9,
                                                  rng.f() * 3, shade(fl, .75 + .1 * j),
                                                  shade(fl, .55)));
                    }
                } else if (o.style == PictureFrameStyle::Daisies) {
                    leafPrim(out, q, la, ll * .8, lf);
                    if (k % 2 == 1)
                        flowerPrim(out, q, unit * .6, 12, white, yellow, rng.f());
                } else {
                    leafPrim(out, q, la, ll, lf);
                    for (int j = 0; j < 3; ++j)
                        out.push_back(
                            ellipse({q[0] + rng.sym() * unit * .4, q[1] + rng.sym() * unit * .4},
                                    unit * .2, unit * .2, 0, shade(fl, 1.2), shade(fl, .6)));
                }
            }
        } else if (o.style == PictureFrameStyle::Bamboo) {
            for (int edge = 0; edge < 4; ++edge) {
                double s0 = std::array<double, 4>{0, b.w, b.w + b.h, 2 * b.w + b.h}[edge],
                       len = edge % 2 == 0 ? b.w : b.h;
                auto a = std::get<0>(b.at(s0 + .01)), z = std::get<0>(b.at(s0 + len - .01));
                auto joints = std::size_t(std::max(1., std::round(len / (spacing * 2))));
                for (std::size_t j = 0; j < joints; ++j) {
                    double f0 = double(j) / joints, f1 = double(j + 1) / joints;
                    Point p0{a[0] + (z[0] - a[0]) * f0, a[1] + (z[1] - a[1]) * f0},
                        p1{a[0] + (z[0] - a[0]) * f1, a[1] + (z[1] - a[1]) * f1};
                    out.push_back(capsule(p0, p1, unit * .35, unit * .35, vine, shade(vine, .8)));
                    out.push_back(disc(p1, unit * .4, shade(vine, .7)));
                }
                for (std::size_t j = 0; j < std::max(std::size_t{1}, joints / 2); ++j) {
                    double f = rng.f();
                    leafPrim(out, {a[0] + (z[0] - a[0]) * f, a[1] + (z[1] - a[1]) * f},
                             rng.f() * 6.3, unit * 1.4, lf);
                }
            }
        } else if (o.style == PictureFrameStyle::Waves || o.style == PictureFrameStyle::Zigzag ||
                   o.style == PictureFrameStyle::DoubleLine || o.style == PictureFrameStyle::Rope) {
            for (std::uint32_t line = 0; line < o.lines; ++line) {
                double offset = (double(line) - (o.lines - 1) / 2.) * thick * 4;
                auto n = lineSegments;
                Point prev{};
                for (std::size_t k = 0; k <= n; ++k) {
                    double s = double(k) / n * total;
                    auto [p, t, normal] = b.at(s);
                    double off = offset;
                    if (o.style == PictureFrameStyle::Waves)
                        off += wave(s + line * step * .25, unit * .4, step);
                    else if (o.style == PictureFrameStyle::Zigzag) {
                        double ph = s / step - std::floor(s / step);
                        off += unit * .4 * (4 * std::abs(ph - .5) - 1);
                    }
                    Point q{p[0] + normal[0] * off, p[1] + normal[1] * off};
                    if (o.style == PictureFrameStyle::Rope) {
                        if (k % 2 == 0)
                            out.push_back(ellipse(q, unit * .45, unit * .2,
                                                  std::atan2(t[1], t[0]) + .7, shade(vine, 1.1),
                                                  shade(vine, .65)));
                    } else if (k)
                        out.push_back(capsule(prev, q, thick, thick, vine, vine));
                    prev = q;
                }
                if (o.style == PictureFrameStyle::DoubleLine) {
                    Border inner{b.x + unit * .6 + offset, b.y + unit * .6 + offset,
                                 std::max(1., b.w - 2 * (unit * .6 + offset)),
                                 std::max(1., b.h - 2 * (unit * .6 + offset))};
                    for (int e = 0; e < 4; ++e) {
                        double s0 = std::array<double, 4>{0, inner.w, inner.w + inner.h,
                                                          2 * inner.w + inner.h}[e],
                               s1 = std::array<double, 4>{inner.w, inner.w + inner.h,
                                                          2 * inner.w + inner.h,
                                                          inner.length() - .001}[e];
                        out.push_back(capsule(std::get<0>(inner.at(s0)), std::get<0>(inner.at(s1)),
                                              thick * .6, thick * .6, vine, vine));
                    }
                }
            }
            if (o.style == PictureFrameStyle::DoubleLine)
                for (int e = 0; e < 4; ++e)
                    out.push_back(ellipse(std::get<0>(b.at(std::array<double, 4>{
                                              0, b.w, b.w + b.h, 2 * b.w + b.h}[e])),
                                          unit * .45, unit * .45, pi / 4, fl, fl));
        } else {
            for (std::size_t k = 0; k < count; ++k) {
                double s = k * step;
                auto [p, t, normal] = b.at(s);
                double ta = std::atan2(t[1], t[0]);
                if (o.style == PictureFrameStyle::Dots) {
                    double r = unit * (k % 2 == 0 ? .4 : .22);
                    out.push_back(ellipse(p, r, r, 0, shade(fl, 1.15), shade(fl, .75)));
                } else if (o.style == PictureFrameStyle::Scallops) {
                    auto c0 = std::get<0>(b.at(s - step * .5));
                    Point mid{(c0[0] + p[0]) / 2, (c0[1] + p[1]) / 2};
                    auto pt = [&](double a) {
                        double r = step * .5;
                        return Point{mid[0] - t[0] * r * std::cos(a) - normal[0] * r * std::sin(a),
                                     mid[1] - t[1] * r * std::cos(a) - normal[1] * r * std::sin(a)};
                    };
                    for (int j = 0; j < 8; ++j)
                        out.push_back(capsule(pt(pi * j / 8), pt(pi * (j + 1) / 8), thick, thick,
                                              vine, vine));
                } else if (o.style == PictureFrameStyle::Snowflakes) {
                    double r = unit * .6;
                    Rgba c{.85F, .92F, 1, 1};
                    for (int j = 0; j < 6; ++j) {
                        double a = ta + j * pi / 3;
                        Point end{p[0] + std::cos(a) * r, p[1] + std::sin(a) * r};
                        out.push_back(capsule(p, end, std::max(.6, thick * .6),
                                              std::max(.5, thick * .4), c, c));
                        Point m{p[0] + std::cos(a) * r * .6, p[1] + std::sin(a) * r * .6};
                        for (double sd : std::array<double, 2>{-.7, .7})
                            out.push_back(capsule(m,
                                                  {m[0] + std::cos(a + sd) * r * .3,
                                                   m[1] + std::sin(a + sd) * r * .3},
                                                  std::max(.5, thick * .4), .4, c, c));
                    }
                } else if (o.style == PictureFrameStyle::Stars)
                    starPrim(out, p, unit * .55, 5, k % 2 == 0 ? yellow : fl, ta - pi / 2);
                else {
                    double r = unit * .3;
                    Point l{p[0] - t[0] * r * .8 - normal[0] * r * .3,
                            p[1] - t[1] * r * .8 - normal[1] * r * .3},
                        right{p[0] + t[0] * r * .8 - normal[0] * r * .3,
                              p[1] + t[1] * r * .8 - normal[1] * r * .3},
                        tip{p[0] + normal[0] * r * 1.6, p[1] + normal[1] * r * 1.6};
                    out.push_back(capsule(l, tip, r, .6, fl, fl));
                    out.push_back(capsule(right, tip, r, .6, fl, fl));
                }
            }
        }
        if (o.fade > 0) {
            double cx = in.width / 2., cy = in.height / 2.;
            for (auto &p : out) {
                auto c = p.a;
                double edge = detail::clamp(std::max(std::abs(c[0] - cx) / (b.w / 2),
                                                     std::abs(c[1] - cy) / (b.h / 2))),
                       k = detail::clamp(1 - o.fade / 100 * (1 - edge) * 8);
                p.c0.a *= float(k);
                p.c1.a *= float(k);
            }
        }
        return composite(in, out);
    });
}
} // namespace iiGraphicAdjust
