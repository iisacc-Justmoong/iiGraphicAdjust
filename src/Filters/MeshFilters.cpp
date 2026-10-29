// Photocraft mesh/plane formulas (MIT), translated to C++23; see docs/MeshFilters.md.
#include "FilterInternal.h"
#include <functional>
#include <iiGraphicAdjust/MeshFilters.h>
#include <numeric>
#include <tuple>

namespace iiGraphicAdjust {
namespace {
namespace fd = filter_detail;
using P = MeshPoint;
using V3 = std::array<double, 3>;
using Tri = std::array<std::size_t, 3>;
using Values = std::vector<double>;
constexpr double limit = 1000000;
constexpr std::size_t maximumNodes = 16384;
P add(P a, P b) {
    return {a[0] + b[0], a[1] + b[1]};
}
P sub(P a, P b) {
    return {a[0] - b[0], a[1] - b[1]};
}
P mul(P a, double k) {
    return {a[0] * k, a[1] * k};
}
double dot(P a, P b) {
    return a[0] * b[0] + a[1] * b[1];
}
double cross(P a, P b) {
    return a[0] * b[1] - a[1] * b[0];
}
double distance(P a, P b) {
    return std::hypot(a[0] - b[0], a[1] - b[1]);
}
bool point(P p) {
    return detail::inRange(p[0], -limit, limit) && detail::inRange(p[1], -limit, limit);
}
bool interpolation(MeshInterpolation i) {
    return static_cast<unsigned>(i) <= 2;
}
Result<void> imageValid(ImageView image) {
    if (auto r = detail::validate(image); !r)
        return r;
    if (image.width > std::size_t(limit) || image.height > std::size_t(limit))
        return std::unexpected(
            Error{ErrorCode::InvalidDimensions,
                  "Mesh filters support dimensions up to one million pixels per axis"});
    return {};
}
template <class F> auto guardedResult(F fn) -> decltype(fn()) {
    auto guarded = detail::allocated(fn);
    if (!guarded)
        return std::unexpected(guarded.error());
    return std::move(*guarded);
}
bool quadValid(const MeshQuad &q) {
    double sign = 0;
    for (int k = 0; k < 4; ++k) {
        if (!point(q[k]))
            return false;
        const auto c = cross(sub(q[(k + 1) % 4], q[k]), sub(q[(k + 2) % 4], q[(k + 1) % 4]));
        if (std::abs(c) < 1e-8)
            return false;
        if (sign && c * sign < 0)
            return false;
        sign = c;
    }
    return true;
}
struct H {
    std::array<double, 9> m{};
    P apply(P p) const {
        const auto z = m[6] * p[0] + m[7] * p[1] + m[8];
        if (std::abs(z) < 1e-12)
            return {NAN, NAN};
        return {(m[0] * p[0] + m[1] * p[1] + m[2]) / z, (m[3] * p[0] + m[4] * p[1] + m[5]) / z};
    }
    std::optional<H> inverse() const {
        const auto &a = m;
        H out{{a[4] * a[8] - a[5] * a[7], a[2] * a[7] - a[1] * a[8], a[1] * a[5] - a[2] * a[4],
               a[5] * a[6] - a[3] * a[8], a[0] * a[8] - a[2] * a[6], a[2] * a[3] - a[0] * a[5],
               a[3] * a[7] - a[4] * a[6], a[1] * a[6] - a[0] * a[7], a[0] * a[4] - a[1] * a[3]}};
        const auto det = a[0] * out.m[0] + a[1] * out.m[3] + a[2] * out.m[6];
        if (std::abs(det) < 1e-12 || !std::isfinite(det))
            return {};
        for (auto &v : out.m)
            v /= det;
        return out;
    }
};
std::optional<H> unitQuad(const MeshQuad &q) {
    if (!quadValid(q))
        return {};
    const auto dx1 = q[1][0] - q[2][0], dx2 = q[3][0] - q[2][0],
               sx = q[0][0] - q[1][0] + q[2][0] - q[3][0], dy1 = q[1][1] - q[2][1],
               dy2 = q[3][1] - q[2][1], sy = q[0][1] - q[1][1] + q[2][1] - q[3][1];
    double g = 0, h = 0;
    if (std::abs(sx) + std::abs(sy) > 1e-12) {
        const auto det = dx1 * dy2 - dx2 * dy1;
        if (std::abs(det) < 1e-12)
            return {};
        g = (sx * dy2 - dx2 * sy) / det;
        h = (dx1 * sy - sx * dy1) / det;
    }
    return H{{q[1][0] - q[0][0] + g * q[1][0], q[3][0] - q[0][0] + h * q[3][0], q[0][0],
              q[1][1] - q[0][1] + g * q[1][1], q[3][1] - q[0][1] + h * q[3][1], q[0][1], g, h, 1}};
}
std::array<double, 4> cubic(double t) {
    const auto t2 = t * t, t3 = t2 * t;
    return {-.5 * t3 + t2 - .5 * t, 1.5 * t3 - 2.5 * t2 + 1, -1.5 * t3 + 2 * t2 + .5 * t,
            .5 * t3 - .5 * t2};
}
Rgba sample(ImageView image, P p, MeshInterpolation mode, fd::Edge edge) {
    if (!std::isfinite(p[0]) || !std::isfinite(p[1]))
        return {0, 0, 0, 0};
    if (mode == MeshInterpolation::Bilinear)
        return fd::straight(fd::sample(image, p[0], p[1], edge));
    auto x = p[0] - .5, y = p[1] - .5;
    if (edge == fd::Edge::Repeat) {
        x = std::clamp(x, 0., double(image.width - 1));
        y = std::clamp(y, 0., double(image.height - 1));
    } else if (x < -2 || y < -2 || x > double(image.width) + 1 || y > double(image.height) + 1)
        return {0, 0, 0, 0};
    const auto snap = [](double a) {
        return std::abs(a - std::round(a)) < 1e-7 ? std::round(a) : a;
    };
    x = snap(x);
    y = snap(y);
    if (mode == MeshInterpolation::Nearest)
        return fd::straight(fd::premul(fd::get(image, static_cast<long long>(std::floor(x + .5)),
                                               static_cast<long long>(std::floor(y + .5)), edge)));
    const auto ix = static_cast<long long>(std::floor(x)),
               iy = static_cast<long long>(std::floor(y));
    const auto wx = cubic(x - ix), wy = cubic(y - iy);
    fd::Pixel out{};
    for (int j = 0; j < 4; ++j)
        for (int i = 0; i < 4; ++i) {
            const auto weight = wx[i] * wy[j];
            if (weight == 0)
                continue;
            const auto p4 = fd::premul(fd::get(image, ix + i - 1, iy + j - 1, edge));
            for (int c = 0; c < 4; ++c)
                out[c] += weight * p4[c];
        }
    return fd::straight(out);
}
bool samePoint(P a, P b, double eps = 1e-9) {
    return std::abs(a[0] - b[0]) < eps && std::abs(a[1] - b[1]) < eps;
}
Image renderMesh(ImageView image, const MeshGeometry &mesh, MeshInterpolation mode) {
    Image out{image.width, image.height, std::vector<Rgba>(image.pixels.size(), Rgba{0, 0, 0, 0})};
    for (auto t : mesh.triangles) {
        const auto a = mesh.destination[t[0]], b = mesh.destination[t[1]],
                   c = mesh.destination[t[2]], sa = mesh.source[t[0]], sb = mesh.source[t[1]],
                   sc = mesh.source[t[2]];
        const auto det = cross(sub(b, a), sub(c, a));
        if (!std::isfinite(det) || std::abs(det) < 1e-12)
            continue;
        const auto loX = std::size_t(std::clamp(std::floor(std::min({a[0], b[0], c[0]}) - .5), 0.,
                                                double(image.width))),
                   loY = std::size_t(std::clamp(std::floor(std::min({a[1], b[1], c[1]}) - .5), 0.,
                                                double(image.height))),
                   hiX = std::size_t(std::clamp(std::ceil(std::max({a[0], b[0], c[0]}) + .5), 0.,
                                                double(image.width))),
                   hiY = std::size_t(std::clamp(std::ceil(std::max({a[1], b[1], c[1]}) + .5), 0.,
                                                double(image.height)));
        for (auto y = loY; y < hiY; ++y)
            for (auto x = loX; x < hiX; ++x) {
                const P p{x + .5, y + .5};
                const auto l1 = cross(sub(p, a), sub(c, a)) / det,
                           l2 = cross(sub(b, a), sub(p, a)) / det, l0 = 1 - l1 - l2;
                if (l0 < -1e-9 || l1 < -1e-9 || l2 < -1e-9)
                    continue;
                const auto source = add(add(mul(sa, l0), mul(sb, l1)), mul(sc, l2));
                const auto i = y * image.width + x;
                out.pixels[i] = samePoint(source, p, 1e-7)
                                    ? image.pixels[i]
                                    : sample(image, source, mode, fd::Edge::Transparent);
            }
    }
    return out;
}
MeshGeometry regular(std::size_t w, std::size_t h, std::size_t nx, std::size_t ny) {
    MeshGeometry mesh;
    for (std::size_t y = 0; y <= ny; ++y)
        for (std::size_t x = 0; x <= nx; ++x)
            mesh.source.push_back({double(w) * x / nx, double(h) * y / ny});
    mesh.destination = mesh.source;
    const auto stride = nx + 1;
    for (std::size_t y = 0; y < ny; ++y)
        for (std::size_t x = 0; x < nx; ++x) {
            const auto a = y * stride + x;
            mesh.triangles.push_back({a, a + 1, a + stride + 1});
            mesh.triangles.push_back({a, a + stride + 1, a + stride});
        }
    return mesh;
}
struct Row {
    std::vector<std::pair<std::size_t, double>> values;
    double rhs;
};
// Direct CGLS representation preserves the reference's line/similarity least-squares terms.
struct LeastSquares {
    std::size_t n;
    std::vector<Row> rows;
    void add(std::vector<std::pair<std::size_t, double>> r, double rhs, double weight) {
        for (auto &[i, v] : r)
            v *= weight;
        rows.push_back({std::move(r), rhs * weight});
    }
    Values ax(const Values &x) const {
        Values v(rows.size());
        for (std::size_t k = 0; k < rows.size(); ++k)
            for (auto [i, c] : rows[k].values)
                v[k] += c * x[i];
        return v;
    }
    Values transpose(const Values &v) const {
        Values out(n);
        for (std::size_t k = 0; k < rows.size(); ++k)
            for (auto [i, c] : rows[k].values)
                out[i] += c * v[k];
        return out;
    }
    Values solve(Values x, unsigned iterations) const {
        auto r = ax(x);
        for (std::size_t k = 0; k < r.size(); ++k)
            r[k] = rows[k].rhs - r[k];
        auto s = transpose(r), p = s;
        auto gamma = std::inner_product(s.begin(), s.end(), s.begin(), 0.);
        const auto g0 = std::max(1e-30, gamma);
        for (unsigned iter = 0; iter < iterations && gamma > g0 * 1e-20; ++iter) {
            const auto q = ax(p);
            const auto qq = std::inner_product(q.begin(), q.end(), q.begin(), 0.);
            if (qq <= 1e-300)
                break;
            const auto alpha = gamma / qq;
            for (std::size_t i = 0; i < n; ++i)
                x[i] += alpha * p[i];
            for (std::size_t k = 0; k < r.size(); ++k)
                r[k] -= alpha * q[k];
            s = transpose(r);
            const auto next = std::inner_product(s.begin(), s.end(), s.begin(), 0.),
                       beta = next / gamma;
            gamma = next;
            for (std::size_t i = 0; i < n; ++i)
                p[i] = s[i] + beta * p[i];
        }
        return x;
    }
};
V3 add3(V3 a, V3 b) {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}
V3 sub3(V3 a, V3 b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
V3 mul3(V3 a, double k) {
    return {a[0] * k, a[1] * k, a[2] * k};
}
double dot3(V3 a, V3 b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
V3 norm3(V3 a) {
    return mul3(a, 1 / std::max(1e-300, std::sqrt(dot3(a, a))));
}
V3 cross3(V3 a, V3 b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
} // namespace
namespace {
bool global(LiquifyTool t) {
    return t >= LiquifyTool::ReconstructAll;
}
bool stationary(LiquifyTool t) {
    return t != LiquifyTool::ForwardWarp && t != LiquifyTool::PushLeft &&
           t != LiquifyTool::LassoMask;
}
bool validLiquify(const LiquifyOptions &o) {
    if (!(o.cell == 0 || detail::inRange(o.cell, 1, 64)) || o.strokes.size() > 64)
        return false;
    std::size_t points = 0, dabs = 0;
    for (const auto &s : o.strokes) {
        if (static_cast<unsigned>(s.tool) > 14 || !detail::inRange(s.size, 1, 15000) ||
            !detail::inRange(s.density, 0, 100) || !detail::inRange(s.pressure, 0, 100) ||
            !detail::inRange(s.rate, 0, 100))
            return false;
        if (s.amount && !detail::inRange(*s.amount, 0, s.tool == LiquifyTool::LassoMask ? 1 : 100))
            return false;
        points += s.points.size();
        if (points > 4096)
            return false;
        for (const auto &p : s.points)
            if (!point(p.position) || !detail::inRange(p.pressure, 0, 1))
                return false;
        if (global(s.tool) || s.tool == LiquifyTool::LassoMask)
            continue;
        if (stationary(s.tool) && !s.points.empty())
            ++dabs;
        for (std::size_t k = 1; k < s.points.size(); ++k) {
            const auto len = distance(s.points[k - 1].position, s.points[k].position);
            if (len == 0) {
                if (stationary(s.tool))
                    ++dabs;
            } else {
                const auto steps = std::max(1., std::ceil(len / std::max(.5, s.size * .1)));
                if (steps > 16384)
                    return false;
                dabs += std::size_t(steps);
            }
            if (dabs > 16384)
                return false;
        }
    }
    return true;
}
struct LiquifyField {
    double cell;
    std::size_t w, h;
    std::vector<P> displacement;
    Values freeze;
    LiquifyField(ImageView image, double spacing)
        : cell(spacing), w(std::size_t(std::ceil(image.width / cell)) + 2),
          h(std::size_t(std::ceil(image.height / cell)) + 2), displacement(w * h), freeze(w * h) {}
    P position(std::size_t i, std::size_t j) const {
        return {.5 + i * cell, .5 + j * cell};
    }
    P bilinear(const std::vector<P> &d, double x, double y) const {
        if (!(x > -1 && y > -1 && x < double(w) && y < double(h)))
            return {};
        x = std::clamp(x, 0., double(w - 1));
        y = std::clamp(y, 0., double(h - 1));
        const auto i = std::min(std::size_t(x), w - 2), j = std::min(std::size_t(y), h - 2);
        const auto fx = x - i, fy = y - j;
        P out{};
        for (int yy = 0; yy < 2; ++yy)
            for (int xx = 0; xx < 2; ++xx)
                out = add(out,
                          mul(d[(j + yy) * w + i + xx], (xx ? fx : 1 - fx) * (yy ? fy : 1 - fy)));
        return out;
    }
    P bicubic(double x, double y) const {
        const auto ix = static_cast<long long>(std::floor(x)),
                   iy = static_cast<long long>(std::floor(y));
        const auto wx = cubic(x - ix), wy = cubic(y - iy);
        P out{};
        for (int j = 0; j < 4; ++j)
            for (int i = 0; i < 4; ++i) {
                const auto xx = std::clamp(ix + i - 1, 0LL, static_cast<long long>(w - 1)),
                           yy = std::clamp(iy + j - 1, 0LL, static_cast<long long>(h - 1));
                out = add(out,
                          mul(displacement[std::size_t(yy) * w + std::size_t(xx)], wx[i] * wy[j]));
            }
        return out;
    }
    void lasso(const LiquifyStroke &s) {
        if (s.points.size() < 3)
            return;
        const auto value = s.amount.value_or(1) > .5 ? 1. : 0.;
        std::vector<double> xs;
        for (std::size_t j = 0; j < h; ++j) {
            const auto y = position(0, j)[1];
            xs.clear();
            auto previous = s.points.back().position;
            for (const auto &pt : s.points) {
                const auto a = pt.position;
                if ((a[1] > y) != (previous[1] > y))
                    xs.push_back(a[0] + (y - a[1]) * (previous[0] - a[0]) / (previous[1] - a[1]));
                previous = a;
            }
            std::sort(xs.begin(), xs.end());
            for (std::size_t k = 0; k + 1 < xs.size(); k += 2) {
                const auto lo = std::clamp(std::ceil((xs[k] - .5) / cell), 0., double(w)),
                           hi = std::clamp(std::ceil((xs[k + 1] - .5) / cell), 0., double(w));
                for (auto i = std::size_t(lo); i < std::size_t(hi); ++i)
                    freeze[j * w + i] = value;
            }
        }
    }
    void dab(const LiquifyStroke &s, P center, P delta, double pointPressure) {
        const auto r = s.size / 2, strength = s.pressure / 100 * pointPressure, rate = s.rate / 100,
                   density = s.density / 100;
        if (strength <= 0)
            return;
        const auto minx = std::clamp(std::floor((center[0] - r - .5) / cell), 0., double(w)),
                   miny = std::clamp(std::floor((center[1] - r - .5) / cell), 0., double(h)),
                   maxx = std::clamp(std::ceil((center[0] + r - .5) / cell) + 1, 0., double(w)),
                   maxy = std::clamp(std::ceil((center[1] + r - .5) / cell) + 1, 0., double(h));
        if (minx >= maxx || miny >= maxy)
            return;
        // Snapshot the affected nodes plus the largest composed-map sampling margin.
        const auto margin =
            std::size_t(
                std::ceil((std::max(std::abs(delta[0]), std::abs(delta[1])) + r * .2) / cell)) +
            2;
        const auto sx = std::size_t(minx) > margin ? std::size_t(minx) - margin : 0;
        const auto sy = std::size_t(miny) > margin ? std::size_t(miny) - margin : 0;
        const auto ex = std::min(w, std::size_t(maxx) + margin);
        const auto ey = std::min(h, std::size_t(maxy) + margin);
        const auto sw = ex - sx;
        std::vector<P> snapshot;
        snapshot.reserve(sw * (ey - sy));
        for (auto y = sy; y < ey; ++y)
            snapshot.insert(snapshot.end(), displacement.begin() + y * w + sx,
                            displacement.begin() + y * w + ex);
        const auto oldAt = [&](std::size_t x, std::size_t y) {
            return x >= sx && x < ex && y >= sy && y < ey ? snapshot[(y - sy) * sw + x - sx]
                                                          : displacement[y * w + x];
        };
        const auto oldSample = [&](double x, double y) {
            if (!(x > -1 && y > -1 && x < double(w) && y < double(h)))
                return P{};
            x = std::clamp(x, 0., double(w - 1));
            y = std::clamp(y, 0., double(h - 1));
            const auto i = std::min(std::size_t(x), w - 2), j = std::min(std::size_t(y), h - 2);
            const auto fx = x - i, fy = y - j;
            P out{};
            for (int yy = 0; yy < 2; ++yy)
                for (int xx = 0; xx < 2; ++xx)
                    out = add(out,
                              mul(oldAt(i + xx, j + yy), (xx ? fx : 1 - fx) * (yy ? fy : 1 - fy)));
            return out;
        };
        for (auto j = std::size_t(miny); j < std::size_t(maxy); ++j)
            for (auto i = std::size_t(minx); i < std::size_t(maxx); ++i) {
                const auto p = position(i, j), d = sub(p, center);
                const auto t2 = dot(d, d) / (r * r);
                if (t2 >= 1)
                    continue;
                const auto kRaw = strength * ((1 - density) + density * (1 - t2) * (1 - t2));
                const auto index = j * w + i;
                if (s.tool == LiquifyTool::Freeze) {
                    freeze[index] = std::min(1., freeze[index] + kRaw);
                    continue;
                }
                if (s.tool == LiquifyTool::Thaw) {
                    freeze[index] = std::max(0., freeze[index] - kRaw);
                    continue;
                }
                const auto k = kRaw * (1 - freeze[index]);
                if (k <= 0)
                    continue;
                const auto current = oldAt(i, j);
                if (s.tool == LiquifyTool::Reconstruct) {
                    displacement[index] = mul(current, 1 - k * (.1 + .4 * rate));
                    continue;
                }
                if (s.tool == LiquifyTool::Smooth) {
                    P mean{};
                    for (auto [dx, dy] : std::array<std::pair<int, int>, 8>{{{-1, 0},
                                                                             {1, 0},
                                                                             {0, -1},
                                                                             {0, 1},
                                                                             {-1, -1},
                                                                             {1, -1},
                                                                             {-1, 1},
                                                                             {1, 1}}}) {
                        const auto xx = std::clamp(static_cast<long long>(i) + dx, 0LL,
                                                   static_cast<long long>(w - 1)),
                                   yy = std::clamp(static_cast<long long>(j) + dy, 0LL,
                                                   static_cast<long long>(h - 1));
                        mean = add(mean, mul(oldAt(std::size_t(xx), std::size_t(yy)), .125));
                    }
                    displacement[index] =
                        add(current, mul(sub(mean, current), k * (.1 + .4 * rate)));
                    continue;
                }
                P q;
                switch (s.tool) {
                case LiquifyTool::ForwardWarp:
                    q = sub(p, mul(delta, k));
                    break;
                case LiquifyTool::PushLeft:
                    q = {p[0] - k * delta[1], p[1] + k * delta[0]};
                    break;
                case LiquifyTool::TwirlClockwise:
                case LiquifyTool::TwirlCounterclockwise: {
                    const auto theta = -k * rate * .12 *
                                       (s.tool == LiquifyTool::TwirlClockwise ? 1 : -1),
                               sn = std::sin(theta), cs = std::cos(theta);
                    q = add(center, {d[0] * cs - d[1] * sn, d[0] * sn + d[1] * cs});
                    break;
                }
                case LiquifyTool::Pucker:
                    q = add(center, mul(d, 1 + k * rate * .06));
                    break;
                default:
                    q = add(center, mul(d, 1 - k * rate * .06));
                    break;
                }
                const auto dq = oldSample((q[0] - .5) / cell, (q[1] - .5) / cell);
                displacement[index] = sub(add(q, dq), p);
            }
    }
    void stroke(const LiquifyStroke &s) {
        switch (s.tool) {
        case LiquifyTool::FreezeAll:
            std::fill(freeze.begin(), freeze.end(), 1);
            return;
        case LiquifyTool::ThawAll:
            std::fill(freeze.begin(), freeze.end(), 0);
            return;
        case LiquifyTool::InvertFreeze:
            for (auto &v : freeze)
                v = 1 - v;
            return;
        case LiquifyTool::ReconstructAll:
            for (std::size_t k = 0; k < displacement.size(); ++k)
                displacement[k] =
                    mul(displacement[k], 1 - s.amount.value_or(100) / 100 * (1 - freeze[k]));
            return;
        case LiquifyTool::LassoMask:
            lasso(s);
            return;
        default:
            break;
        }
        if (s.points.empty())
            return;
        if (stationary(s.tool))
            dab(s, s.points.front().position, {}, s.points.front().pressure);
        for (std::size_t j = 1; j < s.points.size(); ++j) {
            const auto &a = s.points[j - 1];
            const auto &b = s.points[j];
            const auto len = distance(a.position, b.position);
            if (len == 0) {
                if (stationary(s.tool))
                    dab(s, b.position, {}, b.pressure);
                continue;
            }
            const auto steps =
                std::size_t(std::max(1., std::ceil(len / std::max(.5, s.size * .1))));
            const auto delta = mul(sub(b.position, a.position), 1. / steps);
            for (std::size_t k = 1; k <= steps; ++k) {
                const auto t = double(k) / steps;
                const auto c = add(a.position, mul(sub(b.position, a.position), t));
                dab(s, stationary(s.tool) ? c : sub(c, delta), delta,
                    a.pressure + (b.pressure - a.pressure) * t);
            }
        }
    }
};
} // namespace
Result<Image> liquify(ImageView image, const LiquifyOptions &o) {
    if (auto r = imageValid(image); !r)
        return std::unexpected(r.error());
    if (!validLiquify(o))
        return std::unexpected(fd::invalid());
    const auto cell = o.cell == 0 ? (image.pixels.size() <= 4200000 ? 2. : 4.) : o.cell;
    const auto w = std::size_t(std::ceil(image.width / cell)) + 2,
               h = std::size_t(std::ceil(image.height / cell)) + 2;
    if (w > 4000000 / h)
        return std::unexpected(
            Error{ErrorCode::InvalidParameter, "Liquify field exceeds four million nodes"});
    return detail::allocated([&] {
        LiquifyField field(image, cell);
        for (const auto &s : o.strokes)
            field.stroke(s);
        auto out = detail::copy(image);
        for (std::size_t y = 0; y < image.height; ++y)
            for (std::size_t x = 0; x < image.width; ++x) {
                const auto d = field.bicubic(x / cell, y / cell);
                if (d != P{})
                    out.pixels[y * image.width + x] =
                        sample(image, {x + .5 + d[0], y + .5 + d[1]}, MeshInterpolation::Bicubic,
                               fd::Edge::Repeat);
            }
        return out;
    });
}
namespace {
bool validWide(const AdaptiveWideAngleOptions &o) {
    if (static_cast<unsigned>(o.model) > 3 || !detail::inRange(o.focalLength, 0, 1000) ||
        !detail::inRange(o.cropFactor, .1, 10) || !detail::inRange(o.scale, 10, 1000) ||
        o.constraints.size() > 32 || o.meshCells < 2 || o.meshCells > 96 ||
        o.solverIterations < 1 || o.solverIterations > 2000 || !interpolation(o.interpolation))
        return false;
    for (const auto &c : o.constraints)
        if (!point(c.a) || !point(c.b) || distance(c.a, c.b) < 1e-8 ||
            static_cast<unsigned>(c.orientation) > 2)
            return false;
    return true;
}
struct Camera {
    WideAngleModel model;
    double focal;
    P center, size;
    Camera(ImageView image, const AdaptiveWideAngleOptions &o)
        : model(o.model), center{image.width / 2., image.height / 2.},
          size{double(image.width), double(image.height)} {
        auto f35 = o.focalLength * o.cropFactor;
        if (model == WideAngleModel::Auto)
            model =
                std::abs(size[0] / size[1] - 2) < .01
                    ? WideAngleModel::FullSpherical
                    : (f35 > 0 && f35 < 17 ? WideAngleModel::Fisheye : WideAngleModel::Perspective);
        if (o.focalLength == 0)
            f35 = model == WideAngleModel::Fisheye ? 12 : 24;
        focal = f35 / 36 * std::max(size[0], size[1]);
    }
    V3 ray(P p) const {
        const auto d = sub(p, center);
        switch (model) {
        case WideAngleModel::FullSpherical: {
            const auto longitude = d[0] / size[0] * 2 * fd::pi, latitude = d[1] / size[1] * fd::pi;
            return norm3({std::sin(longitude) * std::cos(latitude), std::sin(latitude),
                          std::cos(longitude) * std::cos(latitude)});
        }
        case WideAngleModel::Fisheye: {
            const auto r = std::hypot(d[0], d[1]), theta = r / focal;
            return r < 1e-12 ? V3{0, 0, 1}
                             : norm3({std::sin(theta) * d[0] / r, std::sin(theta) * d[1] / r,
                                      std::cos(theta)});
        }
        default:
            return norm3({d[0], d[1], focal});
        }
    }
    std::optional<P> pixel(V3 d) const {
        switch (model) {
        case WideAngleModel::FullSpherical:
            return P{center[0] + std::atan2(d[0], d[2]) / (2 * fd::pi) * size[0],
                     center[1] + std::asin(std::clamp(d[1], -1., 1.)) / fd::pi * size[1]};
        case WideAngleModel::Fisheye: {
            const auto theta = std::acos(std::clamp(d[2], -1., 1.)), s = std::hypot(d[0], d[1]);
            return s < 1e-12 ? center
                             : P{center[0] + focal * theta * d[0] / s,
                                 center[1] + focal * theta * d[1] / s};
        }
        default:
            if (d[2] <= 1e-6)
                return {};
            return P{center[0] + focal * d[0] / d[2], center[1] + focal * d[1] / d[2]};
        }
    }
    std::optional<std::vector<P>> arc(P a, P b) const {
        if (model != WideAngleModel::FullSpherical && (!std::isfinite(focal) || focal <= 0))
            return {};
        const auto da = ray(a), db = ray(b);
        if (!std::all_of(da.begin(), da.end(), [](double v) { return std::isfinite(v); }) ||
            !std::all_of(db.begin(), db.end(), [](double v) { return std::isfinite(v); }))
            return {};
        const auto cosine = std::clamp(dot3(da, db), -1., 1.);
        if (cosine < -1 + 1e-8)
            return {};
        const auto omega = std::acos(cosine);
        std::vector<P> out;
        for (int k = 0; k <= 24; ++k) {
            const auto t = k / 24.;
            const auto d = omega < 1e-9
                               ? da
                               : norm3(add3(mul3(da, std::sin((1 - t) * omega) / std::sin(omega)),
                                            mul3(db, std::sin(t * omega) / std::sin(omega))));
            if (!std::all_of(d.begin(), d.end(), [](double v) { return std::isfinite(v); }))
                return {};
            if (const auto p = pixel(d)) {
                if (!std::isfinite((*p)[0]) || !std::isfinite((*p)[1]))
                    return {};
                out.push_back(*p);
            }
        }
        return out;
    }
};
using Weights = std::array<std::pair<std::size_t, double>, 4>;
Weights gridWeights(ImageView image, std::size_t nx, std::size_t ny, P p) {
    const auto gx = std::clamp(p[0] / image.width * nx, 0., double(nx) - 1e-9),
               gy = std::clamp(p[1] / image.height * ny, 0., double(ny) - 1e-9);
    const auto i = std::size_t(std::floor(gx)), j = std::size_t(std::floor(gy)),
               a = j * (nx + 1) + i;
    const auto tx = gx - i, ty = gy - j;
    return {{{a, (1 - tx) * (1 - ty)},
             {a + 1, tx * (1 - ty)},
             {a + nx + 1, (1 - tx) * ty},
             {a + nx + 2, tx * ty}}};
}
P weightedPoint(const std::vector<P> &vertices, const Weights &weights) {
    P out{};
    for (auto [i, w] : weights)
        out = add(out, mul(vertices[i], w));
    return out;
}
Result<MeshGeometry> wideSolve(ImageView image, const AdaptiveWideAngleOptions &o) {
    const Camera camera(image, o);
    const auto cell = double(std::max(image.width, image.height)) / o.meshCells;
    const auto nx = std::size_t(std::clamp(std::lround(image.width / cell), 2L, 96L)),
               ny = std::size_t(std::clamp(std::lround(image.height / cell), 2L, 96L));
    auto mesh = regular(image.width, image.height, nx, ny);
    const auto nv = mesh.source.size();
    for (const auto &c : o.constraints) {
        const auto arc = camera.arc(c.a, c.b);
        if (!arc)
            return std::unexpected(
                Error{ErrorCode::InvalidParameter,
                      "Wide-angle constraint has no finite unique great-circle projection"});
        mesh.constraintCurves.push_back(*arc);
    }
    std::vector<std::size_t> active;
    for (std::size_t k = 0; k < mesh.constraintCurves.size(); ++k)
        if (mesh.constraintCurves[k].size() >= 3)
            active.push_back(k);
    Values x(2 * nv);
    for (std::size_t k = 0; k < nv; ++k) {
        x[2 * k] = mesh.source[k][0];
        x[2 * k + 1] = mesh.source[k][1];
    }
    const auto weights = [&](P p) { return gridWeights(image, nx, ny, p); };
    if (!active.empty()) {
        std::vector<P> normals(active.size(), P{0, 1});
        auto shape = mesh.triangles;
        for (std::size_t j = 0; j < ny; ++j)
            for (std::size_t i = 0; i < nx; ++i) {
                const auto a = j * (nx + 1) + i;
                shape.push_back({a, a + 1, a + nx + 1});
                shape.push_back({a + 1, a + nx + 2, a + nx + 1});
            }
        for (unsigned round = 0; round < 4; ++round) {
            LeastSquares ls{2 * nv, {}};
            for (auto t : shape)
                for (int rotation = 0; rotation < 3; ++rotation) {
                    const auto i0 = t[rotation], i1 = t[(rotation + 1) % 3],
                               i2 = t[(rotation + 2) % 3];
                    const auto e = sub(mesh.source[i2], mesh.source[i1]),
                               d = sub(mesh.source[i0], mesh.source[i1]);
                    const auto len = dot(e, e), u = dot(d, e) / len,
                               v = dot(d, P{-e[1], e[0]}) / len;
                    ls.add({{2 * i0, 1},
                            {2 * i1, -1 + u},
                            {2 * i2, -u},
                            {2 * i2 + 1, v},
                            {2 * i1 + 1, -v}},
                           0, 1);
                    ls.add({{2 * i0 + 1, 1},
                            {2 * i1 + 1, -1 + u},
                            {2 * i2 + 1, -u},
                            {2 * i2, -v},
                            {2 * i1, v}},
                           0, 1);
                }
            for (std::size_t k = 0; k < nv; ++k) {
                ls.add({{2 * k, 1}}, mesh.source[k][0], .02);
                ls.add({{2 * k + 1, 1}}, mesh.source[k][1], .02);
            }
            for (std::size_t ci = 0; ci < active.size(); ++ci) {
                const auto k = active[ci];
                const auto &curve = mesh.constraintCurves[k];
                const auto orientation = o.constraints[k].orientation;
                const auto normal = orientation == WideAngleOrientation::Horizontal ? P{0, 1}
                                    : orientation == WideAngleOrientation::Vertical ? P{1, 0}
                                                                                    : normals[ci];
                const auto first = weights(curve.front());
                for (std::size_t j = 1; j < curve.size(); ++j) {
                    std::vector<std::pair<std::size_t, double>> row;
                    for (auto [i, w] : weights(curve[j])) {
                        row.push_back({2 * i, normal[0] * w});
                        row.push_back({2 * i + 1, normal[1] * w});
                    }
                    for (auto [i, w] : first) {
                        row.push_back({2 * i, -normal[0] * w});
                        row.push_back({2 * i + 1, -normal[1] * w});
                    }
                    ls.add(std::move(row), 0, 10);
                }
            }
            x = ls.solve(std::move(x), std::min(o.solverIterations, 400 + 200 * round));
            for (std::size_t k = 0; k < nv; ++k)
                mesh.destination[k] = {x[2 * k], x[2 * k + 1]};
            bool changed = false;
            for (std::size_t ci = 0; ci < active.size(); ++ci) {
                const auto &curve = mesh.constraintCurves[active[ci]];
                const auto a = weightedPoint(mesh.destination, weights(curve.front())),
                           b = weightedPoint(mesh.destination, weights(curve.back())),
                           d = sub(b, a);
                const auto length = std::max(1e-9, std::hypot(d[0], d[1]));
                const P normal{-d[1] / length, d[0] / length};
                if (std::abs(normal[0] - normals[ci][0]) + std::abs(normal[1] - normals[ci][1]) >
                    1e-3)
                    changed = true;
                normals[ci] = normal;
            }
            if ((round == 0 && std::all_of(active.begin(), active.end(),
                                           [&](std::size_t k) {
                                               return o.constraints[k].orientation !=
                                                      WideAngleOrientation::Free;
                                           })) ||
                (round > 0 && !changed))
                break;
        }
    }
    for (auto &p : mesh.destination) {
        p = add(camera.center, mul(sub(p, camera.center), o.scale / 100));
        if (!std::isfinite(p[0]) || !std::isfinite(p[1]))
            return std::unexpected(Error{ErrorCode::InvalidParameter,
                                         "Wide-angle solver did not produce finite vertices"});
    }
    for (const auto &curve : mesh.constraintCurves) {
        if (curve.size() < 3)
            continue;
        const auto a = weightedPoint(mesh.destination, weights(curve.front())),
                   b = weightedPoint(mesh.destination, weights(curve.back())), d = sub(b, a);
        const auto length = std::max(1e-9, std::hypot(d[0], d[1]));
        for (auto q : curve)
            mesh.constraintResidual = std::max(
                mesh.constraintResidual,
                std::abs(cross(sub(weightedPoint(mesh.destination, weights(q)), a), d)) / length);
    }
    return mesh;
}
} // namespace
Result<MeshGeometry> solveAdaptiveWideAngle(ImageView image, const AdaptiveWideAngleOptions &o) {
    if (auto r = imageValid(image); !r)
        return std::unexpected(r.error());
    if (!validWide(o))
        return std::unexpected(fd::invalid());
    return guardedResult([&] { return wideSolve(image, o); });
}
Result<Image> adaptiveWideAngle(ImageView image, const AdaptiveWideAngleOptions &o) {
    if (auto r = imageValid(image); !r)
        return std::unexpected(r.error());
    if (!validWide(o))
        return std::unexpected(fd::invalid());
    return guardedResult([&]() -> Result<Image> {
        if (o.constraints.empty() && o.scale == 100)
            return detail::copy(image);
        auto mesh = wideSolve(image, o);
        if (!mesh)
            return std::unexpected(mesh.error());
        return renderMesh(image, *mesh, o.interpolation);
    });
}
namespace {
bool validPuppet(const PuppetWarpOptions &o) {
    if (o.pins.size() > 64 || static_cast<unsigned>(o.mode) > 2 ||
        static_cast<unsigned>(o.density) > 2 || !detail::inRange(o.expansion, -200, 200) ||
        o.iterations < 1 || o.iterations > 100 || o.solverIterations < 1 ||
        o.solverIterations > 2000 || !interpolation(o.interpolation))
        return false;
    for (const auto &p : o.pins)
        if (!point(p.source) || !point(p.destination) ||
            (p.rotation && !detail::inRange(*p.rotation, -360, 360)) || p.depth < -10000 ||
            p.depth > 10000)
            return false;
    return true;
}
// Exact squared Euclidean distance transform replaces the reference's radius-squared stamp.
// The resulting round growth/erosion membership is the same at integer block coordinates.
void distanceLine(Values &f) {
    const auto n = f.size();
    std::vector<std::size_t> v(n);
    Values z(n + 1), out(n);
    std::size_t k = 0;
    v[0] = 0;
    z[0] = -1e30;
    z[1] = 1e30;
    for (std::size_t q = 1; q < n; ++q) {
        double s;
        do {
            s = ((f[q] + double(q) * q) - (f[v[k]] + double(v[k]) * v[k])) / (2. * (q - v[k]));
            if (s <= z[k] && k > 0)
                --k;
            else
                break;
        } while (true);
        ++k;
        v[k] = q;
        z[k] = s;
        z[k + 1] = 1e30;
    }
    k = 0;
    for (std::size_t q = 0; q < n; ++q) {
        while (z[k + 1] < double(q))
            ++k;
        const auto d = double(q) - v[k];
        out[q] = d * d + f[v[k]];
    }
    f.swap(out);
}
Values distances(const std::vector<bool> &coverage, std::size_t w, std::size_t h, bool target) {
    Values p(coverage.size());
    for (std::size_t k = 0; k < p.size(); ++k)
        p[k] = coverage[k] == target ? 0 : 1e20;
    Values line;
    for (int axis = 0; axis < 2; ++axis) {
        const auto length = axis == 0 ? w : h, lines = axis == 0 ? h : w;
        line.resize(length);
        for (std::size_t a = 0; a < lines; ++a) {
            const auto at = [&](std::size_t x) { return axis == 0 ? a * w + x : x * w + a; };
            for (std::size_t x = 0; x < length; ++x)
                line[x] = p[at(x)];
            distanceLine(line);
            for (std::size_t x = 0; x < length; ++x)
                p[at(x)] = line[x];
        }
    }
    return p;
}
Result<MeshGeometry> puppetMesh(ImageView image, const PuppetWarpOptions &o) {
    const auto cells = o.density == PuppetDensity::Fewer  ? 12.
                       : o.density == PuppetDensity::More ? 32.
                                                          : 20.,
               spacing = std::max(2., double(std::max(image.width, image.height)) / cells);
    const auto block = std::max(1., std::floor(spacing / 4)),
               pad = std::ceil(std::max(0., o.expansion)) + block;
    const auto bw = std::size_t(std::ceil((image.width + 2 * pad) / block)),
               bh = std::size_t(std::ceil((image.height + 2 * pad) / block));
    if (bw > 16000000 / bh)
        return std::unexpected(Error{ErrorCode::InvalidParameter,
                                     "Puppet coverage grid exceeds sixteen million blocks"});
    std::vector<bool> coverage(bw * bh);
    for (std::size_t y = 0; y < image.height; ++y)
        for (std::size_t x = 0; x < image.width; ++x)
            if (image.pixels[y * image.width + x].a > .05f)
                coverage[std::size_t((y + pad) / block) * bw + std::size_t((x + pad) / block)] =
                    true;
    const auto radius = std::lround(std::abs(o.expansion) / block);
    if (radius > 0) {
        const auto grow = o.expansion > 0;
        const auto distance = distances(coverage, bw, bh, grow);
        const auto r2 = double(radius) * radius;
        for (std::size_t y = 0; y < bh; ++y)
            for (std::size_t x = 0; x < bw; ++x) {
                if (grow)
                    coverage[y * bw + x] = distance[y * bw + x] <= r2;
                else {
                    const auto edge = double(std::min({x + 1, y + 1, bw - x, bh - y}));
                    coverage[y * bw + x] = distance[y * bw + x] > r2 && edge * edge > r2;
                }
            }
    }
    const auto cw = std::size_t(std::ceil(bw * block / spacing)),
               ch = std::size_t(std::ceil(bh * block / spacing));
    if ((cw + 1) > maximumNodes / (ch + 1))
        return std::unexpected(
            Error{ErrorCode::InvalidParameter, "Puppet mesh exceeds 16384 grid nodes"});
    std::vector<std::size_t> index((cw + 1) * (ch + 1), std::numeric_limits<std::size_t>::max());
    MeshGeometry mesh;
    const auto vertex = [&](std::size_t x, std::size_t y) {
        const auto k = y * (cw + 1) + x;
        if (index[k] == std::numeric_limits<std::size_t>::max()) {
            index[k] = mesh.source.size();
            mesh.source.push_back({-pad + x * spacing, -pad + y * spacing});
        }
        return index[k];
    };
    for (std::size_t y = 0; y < ch; ++y)
        for (std::size_t x = 0; x < cw; ++x) {
            const auto x0 = std::min(bw, std::size_t(std::floor(x * spacing / block))),
                       y0 = std::min(bh, std::size_t(std::floor(y * spacing / block))),
                       x1 = std::min(bw, std::size_t(std::ceil((x + 1) * spacing / block))),
                       y1 = std::min(bh, std::size_t(std::ceil((y + 1) * spacing / block)));
            bool any = false;
            for (auto yy = y0; yy < y1 && !any; ++yy)
                for (auto xx = x0; xx < x1; ++xx)
                    if (coverage[yy * bw + xx]) {
                        any = true;
                        break;
                    }
            if (!any)
                continue;
            const auto a = vertex(x, y), b = vertex(x + 1, y), c = vertex(x + 1, y + 1),
                       d = vertex(x, y + 1);
            if ((x + y) % 2 == 0) {
                mesh.triangles.push_back({a, b, c});
                mesh.triangles.push_back({a, c, d});
            } else {
                mesh.triangles.push_back({a, b, d});
                mesh.triangles.push_back({b, c, d});
            }
        }
    mesh.destination = mesh.source;
    return mesh;
}
struct Bound {
    Tri verts{};
    std::array<double, 3> bary{};
};
Bound bindPin(const MeshGeometry &mesh, P p) {
    for (auto t : mesh.triangles) {
        const auto a = mesh.source[t[0]], b = mesh.source[t[1]], c = mesh.source[t[2]];
        const auto det = cross(sub(b, a), sub(c, a));
        if (std::abs(det) < 1e-12)
            continue;
        const auto l1 = cross(sub(p, a), sub(c, a)) / det, l2 = cross(sub(b, a), sub(p, a)) / det,
                   l0 = 1 - l1 - l2;
        if (l0 >= -1e-9 && l1 >= -1e-9 && l2 >= -1e-9)
            return {t, {l0, l1, l2}};
    }
    std::size_t best = 0;
    double dist = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < mesh.source.size(); ++i) {
        const auto d = distance(mesh.source[i], p);
        if (d < dist) {
            dist = d;
            best = i;
        }
    }
    return {{best, best, best}, {1, 0, 0}};
}
double cot(P a, P b, P c) {
    const auto u = sub(b, a), v = sub(c, a);
    const auto area = std::abs(cross(u, v));
    return area < 1e-12 ? 0 : dot(u, v) / area;
}
struct Triplet {
    std::size_t i, j;
    double value;
};
struct Csr {
    std::vector<std::size_t> rows, cols;
    Values vals, diag;
    Csr(std::size_t n, std::vector<Triplet> t) : rows(n + 1), diag(n, 1) {
        std::sort(t.begin(), t.end(),
                  [](auto a, auto b) { return std::tie(a.i, a.j) < std::tie(b.i, b.j); });
        std::size_t k = 0;
        for (std::size_t i = 0; i < n; ++i) {
            rows[i] = cols.size();
            while (k < t.size() && t[k].i == i) {
                const auto j = t[k].j;
                double sum = 0;
                while (k < t.size() && t[k].i == i && t[k].j == j)
                    sum += t[k++].value;
                cols.push_back(j);
                vals.push_back(sum);
                if (j == i && sum > 0)
                    diag[i] = sum;
            }
        }
        rows[n] = cols.size();
    }
    Values multiply(const Values &x) const {
        Values out(diag.size());
        for (std::size_t i = 0; i < out.size(); ++i)
            for (auto k = rows[i]; k < rows[i + 1]; ++k)
                out[i] += vals[k] * x[cols[k]];
        return out;
    }
    Values solve(const Values &b, Values x, unsigned iterations) const {
        auto r = multiply(x);
        for (std::size_t i = 0; i < x.size(); ++i)
            r[i] = b[i] - r[i];
        Values z(x.size());
        for (std::size_t i = 0; i < x.size(); ++i)
            z[i] = r[i] / diag[i];
        auto p = z;
        auto rz = std::inner_product(r.begin(), r.end(), z.begin(), 0.);
        const auto norm =
            std::max(1e-30, std::sqrt(std::inner_product(b.begin(), b.end(), b.begin(), 0.)));
        for (unsigned iter = 0; iter < iterations; ++iter) {
            if (std::sqrt(std::inner_product(r.begin(), r.end(), r.begin(), 0.)) <= 1e-12 * norm)
                break;
            const auto ap = multiply(p);
            const auto pap = std::inner_product(p.begin(), p.end(), ap.begin(), 0.);
            if (std::abs(pap) < 1e-300)
                break;
            const auto alpha = rz / pap;
            for (std::size_t i = 0; i < x.size(); ++i) {
                x[i] += alpha * p[i];
                r[i] -= alpha * ap[i];
                z[i] = r[i] / diag[i];
            }
            const auto next = std::inner_product(r.begin(), r.end(), z.begin(), 0.);
            if (std::abs(rz) < 1e-300)
                break;
            const auto beta = next / rz;
            rz = next;
            for (std::size_t i = 0; i < x.size(); ++i)
                p[i] = z[i] + beta * p[i];
        }
        return x;
    }
};
struct Band {
    std::size_t n, p;
    Values l;
    static std::optional<Band> create(const Csr &a) {
        const auto n = a.diag.size();
        std::size_t p = 0;
        for (std::size_t i = 0; i < n; ++i)
            for (auto k = a.rows[i]; k < a.rows[i + 1]; ++k)
                p = std::max(p, i > a.cols[k] ? i - a.cols[k] : a.cols[k] - i);
        if (n * (p + 1) > 4000000)
            return {};
        Band out{n, p, Values(n * (p + 1))};
        const auto w = p + 1;
        for (std::size_t i = 0; i < n; ++i)
            for (auto k = a.rows[i]; k < a.rows[i + 1]; ++k)
                if (a.cols[k] <= i)
                    out.l[i * w + a.cols[k] + p - i] = a.vals[k];
        for (std::size_t i = 0; i < n; ++i) {
            const auto lo = i > p ? i - p : 0;
            for (auto j = lo; j <= i; ++j) {
                const auto start = std::max(lo, j > p ? j - p : 0);
                auto sum = out.l[i * w + j + p - i];
                for (auto k = start; k < j; ++k)
                    sum -= out.l[i * w + k + p - i] * out.l[j * w + k + p - j];
                if (i == j) {
                    if (sum <= 0 || !std::isfinite(sum))
                        return {};
                    out.l[i * w + p] = std::sqrt(sum);
                } else
                    out.l[i * w + j + p - i] = sum / out.l[j * w + p];
            }
        }
        return out;
    }
    Values solve(const Values &b) const {
        const auto w = p + 1;
        auto y = b;
        Values x(n);
        for (std::size_t i = 0; i < n; ++i) {
            auto sum = y[i];
            for (auto k = i > p ? i - p : 0; k < i; ++k)
                sum -= l[i * w + k + p - i] * y[k];
            y[i] = sum / l[i * w + p];
        }
        for (auto i = n; i-- > 0;) {
            auto sum = y[i];
            for (auto k = i + 1; k < std::min(n, i + p + 1); ++k)
                sum -= l[k * w + i + p - k] * x[k];
            x[i] = sum / l[i * w + p];
        }
        return x;
    }
};
Result<MeshGeometry> puppetSolve(ImageView image, const PuppetWarpOptions &o) {
    auto built = puppetMesh(image, o);
    if (!built)
        return std::unexpected(built.error());
    auto mesh = std::move(*built);
    const auto nv = mesh.source.size();
    if (nv == 0 || o.pins.empty())
        return mesh;
    std::vector<std::array<double, 3>> weights;
    for (auto t : mesh.triangles) {
        const auto a = mesh.source[t[0]], b = mesh.source[t[1]], c = mesh.source[t[2]];
        weights.push_back({.5 * cot(a, b, c), .5 * cot(b, c, a), .5 * cot(c, a, b)});
    }
    std::vector<Bound> bound;
    for (const auto &pin : o.pins)
        bound.push_back(bindPin(mesh, pin.source));
    std::vector<std::size_t> parent(nv);
    std::iota(parent.begin(), parent.end(), 0);
    const auto find = [&](std::size_t v) {
        while (parent[v] != v) {
            parent[v] = parent[parent[v]];
            v = parent[v];
        }
        return v;
    };
    for (auto t : mesh.triangles)
        for (int k = 1; k < 3; ++k) {
            const auto a = find(t[0]), b = find(t[k]);
            if (a != b)
                parent[a] = b;
        }
    std::vector<bool> pinned(nv), anchored(nv);
    for (const auto &b : bound)
        for (auto v : b.verts)
            pinned[find(v)] = true;
    for (std::size_t k = 0; k < nv; ++k)
        anchored[k] = !pinned[find(k)];
    std::vector<std::vector<std::size_t>> fixedTriangles(o.pins.size());
    for (std::size_t k = 0; k < o.pins.size(); ++k)
        if (o.pins[k].rotation) {
            auto best = bound[k].verts[0];
            for (auto v : bound[k].verts)
                if (distance(mesh.source[v], o.pins[k].source) <
                    distance(mesh.source[best], o.pins[k].source))
                    best = v;
            for (std::size_t t = 0; t < mesh.triangles.size(); ++t)
                if (std::find(mesh.triangles[t].begin(), mesh.triangles[t].end(), best) !=
                    mesh.triangles[t].end())
                    fixedTriangles[k].push_back(t);
        }
    std::vector<Triplet> trip;
    double diagonalSum = 0;
    for (std::size_t t = 0; t < mesh.triangles.size(); ++t)
        for (int k = 0; k < 3; ++k) {
            const auto i = mesh.triangles[t][(k + 1) % 3], j = mesh.triangles[t][(k + 2) % 3];
            const auto w = weights[t][k];
            trip.insert(trip.end(), {{i, i, w}, {j, j, w}, {i, j, -w}, {j, i, -w}});
            diagonalSum += 2 * w;
        }
    const auto pinWeight = 1e4 * std::max(1., diagonalSum / nv);
    for (const auto &b : bound)
        for (int a = 0; a < 3; ++a)
            for (int c = 0; c < 3; ++c)
                trip.push_back({b.verts[a], b.verts[c], pinWeight * b.bary[a] * b.bary[c]});
    for (std::size_t i = 0; i < nv; ++i)
        if (anchored[i])
            trip.push_back({i, i, pinWeight});
    const Csr matrix(nv, std::move(trip));
    const auto band = Band::create(matrix);
    using Rotation = std::array<double, 4>;
    const auto globalStep = [&](const std::vector<Rotation> &rotations) {
        Values bx(nv), by(nv), sx(nv), sy(nv);
        for (std::size_t t = 0; t < mesh.triangles.size(); ++t)
            for (int k = 0; k < 3; ++k) {
                const auto i = mesh.triangles[t][(k + 1) % 3], j = mesh.triangles[t][(k + 2) % 3];
                const auto e = sub(mesh.source[i], mesh.source[j]);
                const auto &r = rotations[t];
                const P re{r[0] * e[0] + r[1] * e[1], r[2] * e[0] + r[3] * e[1]};
                bx[i] += weights[t][k] * re[0];
                by[i] += weights[t][k] * re[1];
                bx[j] -= weights[t][k] * re[0];
                by[j] -= weights[t][k] * re[1];
            }
        for (std::size_t k = 0; k < bound.size(); ++k)
            for (int a = 0; a < 3; ++a) {
                const auto i = bound[k].verts[a];
                bx[i] += pinWeight * bound[k].bary[a] * o.pins[k].destination[0];
                by[i] += pinWeight * bound[k].bary[a] * o.pins[k].destination[1];
            }
        for (std::size_t i = 0; i < nv; ++i) {
            if (anchored[i]) {
                bx[i] += pinWeight * mesh.source[i][0];
                by[i] += pinWeight * mesh.source[i][1];
            }
            sx[i] = mesh.destination[i][0];
            sy[i] = mesh.destination[i][1];
        }
        sx = band ? band->solve(bx) : matrix.solve(bx, std::move(sx), o.solverIterations);
        sy = band ? band->solve(by) : matrix.solve(by, std::move(sy), o.solverIterations);
        for (std::size_t i = 0; i < nv; ++i)
            mesh.destination[i] = {sx[i], sy[i]};
    };
    std::vector<Rotation> rotations(mesh.triangles.size(), Rotation{1, 0, 0, 1});
    for (std::size_t t = 0; t < mesh.triangles.size(); ++t) {
        P center{};
        for (auto i : mesh.triangles[t])
            center = add(center, mul(mesh.source[i], 1. / 3));
        double total = 0, angle = 0;
        for (const auto &pin : o.pins)
            if (pin.rotation) {
                const auto delta = sub(pin.source, center);
                const auto w = 1 / (dot(delta, delta) + 1);
                total += w;
                angle += w * *pin.rotation * fd::pi / 180;
            }
        if (total > 0) {
            const auto a = angle / total;
            rotations[t] = {std::cos(a), -std::sin(a), std::sin(a), std::cos(a)};
        }
    }
    globalStep(rotations);
    for (unsigned iter = 0; iter < o.iterations; ++iter) {
        for (std::size_t t = 0; t < mesh.triangles.size(); ++t) {
            Rotation covariance{};
            double norm = 0;
            for (int k = 0; k < 3; ++k) {
                const auto i = mesh.triangles[t][(k + 1) % 3], j = mesh.triangles[t][(k + 2) % 3];
                const auto d = sub(mesh.destination[i], mesh.destination[j]),
                           e = sub(mesh.source[i], mesh.source[j]);
                const auto w = weights[t][k];
                covariance[0] += w * d[0] * e[0];
                covariance[1] += w * d[0] * e[1];
                covariance[2] += w * d[1] * e[0];
                covariance[3] += w * d[1] * e[1];
                norm += w * dot(e, e);
            }
            const auto cosine = covariance[0] + covariance[3], sine = covariance[2] - covariance[1],
                       theta = std::atan2(sine, cosine);
            auto scale =
                o.mode == PuppetMode::Rigid ? 1. : std::hypot(cosine, sine) / std::max(1e-12, norm);
            if (o.mode == PuppetMode::Normal)
                scale = std::sqrt(scale);
            if (!std::isfinite(scale) || scale <= 0)
                scale = 1;
            rotations[t] = {scale * std::cos(theta), -scale * std::sin(theta),
                            scale * std::sin(theta), scale * std::cos(theta)};
        }
        for (std::size_t k = 0; k < o.pins.size(); ++k)
            if (o.pins[k].rotation) {
                const auto angle = *o.pins[k].rotation * fd::pi / 180;
                for (auto t : fixedTriangles[k])
                    rotations[t] = {std::cos(angle), -std::sin(angle), std::sin(angle),
                                    std::cos(angle)};
            }
        globalStep(rotations);
    }
    for (auto p : mesh.destination)
        if (!std::isfinite(p[0]) || !std::isfinite(p[1]))
            return std::unexpected(Error{ErrorCode::InvalidParameter,
                                         "Puppet solver did not produce finite vertices"});
    if (std::any_of(o.pins.begin(), o.pins.end(), [](const auto &p) { return p.depth != 0; })) {
        const auto depth = [&](Tri t) {
            P center{};
            for (auto i : t)
                center = add(center, mul(mesh.source[i], 1. / 3));
            std::size_t best = 0;
            for (std::size_t k = 1; k < o.pins.size(); ++k)
                if (distance(o.pins[k].source, center) < distance(o.pins[best].source, center))
                    best = k;
            return o.pins[best].depth;
        };
        std::stable_sort(mesh.triangles.begin(), mesh.triangles.end(),
                         [&](Tri a, Tri b) { return depth(a) < depth(b); });
    }
    return mesh;
}
} // namespace
Result<MeshGeometry> solvePuppetWarp(ImageView image, const PuppetWarpOptions &o) {
    if (auto r = imageValid(image); !r)
        return std::unexpected(r.error());
    if (!validPuppet(o))
        return std::unexpected(fd::invalid());
    return guardedResult([&] { return puppetSolve(image, o); });
}
Result<Image> puppetWarp(ImageView image, const PuppetWarpOptions &o) {
    if (auto r = imageValid(image); !r)
        return std::unexpected(r.error());
    if (!validPuppet(o))
        return std::unexpected(fd::invalid());
    return guardedResult([&]() -> Result<Image> {
        if (std::all_of(o.pins.begin(), o.pins.end(), [](const auto &p) {
                return p.source == p.destination &&
                       (!p.rotation || std::fmod(*p.rotation, 360) == 0);
            }))
            return detail::copy(image);
        auto mesh = puppetSolve(image, o);
        if (!mesh)
            return std::unexpected(mesh.error());
        return renderMesh(image, *mesh, o.interpolation);
    });
}
namespace {
bool validPerspective(const PerspectiveWarpOptions &o) {
    if (o.planes.size() > 16 || static_cast<unsigned>(o.straighten) > 3 ||
        !detail::inRange(o.cell, 1, 64) || !interpolation(o.interpolation))
        return false;
    for (const auto &p : o.planes)
        if (!quadValid(p.source) || !quadValid(p.destination))
            return false;
    return true;
}
using CornerGroup = std::vector<std::pair<std::size_t, std::size_t>>;
std::vector<CornerGroup> linked(const std::vector<PerspectivePlane> &planes) {
    std::vector<CornerGroup> groups;
    for (std::size_t pi = 0; pi < planes.size(); ++pi)
        for (std::size_t ci = 0; ci < 4; ++ci) {
            const auto point = planes[pi].source[ci];
            auto found = std::find_if(groups.begin(), groups.end(), [&](const auto &g) {
                const auto [p, c] = g.front();
                const auto other = planes[p].source[c];
                return std::abs(point[0] - other[0]) <= .5 && std::abs(point[1] - other[1]) <= .5;
            });
            if (found == groups.end())
                groups.push_back({{pi, ci}});
            else
                found->push_back({pi, ci});
        }
    return groups;
}
void unify(std::vector<PerspectivePlane> &planes) {
    for (const auto &group : linked(planes)) {
        P source{}, destination{};
        for (auto [p, c] : group) {
            source = add(source, mul(planes[p].source[c], 1. / group.size()));
            destination = add(destination, mul(planes[p].destination[c], 1. / group.size()));
        }
        for (auto [p, c] : group) {
            planes[p].source[c] = source;
            planes[p].destination[c] = destination;
        }
    }
}
void straighten(std::vector<PerspectivePlane> &planes, PerspectiveStraighten mode) {
    if (mode == PerspectiveStraighten::None)
        return;
    unify(planes);
    const auto groups = linked(planes);
    std::vector<P> positions;
    for (const auto &g : groups)
        positions.push_back(planes[g.front().first].destination[g.front().second]);
    const auto groupOf = [&](std::size_t p, std::size_t c) {
        for (std::size_t i = 0; i < groups.size(); ++i)
            if (std::find(groups[i].begin(), groups[i].end(), std::pair{p, c}) != groups[i].end())
                return i;
        return std::size_t{0};
    };
    for (std::size_t p = 0; p < planes.size(); ++p)
        for (std::size_t k = 0; k < 4; ++k) {
            const auto a = groupOf(p, k), b = groupOf(p, (k + 1) % 4);
            const auto dx = std::abs(positions[a][0] - positions[b][0]),
                       dy = std::abs(positions[a][1] - positions[b][1]);
            if (dy <= dx && (mode == PerspectiveStraighten::Horizontal ||
                             mode == PerspectiveStraighten::Auto)) {
                const auto y = (positions[a][1] + positions[b][1]) / 2;
                positions[a][1] = positions[b][1] = y;
            } else if (dx < dy && (mode == PerspectiveStraighten::Vertical ||
                                   mode == PerspectiveStraighten::Auto)) {
                const auto x = (positions[a][0] + positions[b][0]) / 2;
                positions[a][0] = positions[b][0] = x;
            }
        }
    for (std::size_t i = 0; i < groups.size(); ++i)
        for (auto [p, c] : groups[i])
            planes[p].destination[c] = positions[i];
}
double quadDistance(const MeshQuad &q, P p) {
    double best = std::numeric_limits<double>::infinity();
    for (int k = 0; k < 4; ++k) {
        const auto a = q[k], b = q[(k + 1) % 4], d = sub(b, a);
        const auto t = dot(d, d) > 0 ? std::clamp(dot(sub(p, a), d) / dot(d, d), 0., 1.) : 0;
        best = std::min(best, distance(add(a, mul(d, t)), p));
    }
    return best;
}
struct PlaneMap {
    H source, inverse, destination;
    MeshQuad quad;
    std::array<std::optional<std::size_t>, 4> shared;
};
struct PerspectiveMap {
    std::vector<PlaneMap> maps;
    static std::optional<PerspectiveMap> create(std::vector<PerspectivePlane> planes,
                                                PerspectiveStraighten mode) {
        unify(planes);
        straighten(planes, mode);
        const auto near = [](P a, P b) {
            return std::abs(a[0] - b[0]) <= .5 && std::abs(a[1] - b[1]) <= .5;
        };
        PerspectiveMap out;
        for (std::size_t i = 0; i < planes.size(); ++i) {
            const auto s = unitQuad(planes[i].source), d = unitQuad(planes[i].destination);
            if (!s || !d)
                return {};
            const auto inv = s->inverse();
            if (!inv)
                return {};
            PlaneMap map{*s, *inv, *d, planes[i].source, {}};
            for (std::size_t k = 0; k < 4; ++k) {
                const auto a = planes[i].source[k], b = planes[i].source[(k + 1) % 4];
                for (std::size_t j = 0; j < planes.size() && !map.shared[k]; ++j)
                    if (j != i)
                        for (std::size_t m = 0; m < 4; ++m) {
                            const auto c = planes[j].source[m], d = planes[j].source[(m + 1) % 4];
                            if ((near(a, c) && near(b, d)) || (near(a, d) && near(b, c))) {
                                map.shared[k] = j;
                                break;
                            }
                        }
            }
            out.maps.push_back(map);
        }
        return out;
    }
    P map(P p) const {
        if (maps.empty())
            return p;
        std::size_t best = 0;
        double distance = std::numeric_limits<double>::infinity();
        P coordinates{};
        for (std::size_t i = 0; i < maps.size(); ++i) {
            const auto uv = maps[i].inverse.apply(p);
            if (uv[0] >= -1e-9 && uv[0] <= 1 + 1e-9 && uv[1] >= -1e-9 && uv[1] <= 1 + 1e-9) {
                best = i;
                coordinates = uv;
                break;
            }
            const auto d = quadDistance(maps[i].quad, p);
            if (d < distance) {
                distance = d;
                best = i;
                coordinates = uv;
            }
        }
        const auto &pm = maps[best];
        auto out = pm.destination.apply(coordinates);
        if (!std::isfinite(out[0]) || !std::isfinite(out[1]))
            return p;
        const auto u = std::clamp(coordinates[0], 0., 1.), v = std::clamp(coordinates[1], 0., 1.);
        for (std::size_t k = 0; k < 4; ++k)
            if (pm.shared[k]) {
                P edge;
                double weight;
                switch (k) {
                case 0:
                    edge = {u, 0};
                    weight = 1 - v;
                    break;
                case 1:
                    edge = {1, v};
                    weight = u;
                    break;
                case 2:
                    edge = {u, 1};
                    weight = v;
                    break;
                default:
                    edge = {0, v};
                    weight = 1 - u;
                    break;
                }
                if (weight <= 0)
                    continue;
                const auto mine = pm.destination.apply(edge), source = pm.source.apply(edge);
                const auto &other = maps[*pm.shared[k]];
                const auto theirs = other.destination.apply(other.inverse.apply(source));
                out = add(out, mul(sub(mul(add(mine, theirs), .5), mine), weight));
            }
        return out;
    }
};
} // namespace
Result<MeshPoint> mapPerspectiveWarpPoint(MeshPoint p, const PerspectiveWarpOptions &o) {
    if (!point(p) || !validPerspective(o))
        return std::unexpected(fd::invalid());
    return guardedResult([&]() -> Result<P> {
        const auto map = PerspectiveMap::create(o.planes, o.straighten);
        if (!map)
            return std::unexpected(fd::invalid());
        const auto result = map->map(p);
        if (!std::isfinite(result[0]) || !std::isfinite(result[1]))
            return std::unexpected(fd::invalid());
        return result;
    });
}
Result<Image> perspectiveWarp(ImageView image, const PerspectiveWarpOptions &o) {
    if (auto r = imageValid(image); !r)
        return std::unexpected(r.error());
    if (!validPerspective(o))
        return std::unexpected(fd::invalid());
    return guardedResult([&]() -> Result<Image> {
        const auto map = PerspectiveMap::create(o.planes, o.straighten);
        if (!map)
            return std::unexpected(fd::invalid());
        if (o.straighten == PerspectiveStraighten::None &&
            std::all_of(o.planes.begin(), o.planes.end(),
                        [](const auto &p) { return p.source == p.destination; }))
            return detail::copy(image);
        const auto nx = std::size_t(std::clamp(std::ceil(image.width / o.cell), 1., 512.)),
                   ny = std::size_t(std::clamp(std::ceil(image.height / o.cell), 1., 512.));
        auto mesh = regular(image.width, image.height, nx, ny);
        for (std::size_t i = 0; i < mesh.source.size(); ++i) {
            mesh.destination[i] = map->map(mesh.source[i]);
            if (!std::isfinite(mesh.destination[i][0]) || !std::isfinite(mesh.destination[i][1]))
                return std::unexpected(fd::invalid());
        }
        return renderMesh(image, mesh, o.interpolation);
    });
}
namespace {
V3 meet(P a, P b, P c, P d) {
    return cross3(cross3({a[0], a[1], 1}, {b[0], b[1], 1}),
                  cross3({c[0], c[1], 1}, {d[0], d[1], 1}));
}
std::optional<double> estimatedFocal(const MeshQuad &q, P center) {
    const auto v1 = meet(q[0], q[1], q[3], q[2]), v2 = meet(q[0], q[3], q[1], q[2]);
    if (std::abs(v1[2]) < 1e-9 || std::abs(v2[2]) < 1e-9)
        return {};
    const P a{v1[0] / v1[2] - center[0], v1[1] / v1[2] - center[1]},
        b{v2[0] / v2[2] - center[0], v2[1] / v2[2] - center[1]};
    const auto f2 = -dot(a, b);
    return f2 > 1 ? std::optional<double>(std::sqrt(f2)) : std::nullopt;
}
struct Lifted {
    double focal;
    P center;
    std::array<V3, 4> points;
    std::optional<P> project(V3 p) const {
        if (p[2] <= 1e-9)
            return {};
        return P{center[0] + focal * p[0] / p[2], center[1] + focal * p[1] / p[2]};
    }
    P lengths() const {
        const auto length = [](V3 a, V3 b) {
            const auto d = sub3(a, b);
            return std::sqrt(dot3(d, d));
        };
        return {(length(points[0], points[1]) + length(points[3], points[2])) / 2,
                (length(points[0], points[3]) + length(points[1], points[2])) / 2};
    }
};
Lifted lift(const MeshQuad &q, double focal, P center) {
    const auto ray = [&](P p) {
        return V3{(p[0] - center[0]) / focal, (p[1] - center[1]) / focal, 1};
    };
    const auto direction = [&](P a, P b, P c, P d) {
        const auto v = meet(a, b, c, d);
        return std::abs(v[2]) < 1e-9 * std::max(1., std::abs(v[0]) + std::abs(v[1]))
                   ? norm3({v[0], v[1], 0})
                   : norm3(ray({v[0] / v[2], v[1] / v[2]}));
    };
    auto normal =
        norm3(cross3(direction(q[0], q[1], q[3], q[2]), direction(q[0], q[3], q[1], q[2])));
    if (normal[2] < 0)
        normal = mul3(normal, -1);
    Lifted out{focal, center, {}};
    for (int k = 0; k < 4; ++k) {
        const auto r = ray(q[k]);
        out.points[k] = mul3(r, 1 / std::max(1e-9, std::abs(dot3(normal, r))));
    }
    return out;
}
bool validVanishing(const VanishingPointOptions &o) {
    if (o.planes.size() > 16 || o.tearOffs.size() > 16 ||
        o.planes.size() + o.tearOffs.size() > 32 || o.pastes.size() > 32 || o.clones.size() > 64 ||
        !(o.focalLength == 0 || detail::inRange(o.focalLength, 1, 100000)))
        return false;
    if (o.planes.empty() && (!o.tearOffs.empty() || !o.pastes.empty() || !o.clones.empty()))
        return false;
    for (const auto &p : o.planes)
        if (!quadValid(p.corners))
            return false;
    auto count = o.planes.size();
    for (const auto &t : o.tearOffs) {
        if (t.from >= count || static_cast<unsigned>(t.edge) > 3 ||
            !detail::inRange(t.angle, 1, 179) || !detail::inRange(t.depth, .05, 10))
            return false;
        ++count;
    }
    for (const auto &p : o.pastes)
        if (p.plane >= count || !detail::inRange(p.at[0], -2, 3) ||
            !detail::inRange(p.at[1], -2, 3) || !detail::inRange(p.width, .001, 10) ||
            !detail::inRange(p.opacity, 0, 1))
            return false;
    std::size_t points = 0;
    for (const auto &c : o.clones) {
        points += c.points.size();
        if (points > 4096 || !point(c.source) || !detail::inRange(c.size, 1, 4096) ||
            !detail::inRange(c.hardness, 0, 1) || !detail::inRange(c.opacity, 0, 1))
            return false;
        for (auto p : c.points)
            if (!point(p))
                return false;
    }
    return true;
}
struct Scene {
    std::vector<VanishingPlane> planes;
    double focal;
    P center;
    std::optional<std::tuple<H, H, P>> frame(std::size_t i) const {
        const auto h = unitQuad(planes[i].corners);
        if (!h)
            return {};
        const auto inverse = h->inverse();
        if (!inverse)
            return {};
        const auto lengths = lift(planes[i].corners, focal, center).lengths();
        if (!std::isfinite(lengths[0]) || !std::isfinite(lengths[1]) || lengths[0] < 1e-12 ||
            lengths[1] < 1e-12)
            return {};
        return std::tuple{*h, *inverse, lengths};
    }
    std::optional<std::size_t> planeAt(P p) const {
        for (auto i = planes.size(); i-- > 0;) {
            const auto f = frame(i);
            if (!f)
                continue;
            const auto uv = std::get<1>(*f).apply(p);
            if (uv[0] >= 0 && uv[0] <= 1 && uv[1] >= 0 && uv[1] <= 1)
                return i;
        }
        return {};
    }
    bool tear(const VanishingTearOff &t) {
        const auto lf = lift(planes[t.from].corners, focal, center);
        int ia = 0, ib = 1, ic = 3;
        switch (t.edge) {
        case VanishingEdge::Right:
            ia = 1;
            ib = 2;
            ic = 0;
            break;
        case VanishingEdge::Bottom:
            ia = 2;
            ib = 3;
            ic = 1;
            break;
        case VanishingEdge::Left:
            ia = 3;
            ib = 0;
            ic = 2;
            break;
        default:
            break;
        }
        const auto a = lf.points[ia], b = lf.points[ib], axis = norm3(sub3(b, a)),
                   inward = sub3(lf.points[ic], a), out = mul3(norm3(inward), -1);
        const auto len = std::sqrt(dot3(inward, inward)) * t.depth,
                   theta = (180 - t.angle) * fd::pi / 180;
        std::optional<MeshQuad> best;
        double score = -std::numeric_limits<double>::infinity();
        for (auto sign : {1., -1.}) {
            const auto s = std::sin(sign * theta), c = std::cos(sign * theta);
            const auto rotation = add3(add3(mul3(out, c), mul3(cross3(axis, out), s)),
                                       mul3(axis, dot3(axis, out) * (1 - c))),
                       a2 = add3(a, mul3(rotation, len)), b2 = add3(b, mul3(rotation, len));
            const auto pa = lf.project(a), pb = lf.project(b), pa2 = lf.project(a2),
                       pb2 = lf.project(b2);
            if (!pa || !pb || !pa2 || !pb2)
                continue;
            auto normal = norm3(cross3(sub3(b, a), sub3(a2, a)));
            if (dot3(normal, a) > 0)
                normal = mul3(normal, -1);
            const auto value = dot3(normal, inward);
            MeshQuad corners;
            switch (t.edge) {
            case VanishingEdge::Top:
                corners = {*pa2, *pb2, *pb, *pa};
                break;
            case VanishingEdge::Right:
                corners = {*pa, *pa2, *pb2, *pb};
                break;
            case VanishingEdge::Bottom:
                corners = {*pb, *pa, *pa2, *pb2};
                break;
            default:
                corners = {*pb2, *pb, *pa, *pa2};
                break;
            }
            if (value > score && quadValid(corners)) {
                score = value;
                best = corners;
            }
        }
        if (!best)
            return false;
        planes.push_back({*best});
        return true;
    }
};
Result<Scene> scene(ImageView image, const VanishingPointOptions &o) {
    Scene out{o.planes, o.focalLength, {image.width / 2., image.height / 2.}};
    if (out.focal <= 0)
        out.focal = out.planes.empty() ? double(std::max(image.width, image.height))
                                       : estimatedFocal(out.planes[0].corners, out.center)
                                             .value_or(double(std::max(image.width, image.height)));
    for (const auto &tear : o.tearOffs)
        if (!out.tear(tear))
            return std::unexpected(
                Error{ErrorCode::InvalidParameter,
                      "Vanishing tear-off does not project to a valid convex plane"});
    for (std::size_t i = 0; i < out.planes.size(); ++i)
        if (!out.frame(i))
            return std::unexpected(fd::invalid());
    return out;
}
Rgba over(Rgba destination, Rgba source, double opacity) {
    auto src = fd::premul(source);
    const auto dst = fd::premul(destination);
    for (auto &v : src)
        v *= opacity;
    fd::Pixel out{};
    for (int c = 0; c < 4; ++c)
        out[c] = src[c] + dst[c] * (1 - src[3]);
    return fd::straight(out);
}
void pasteOnPlane(Image &output, const Scene &scene, const VanishingPaste &paste) {
    if (paste.opacity == 0)
        return;
    const auto [h, inverse, lengths] = *scene.frame(paste.plane);
    const auto su = paste.width / paste.image.width,
               sv = paste.width * lengths[0] / lengths[1] / paste.image.width;
    for (std::size_t y = 0; y < output.height; ++y)
        for (std::size_t x = 0; x < output.width; ++x) {
            const auto uv = inverse.apply({x + .5, y + .5});
            if (uv[0] < 0 || uv[0] > 1 || uv[1] < 0 || uv[1] > 1)
                continue;
            const P p{(uv[0] - paste.at[0]) / su, (uv[1] - paste.at[1]) / sv};
            if (p[0] < 0 || p[1] < 0 || p[0] >= paste.image.width || p[1] >= paste.image.height)
                continue;
            const auto color =
                sample(paste.image, p, MeshInterpolation::Bicubic, fd::Edge::Transparent);
            if (color.a > 0)
                output.pixels[y * output.width + x] =
                    over(output.pixels[y * output.width + x], color, paste.opacity);
        }
}
void cloneOnPlane(Image &output, const Scene &scene, const VanishingCloneStroke &stroke) {
    if (stroke.points.empty() || stroke.opacity == 0)
        return;
    const auto sourcePlane = scene.planeAt(stroke.source),
               destinationPlane = scene.planeAt(stroke.points[0]);
    if (!sourcePlane || !destinationPlane)
        return;
    const auto [hs, is, ls] = *scene.frame(*sourcePlane);
    const auto [hd, id, ld] = *scene.frame(*destinationPlane);
    const auto metric = [](const H &inv, P size, P p) {
        const auto uv = inv.apply(p);
        return P{uv[0] * size[0], uv[1] * size[1]};
    };
    const auto ms = metric(is, ls, stroke.source), first = metric(id, ld, stroke.points[0]),
               offset = sub(ms, first),
               probe = metric(id, ld, {stroke.points[0][0] + stroke.size / 2, stroke.points[0][1]});
    const auto radius = std::max(1e-9, distance(probe, first)),
               hard = std::min(.99, stroke.hardness);
    const auto original = output;
    for (auto center : stroke.points) {
        const auto mc = metric(id, ld, center);
        double x0 = std::numeric_limits<double>::infinity(), y0 = x0, x1 = -x0, y1 = -x0;
        for (int k = 0; k < 16; ++k) {
            const auto angle = k / 16. * 2 * fd::pi;
            const auto p = hd.apply({(mc[0] + radius * std::cos(angle)) / ld[0],
                                     (mc[1] + radius * std::sin(angle)) / ld[1]});
            if (!std::isfinite(p[0]) || !std::isfinite(p[1]))
                continue;
            x0 = std::min(x0, p[0]);
            y0 = std::min(y0, p[1]);
            x1 = std::max(x1, p[0]);
            y1 = std::max(y1, p[1]);
        }
        if (!std::isfinite(x0) || x1 - x0 > 4096 || y1 - y0 > 4096)
            continue;
        const auto minx = std::size_t(std::clamp(std::floor(x0), 0., double(output.width))),
                   miny = std::size_t(std::clamp(std::floor(y0), 0., double(output.height))),
                   maxx = std::size_t(std::clamp(std::ceil(x1) + 1, 0., double(output.width))),
                   maxy = std::size_t(std::clamp(std::ceil(y1) + 1, 0., double(output.height)));
        for (auto y = miny; y < maxy; ++y)
            for (auto x = minx; x < maxx; ++x) {
                const auto m = metric(id, ld, {x + .5, y + .5});
                const auto d = distance(m, mc) / radius;
                if (!std::isfinite(d) || d >= 1)
                    continue;
                const auto fall = d <= hard ? 1 : 1 - (d - hard) / (1 - hard),
                           weight = fall * fall * (3 - 2 * fall) * stroke.opacity;
                const auto sm = add(m, offset), p = hs.apply({sm[0] / ls[0], sm[1] / ls[1]});
                const auto color =
                    sample(original.view(), p, MeshInterpolation::Bilinear, fd::Edge::Transparent);
                auto dst = fd::premul(output.pixels[y * output.width + x]);
                const auto src = fd::premul(color);
                for (int c = 0; c < 4; ++c)
                    dst[c] += (src[c] - dst[c]) * weight;
                output.pixels[y * output.width + x] = fd::straight(dst);
            }
    }
}
Result<void> vanishingInput(ImageView image, const VanishingPointOptions &o) {
    if (auto r = imageValid(image); !r)
        return r;
    if (!validVanishing(o))
        return std::unexpected(fd::invalid());
    for (const auto &p : o.pastes)
        if (auto r = imageValid(p.image); !r)
            return r;
    return {};
}
} // namespace
Result<std::vector<VanishingPlane>> vanishingPointPlanes(ImageView image,
                                                         const VanishingPointOptions &o) {
    if (auto r = vanishingInput(image, o); !r)
        return std::unexpected(r.error());
    return guardedResult([&]() -> Result<std::vector<VanishingPlane>> {
        auto prepared = scene(image, o);
        if (!prepared)
            return std::unexpected(prepared.error());
        return std::move(prepared->planes);
    });
}
Result<Image> vanishingPoint(ImageView image, const VanishingPointOptions &o) {
    if (auto r = vanishingInput(image, o); !r)
        return std::unexpected(r.error());
    return guardedResult([&]() -> Result<Image> {
        auto prepared = scene(image, o);
        if (!prepared)
            return std::unexpected(prepared.error());
        auto output = detail::copy(image);
        for (const auto &p : o.pastes)
            pasteOnPlane(output, *prepared, p);
        for (const auto &c : o.clones)
            cloneOnPlane(output, *prepared, c);
        return output;
    });
}
} // namespace iiGraphicAdjust
