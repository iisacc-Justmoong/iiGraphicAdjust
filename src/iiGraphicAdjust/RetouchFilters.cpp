#include "DevelopInternal.h"
#include "Internal.h"
#include <array>
#include <iiGraphicAdjust/RetouchFilters.h>
#include <numbers>

namespace iiGraphicAdjust {
namespace {
using Point = std::array<double, 2>;
using Color = std::array<double, 3>;
using Plane = std::vector<double>;
using Matrix = develop_detail::Matrix;
constexpr std::size_t maxPixels = 64000000;
constexpr double pi = std::numbers::pi;
auto bad() {
    return std::unexpected(Error{ErrorCode::InvalidParameter,
                                 "Retouch parameter or work estimate exceeds documented limits"});
}
Result<void> validate(ImageView in) {
    if (auto valid = detail::validate(in); !valid)
        return valid;
    if (in.pixels.size() > maxPixels)
        return std::unexpected(
            Error{ErrorCode::InvalidDimensions, "Retouch filters support at most 64000000 pixels"});
    return {};
}
bool validPoint(Point p, double lo = 0, double hi = 1) {
    return detail::inRange(p[0], lo, hi) && detail::inRange(p[1], lo, hi);
}
double smooth(double a, double b, double x) {
    if (a == b)
        return x >= b ? 1 : 0;
    auto t = detail::clamp((x - a) / (b - a));
    return t * t * (3 - 2 * t);
}
// Same primary/white-point construction as lightcraft_color::RgbSpace::to_xyz.
Matrix space(Color r, Color g, Color b) {
    Matrix m{};
    std::array<Color, 3> p{r, g, b};
    for (int c = 0; c < 3; ++c) {
        m[0][c] = p[c][0] / p[c][1];
        m[1][c] = 1;
        m[2][c] = (1 - p[c][0] - p[c][1]) / p[c][1];
    }
    auto s = develop_detail::multiply(develop_detail::inverse(m),
                                      Color{.3127 / .3290, 1, (1 - .3127 - .3290) / .3290});
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            m[row][col] *= s[col];
    return m;
}
const Matrix &toWorking() {
    static const Matrix m = develop_detail::multiply(
        develop_detail::inverse(space({.708, .292, 0}, {.170, .797, 0}, {.131, .046, 0})),
        space({.64, .33, 0}, {.30, .60, 0}, {.15, .06, 0}));
    return m;
}
const Matrix &toSrgb() {
    static const Matrix m = develop_detail::inverse(toWorking());
    return m;
}
Color decode(Rgba p) {
    if (p.a == 0)
        return {};
    return develop_detail::multiply(
        toWorking(), Color{detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)});
}
Rgba encode(Color c, float alpha) {
    c = develop_detail::multiply(toSrgb(), c);
    return {detail::encoded(c[0]), detail::encoded(c[1]), detail::encoded(c[2]), alpha};
}
double lum(Color c) {
    return .2627 * c[0] + .6780 * c[1] + .0593 * c[2];
}
struct Frame {
    std::size_t w, h;
    std::vector<Color> colors;
    Plane alpha;
};
Frame frame(ImageView in) {
    Frame f{in.width, in.height, {}, {}};
    f.colors.reserve(in.pixels.size());
    f.alpha.reserve(in.pixels.size());
    for (auto p : in.pixels) {
        f.colors.push_back(decode(p));
        f.alpha.push_back(p.a);
    }
    return f;
}
struct Sample {
    Color c{};
    double a = 0;
};
Sample sample(const Frame &f, double x, double y) {
    // Source raster uses continuous pixel centres at (x+.5,y+.5).
    x = std::clamp(x - .5, 0., double(f.w - 1));
    y = std::clamp(y - .5, 0., double(f.h - 1));
    auto ix = std::size_t(x), iy = std::size_t(y);
    double tx = x - ix, ty = y - iy;
    Sample s;
    for (int dy = 0; dy < 2; ++dy)
        for (int dx = 0; dx < 2; ++dx) {
            auto i = std::min(iy + dy, f.h - 1) * f.w + std::min(ix + dx, f.w - 1);
            auto k = (dx ? tx : 1 - tx) * (dy ? ty : 1 - ty) * f.alpha[i];
            s.a += k;
            for (int c = 0; c < 3; ++c)
                s.c[c] += f.colors[i][c] * k;
        }
    if (s.a > 0)
        for (auto &c : s.c)
            c /= s.a;
    return s;
}
// Source raster Gaussian is always three variance-matched box filters, including small sigma.
Plane blur(Plane src, std::size_t w, std::size_t h, double sigma) {
    if (sigma <= .3)
        return src;
    auto lower = std::max(1L, long(std::floor(std::sqrt(4 * sigma * sigma + 1))));
    if (lower % 2 == 0)
        --lower;
    auto count =
        std::clamp(std::lround((12 * sigma * sigma - 3 * double(lower) * lower - 12 * lower - 9) /
                               (-4 * double(lower) - 4)),
                   0L, 3L);
    Plane dst(src.size());
    for (int axis = 0; axis < 2; ++axis)
        for (int pass = 0; pass < 3; ++pass) {
            auto radius = std::size_t((pass < count ? lower : lower + 2) / 2);
            if (!radius)
                continue;
            auto len = axis ? h : w, lines = axis ? w : h;
            for (std::size_t line = 0; line < lines; ++line) {
                auto at = [&](std::size_t p) { return axis ? p * w + line : line * w + p; };
                double sum = (radius + 1) * src[at(0)];
                auto stop = std::min(radius, len - 1);
                for (std::size_t k = 1; k <= stop; ++k)
                    sum += src[at(k)];
                if (radius > len - 1)
                    sum += (radius - len + 1) * src[at(len - 1)];
                for (std::size_t p = 0; p < len; ++p) {
                    dst[at(p)] = sum / double(2 * radius + 1);
                    auto add = radius >= len - 1 - p ? len - 1 : p + radius + 1;
                    auto sub = p < radius ? 0 : p - radius;
                    sum += src[at(add)] - src[at(sub)];
                }
            }
            src.swap(dst);
        }
    return src;
}
Plane normalizedBlur(const Plane &p, const Plane &a, std::size_t w, std::size_t h, double sigma) {
    auto weighted = p;
    for (std::size_t i = 0; i < p.size(); ++i)
        weighted[i] *= a[i];
    auto mass = blur(a, w, h, sigma);
    weighted = blur(std::move(weighted), w, h, sigma);
    for (std::size_t i = 0; i < p.size(); ++i)
        weighted[i] = mass[i] > 1e-12 ? weighted[i] / mass[i] : 0;
    return weighted;
}
using Ring = std::array<Sample, 72>;
std::optional<Ring> ring(const Frame &f, Point center, double r0, double r1) {
    Ring out{};
    std::size_t at = 0;
    for (int i = 0; i < 24; ++i) {
        double angle = i * 2 * pi / 24;
        for (auto radius : {r0, (r0 + r1) / 2, r1}) {
            auto x = center[0] + std::cos(angle) * radius;
            auto y = center[1] + std::sin(angle) * radius;
            if (x < 0 || y < 0 || x >= f.w || y >= f.h)
                return {};
            out[at++] = sample(f, x, y);
        }
    }
    return out;
}
std::optional<double> score(const Frame &f, Point target, Point source, double r) {
    auto a = ring(f, target, 1.05 * r, 1.6 * r);
    auto b = ring(f, source, 1.05 * r, 1.6 * r);
    if (!a || !b)
        return {};
    double s = 0, weight = 0;
    for (std::size_t i = 0; i < a->size(); ++i) {
        auto k = std::min((*a)[i].a, (*b)[i].a);
        weight += k;
        for (int c = 0; c < 3; ++c) {
            auto delta =
                std::sqrt(std::max(0., (*a)[i].c[c])) - std::sqrt(std::max(0., (*b)[i].c[c]));
            s += delta * delta * k;
        }
    }
    if (weight <= 1e-6)
        return {};
    s *= a->size() / weight;
    if (auto inside = ring(f, source, .2 * r, .9 * r)) {
        double mean = 0, mass = 0;
        for (auto p : *a) {
            mean += std::sqrt(std::max(0., p.c[1])) * p.a;
            mass += p.a;
        }
        if (mass > 0) {
            mean /= mass;
            double var = 0, imass = 0;
            for (auto p : *inside) {
                var += std::pow(std::sqrt(std::max(0., p.c[1])) - mean, 2) * p.a;
                imass += p.a;
            }
            if (imass > 0)
                s += var / imass * a->size() * .5;
        }
    }
    return s;
}
std::vector<SpotSource> ranked(const Frame &f, Point target, double r,
                               const std::optional<Point> &avoid = {}) {
    std::vector<SpotSource> out;
    for (double distance : {2.4, 3.2, 4.2})
        for (int i = 0; i < 16; ++i) {
            double angle = i * 2 * pi / 16;
            Point d{std::cos(angle) * r * distance, std::sin(angle) * r * distance};
            if (avoid && std::hypot(d[0] - (*avoid)[0] * f.w, d[1] - (*avoid)[1] * f.h) <= r)
                continue;
            if (auto s = score(f, target, {target[0] + d[0], target[1] + d[1]}, r))
                out.push_back({{d[0] / f.w, d[1] / f.h}, *s});
        }
    std::stable_sort(out.begin(), out.end(), [](auto a, auto b) { return a.score < b.score; });
    return out;
}
bool validSpot(const SpotOptions &o) {
    if (o.points.size() > 4096 || !detail::inRange(o.size, .001, .25) ||
        !detail::inRange(o.feather, 0, 100) || !detail::inRange(o.opacity, 0, 100) ||
        (o.sourceOffset && !validPoint(*o.sourceOffset, -1, 1)))
        return false;
    return std::all_of(o.points.begin(), o.points.end(), [](auto p) { return validPoint(p); });
}
Result<Image> spot(ImageView in, const SpotOptions &o, bool heal) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (!validSpot(o))
        return bad();
    const double radius = std::max(1., o.size * std::max(in.width, in.height));
    double x0 = in.width, y0 = in.height, x1 = 0, y1 = 0;
    for (auto p : o.points) {
        x0 = std::min(x0, p[0] * in.width - 2 * radius);
        y0 = std::min(y0, p[1] * in.height - 2 * radius);
        x1 = std::max(x1, p[0] * in.width + 2 * radius);
        y1 = std::max(y1, p[1] * in.height + 2 * radius);
    }
    auto bx0 = std::size_t(std::clamp(std::floor(x0), 0., double(in.width)));
    auto by0 = std::size_t(std::clamp(std::floor(y0), 0., double(in.height)));
    auto bx1 = std::size_t(std::clamp(std::ceil(x1), 0., double(in.width)));
    auto by1 = std::size_t(std::clamp(std::ceil(y1), 0., double(in.height)));
    if (bx1 < bx0 || by1 < by0)
        return detail::allocated([&] { return detail::copy(in); });
    const auto w = bx1 - bx0, h = by1 - by0;
    if (double(w) * h * o.points.size() > 128000000)
        return bad();
    return detail::allocated([&] {
        auto out = detail::copy(in);
        if (o.points.empty() || o.opacity == 0 || w == 0 || h == 0)
            return out;
        auto f = frame(in);
        Point offset{};
        if (o.sourceOffset)
            offset = {(*o.sourceOffset)[0] * in.width, (*o.sourceOffset)[1] * in.height};
        else {
            auto first = o.points.front();
            auto sources = ranked(f, {first[0] * in.width, first[1] * in.height}, radius);
            offset = sources.empty()
                         ? Point{2.5 * radius, 0}
                         : Point{sources[0].offset[0] * in.width, sources[0].offset[1] * in.height};
        }
        Plane alpha(w * h), outside(w * h);
        std::vector<Sample> src(w * h);
        for (std::size_t y = 0; y < h; ++y)
            for (std::size_t x = 0; x < w; ++x) {
                auto i = y * w + x, original = (y + by0) * in.width + x + bx0;
                double px = x + bx0 + .5, py = y + by0 + .5, distance = 1e100;
                for (auto p : o.points)
                    distance =
                        std::min(distance, std::hypot(px - p[0] * in.width, py - p[1] * in.height));
                alpha[i] = (1 - smooth(radius * (1 - o.feather / 100 * .8), radius, distance)) *
                           o.opacity / 100;
                src[i] = sample(f, px + offset[0], py + offset[1]);
                outside[i] = alpha[i] < .02 ? std::min(f.alpha[original], src[i].a) : 0;
            }
        std::array<Plane, 3> low;
        if (heal) {
            auto mass = blur(outside, w, h, std::max(1., radius * .6));
            for (int c = 0; c < 3; ++c) {
                Plane diff(w * h);
                for (std::size_t y = 0; y < h; ++y)
                    for (std::size_t x = 0; x < w; ++x) {
                        auto i = y * w + x, original = (y + by0) * in.width + x + bx0;
                        diff[i] = (f.colors[original][c] - src[i].c[c]) * outside[i];
                    }
                low[c] = blur(std::move(diff), w, h, std::max(1., radius * .6));
                for (std::size_t i = 0; i < mass.size(); ++i)
                    low[c][i] /= std::max(1e-4, mass[i]);
            }
        }
        for (std::size_t y = 0; y < h; ++y)
            for (std::size_t x = 0; x < w; ++x) {
                auto i = y * w + x, original = (y + by0) * in.width + x + bx0;
                auto a = alpha[i] * src[i].a;
                if (a <= 0 || f.alpha[original] == 0)
                    continue;
                auto c = f.colors[original];
                for (int channel = 0; channel < 3; ++channel) {
                    auto value = src[i].c[channel] + (heal ? low[channel][i] : 0);
                    c[channel] = std::max(0., c[channel] + (value - c[channel]) * a);
                }
                out.pixels[original] = encode(c, in.pixels[original].a);
            }
        return out;
    });
}
bool validEye(const EyeOptions &o) {
    return validPoint(o.center) && detail::inRange(o.radiusX, .0001, .5) &&
           detail::inRange(o.radiusY, .0001, .5) && detail::inRange(o.pupilSize, 0, 100) &&
           detail::inRange(o.darken, 0, 100) && (!o.catchlight || validPoint(*o.catchlight, -1, 1));
}
DetectedPupil pupil(ImageView in, const EyeOptions &o, EyeKind kind) {
    DetectedPupil fallback{o.center, .5 * std::min(o.radiusX, o.radiusY), false};
    struct EyeSample {
        Point n;
        Color c;
        double alpha;
    };
    std::vector<EyeSample> samples;
    samples.reserve(1600);
    double longEdge = std::max(in.width, in.height), mass = 0, mean = 0, ymax = 0;
    for (int y = 0; y < 40; ++y)
        for (int x = 0; x < 40; ++x) {
            double dx = (x + .5) / 40 * 2 - 1, dy = (y + .5) / 40 * 2 - 1;
            if (dx * dx + dy * dy > 1)
                continue;
            Point n{o.center[0] + dx * o.radiusX * longEdge / in.width,
                    o.center[1] + dy * o.radiusY * longEdge / in.height};
            if (n[0] < 0 || n[1] < 0 || n[0] >= 1 || n[1] >= 1)
                continue;
            auto p =
                in.pixels[std::size_t(n[1] * in.height) * in.width + std::size_t(n[0] * in.width)];
            if (p.a == 0)
                continue;
            auto c = decode(p);
            samples.push_back({n, c, p.a});
            mass += p.a;
            mean += lum(c) * p.a;
            ymax = std::max(ymax, lum(c));
        }
    if (samples.empty())
        return fallback;
    if (kind == EyeKind::Pet && (ymax <= 1e-6 || ymax < 1.5 * mean / mass))
        return fallback;
    double total = 0;
    Point center{};
    for (auto s : samples) {
        double weight = 0;
        if (kind == EyeKind::Pet)
            weight = smooth(.5, .8, lum(s.c) / ymax);
        else {
            auto c = s.c;
            if (c[0] > .001)
                weight =
                    smooth(.22, .42, (c[0] - std::max(c[1], c[2])) / (c[0] + c[1] + c[2] + 1e-6));
        }
        weight *= s.alpha;
        total += weight;
        center[0] += s.n[0] * weight;
        center[1] += s.n[1] * weight;
    }
    if (total < 3)
        return fallback;
    for (auto &v : center)
        v /= total;
    return {center, std::sqrt(total / mass * o.radiusX * o.radiusY), true};
}
Result<Image> eye(ImageView in, const EyeOptions &o, EyeKind kind) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (!validEye(o))
        return bad();
    return detail::allocated([&] {
        auto out = detail::copy(in);
        auto detected = pupil(in, o, kind);
        auto radius = std::min(detected.radius * (.5 + o.pupilSize / 100),
                               std::max(o.radiusX, o.radiusY) * 1.1);
        auto r = std::max(.5, radius * std::max(in.width, in.height));
        Point center{detected.center[0] * in.width, detected.center[1] * in.height};
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                if (in.pixels[i].a == 0)
                    continue;
                double px = x + .5, py = y + .5;
                auto d = std::hypot(px - center[0], py - center[1]) / r;
                auto m = 1 - (kind == EyeKind::Pet ? smooth(.95, 1.2, d) : smooth(1, 1.35, d));
                auto c = decode(in.pixels[i]);
                bool changed = false;
                if (m > 0) {
                    if (kind == EyeKind::Pet) {
                        auto k = std::min(lum(c), .04) * (1 - .85 * o.darken / 100);
                        for (auto &v : c)
                            v += (k - v) * m;
                        changed = true;
                    } else {
                        auto gate =
                            smooth(.25, .5, (c[0] - std::max(c[1], c[2])) / std::max(1e-6, c[0]));
                        auto a = m * gate, n = .5 * (c[1] + c[2]) * (1 - .9 * o.darken / 100);
                        for (auto &v : c)
                            v += (n - v) * a;
                        changed = a > 0;
                    }
                }
                if (o.catchlight && kind == EyeKind::Pet) {
                    auto lx =
                        center[0] + (*o.catchlight)[0] * radius * std::max(in.width, in.height);
                    auto ly =
                        center[1] + (*o.catchlight)[1] * radius * std::max(in.width, in.height);
                    auto dc = std::hypot(px - lx, py - ly) / std::max(.5, .22 * r);
                    auto mc = 1 - smooth(.5, 1, dc);
                    for (auto &v : c)
                        v += (.9 - v) * mc;
                    changed |= mc > 0;
                }
                if (changed)
                    out.pixels[i] = encode(c, in.pixels[i].a);
            }
        return out;
    });
}
struct EncodedFrame {
    Plane l, a;
};
EncodedFrame encodedFrame(ImageView in) {
    EncodedFrame f;
    f.l.reserve(in.pixels.size());
    f.a.reserve(in.pixels.size());
    for (auto p : in.pixels) {
        f.l.push_back(p.a > 0 ? .2126 * p.r + .7152 * p.g + .0722 * p.b : 0);
        f.a.push_back(p.a);
    }
    return f;
}
} // namespace
Result<Image> cloneSpot(ImageView in, const SpotOptions &o) {
    return spot(in, o, false);
}
Result<Image> healSpot(ImageView in, const SpotOptions &o) {
    return spot(in, o, true);
}
Result<Image> removeSpot(ImageView in, const SpotOptions &o) {
    return spot(in, o, true);
}
Result<std::vector<SpotSource>> rankSpotSources(ImageView in, const SpotSourceOptions &o) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (!validPoint(o.target) || !detail::inRange(o.size, .001, .25) ||
        (o.avoidOffset && !validPoint(*o.avoidOffset, -1, 1)))
        return bad();
    return detail::allocated([&] {
        return ranked(frame(in), {o.target[0] * in.width, o.target[1] * in.height},
                      std::max(1., o.size * std::max(in.width, in.height)), o.avoidOffset);
    });
}
Result<DetectedPupil> detectPupil(ImageView in, const EyeOptions &o, EyeKind kind) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (!validEye(o) || kind < EyeKind::Red || kind > EyeKind::Pet)
        return bad();
    return detail::allocated([&] { return pupil(in, o, kind); });
}
Result<Image> redEye(ImageView in, const EyeOptions &o) {
    return eye(in, o, EyeKind::Red);
}
Result<Image> petEye(ImageView in, const EyeOptions &o) {
    return eye(in, o, EyeKind::Pet);
}
Result<Image> visualizeSpots(ImageView in, const VisualizeSpotsOptions &o) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (!detail::inRange(o.sensitivity, 0, 100))
        return bad();
    return detail::allocated([&] {
        auto f = encodedFrame(in);
        auto b = normalizedBlur(f.l, f.a, in.width, in.height,
                                std::max(.8, .006 * std::max(in.width, in.height)));
        auto threshold = .01 + .2 * std::pow(1 - o.sensitivity / 100, 2);
        auto out = detail::copy(in);
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            if (in.pixels[i].a > 0) {
                float v = std::abs(f.l[i] - b[i]) > threshold ? 1 : 0;
                out.pixels[i] = {v, v, v, in.pixels[i].a};
            }
        return out;
    });
}
Result<std::vector<DustSpot>> detectDustSpots(ImageView in, const DustDetectionOptions &o) {
    if (auto r = validate(in); !r)
        return std::unexpected(r.error());
    if (!detail::inRange(o.sensitivity, 0, 100) || o.maxSpots < 1 || o.maxSpots > 60)
        return bad();
    return detail::allocated([&] {
        std::vector<DustSpot> out;
        auto w = in.width, h = in.height;
        if (w < 32 || h < 32)
            return out;
        auto f = encodedFrame(in);
        double longEdge = std::max(w, h);
        auto around = normalizedBlur(f.l, f.a, w, h, std::max(2., .012 * longEdge));
        Plane dark(f.l.size()), texture(f.l.size());
        for (std::size_t i = 0; i < dark.size(); ++i) {
            dark[i] = std::max(0., around[i] - f.l[i]);
            texture[i] = std::abs(f.l[i] - around[i]);
        }
        texture = normalizedBlur(texture, f.a, w, h, std::max(3., .02 * longEdge));
        auto minDark = .03 - .022 * o.sensitivity / 100;
        auto candidate = [&](std::size_t i) {
            return f.a[i] > 0 && dark[i] > minDark && dark[i] > 3 * texture[i] && f.l[i] > .08;
        };
        std::vector<bool> seen(dark.size());
        std::vector<std::size_t> stack;
        double rmin = .0015 * longEdge, rmax = .02 * longEdge;
        for (std::size_t start = 0; start < dark.size(); ++start) {
            if (seen[start] || !candidate(start))
                continue;
            stack.clear();
            stack.push_back(start);
            seen[start] = true;
            std::size_t n = 0, x0 = w, x1 = 0, y0 = h, y1 = 0;
            double sx = 0, sy = 0, peak = 0;
            while (!stack.empty()) {
                auto i = stack.back();
                stack.pop_back();
                auto x = i % w, y = i / w;
                ++n;
                sx += x;
                sy += y;
                peak = std::max(peak, dark[i]);
                x0 = std::min(x0, x);
                x1 = std::max(x1, x);
                y0 = std::min(y0, y);
                y1 = std::max(y1, y);
                auto push = [&](std::size_t j) {
                    if (!seen[j] && candidate(j)) {
                        seen[j] = true;
                        stack.push_back(j);
                    }
                };
                if (x)
                    push(i - 1);
                if (x + 1 < w)
                    push(i + 1);
                if (y)
                    push(i - w);
                if (y + 1 < h)
                    push(i + w);
            }
            double bw = x1 - x0 + 1, bh = y1 - y0 + 1, r = std::max(bw, bh) / 2;
            bool round = std::max(bw, bh) / std::min(bw, bh) <= 1.8 && n >= .45 * bw * bh;
            if (n <= rmax * rmax * 4 && r >= rmin && r <= rmax && round)
                out.push_back({{(sx / n + .5) / w, (sy / n + .5) / h}, r / longEdge * 1.6, peak});
        }
        std::stable_sort(out.begin(), out.end(),
                         [](auto a, auto b) { return a.strength > b.strength; });
        if (out.size() > o.maxSpots)
            out.resize(o.maxSpots);
        return out;
    });
}
} // namespace iiGraphicAdjust
