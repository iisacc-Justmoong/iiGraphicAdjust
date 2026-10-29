// Algorithms adapted from LightCraft; see third_party/lightcraft/LICENSE-MIT.
#include "DevelopInternal.h"
#include <iiGraphicAdjust/Optics.h>

namespace iiGraphicAdjust {
namespace dd = develop_detail;
namespace {
struct Sample {
    double value = 0, alpha = 0;
};
Sample sample(const dd::Frame &f, double x, double y, int channel, bool clampEdges = false) {
    if (!clampEdges &&
        (x < -.5 || y < -.5 || x > double(f.width) - .5 || y > double(f.height) - .5))
        return {};
    x = std::clamp(x, 0., double(f.width - 1));
    y = std::clamp(y, 0., double(f.height - 1));
    const auto x0 = static_cast<std::size_t>(x), y0 = static_cast<std::size_t>(y),
               x1 = std::min(x0 + 1, f.width - 1), y1 = std::min(y0 + 1, f.height - 1);
    const auto tx = x - x0, ty = y - y0;
    const std::array<std::size_t, 4> at{y0 * f.width + x0, y0 * f.width + x1, y1 * f.width + x0,
                                        y1 * f.width + x1};
    const std::array<double, 4> weights{(1 - tx) * (1 - ty), tx * (1 - ty), (1 - tx) * ty, tx * ty};
    Sample out;
    for (int i = 0; i < 4; ++i) {
        const auto mass = weights[i] * f.alpha[at[i]];
        out.alpha += mass;
        out.value += mass * f.rgb[at[i]][channel];
    }
    if (out.alpha > 0)
        out.value /= out.alpha;
    return out;
}
bool valid(const OpticsOptions &o) {
    for (auto v : {o.distortion, o.vignetting, o.redChromaticAberration, o.blueChromaticAberration})
        if (!detail::inRange(v, -100, 100))
            return false;
    for (auto v :
         {o.vignettingMidpoint, o.purpleHueLow, o.purpleHueHigh, o.greenHueLow, o.greenHueHigh})
        if (!detail::inRange(v, 0, 100))
            return false;
    if (o.purpleHueLow > o.purpleHueHigh || o.greenHueLow > o.greenHueHigh ||
        !detail::inRange(o.purpleDefringe, 0, 20) || !detail::inRange(o.greenDefringe, 0, 20) ||
        !detail::inRange(o.profileDistortion, 0, 200) ||
        !detail::inRange(o.profileVignetting, 0, 200))
        return false;
    const auto position = [](auto p) {
        return detail::inRange(p.centerX, 0, 1) && detail::inRange(p.centerY, 0, 1) &&
               detail::inRange(p.radius, 1e-6, 2);
    };
    if (o.embeddedWarp) {
        if (!position(*o.embeddedWarp))
            return false;
        for (auto row : o.embeddedWarp->planes)
            for (auto v : row)
                if (!detail::inRange(v, -16, 16))
                    return false;
    }
    if (o.embeddedVignette) {
        if (!position(*o.embeddedVignette))
            return false;
        for (auto v : o.embeddedVignette->coefficients)
            if (!detail::inRange(v, -16, 16))
                return false;
    }
    return true;
}
std::array<double, 2> estimate(const dd::Frame &f) {
    if (f.width < 32 || f.height < 32)
        return {};
    struct Edge {
        double weight = 0;
        std::size_t x = 0, y = 0;
    };
    std::array<std::vector<Edge>, 24 * 24> cells;
    const auto cx = double(f.width - 1) / 2, cy = double(f.height - 1) / 2,
               hd = std::hypot(f.width, f.height) / 2;
    for (std::size_t y = 6; y < f.height - 6; y += 2)
        for (std::size_t x = 6; x < f.width - 6; x += 2) {
            if (f.alpha[y * f.width + x] == 0 || f.alpha[y * f.width + x + 1] == 0 ||
                f.alpha[y * f.width + x - 1] == 0 || f.alpha[(y + 1) * f.width + x] == 0 ||
                f.alpha[(y - 1) * f.width + x] == 0)
                continue;
            const auto gx = f.rgb[y * f.width + x + 1][1] - f.rgb[y * f.width + x - 1][1],
                       gy = f.rgb[(y + 1) * f.width + x][1] - f.rgb[(y - 1) * f.width + x][1],
                       magnitude = std::hypot(gx, gy);
            const auto mean = (f.rgb[y * f.width + x + 1][1] + f.rgb[y * f.width + x - 1][1] +
                               f.rgb[(y + 1) * f.width + x][1] + f.rgb[(y - 1) * f.width + x][1]) *
                                  .25 +
                              .02;
            const auto relative = magnitude / mean, vx = x - cx, vy = y - cy,
                       r = std::hypot(vx, vy);
            if (relative < .2 || r < .2 * hd ||
                std::abs((gx * vx + gy * vy) / (magnitude * r)) < .85)
                continue;
            auto &cell = cells[(y * 24 / f.height) * 24 + x * 24 / f.width];
            const Edge edge{relative, x, y};
            if (cell.size() < 12)
                cell.push_back(edge);
            else {
                auto it = std::min_element(cell.begin(), cell.end(),
                                           [](auto a, auto b) { return a.weight < b.weight; });
                if (it->weight < relative)
                    *it = edge;
            }
        }
    const auto normalize = [](std::array<double, 9> &p) {
        double mean = 0, variance = 0;
        for (auto v : p)
            mean += v / 9;
        for (auto v : p)
            variance += (v - mean) * (v - mean) / 9;
        const auto sd = std::sqrt(variance);
        if (sd < 1e-5)
            return false;
        for (auto &v : p)
            v = (v - mean) / sd;
        return true;
    };
    struct Observation {
        double r, delta, weight;
    };
    std::array<std::vector<Observation>, 2> observations;
    for (const auto &cell : cells)
        for (auto e : cell) {
            const auto vx = e.x - cx, vy = e.y - cy, r = std::hypot(vx, vy), ux = vx / r,
                       uy = vy / r;
            std::array<double, 9> green{};
            bool visible = true;
            for (int t = -4; t <= 4; ++t) {
                const auto p = sample(f, e.x + ux * t, e.y + uy * t, 1);
                green[t + 4] = p.value;
                visible = visible && p.alpha > 0;
            }
            if (!visible || !normalize(green))
                continue;
            for (int k = 0; k < 2; ++k) {
                double best = std::numeric_limits<double>::max();
                int shift = 0;
                for (int d = -60; d <= 60; ++d) {
                    std::array<double, 9> p{};
                    bool coverage = true;
                    for (int t = -4; t <= 4; ++t) {
                        const auto v = sample(f, e.x + ux * (t + d / 20.), e.y + uy * (t + d / 20.),
                                              k == 0 ? 0 : 2);
                        p[t + 4] = v.value;
                        coverage = coverage && v.alpha > 0;
                    }
                    if (!coverage || !normalize(p))
                        continue;
                    double cost = 0;
                    for (int i = 0; i < 9; ++i)
                        cost += (p[i] - green[i]) * (p[i] - green[i]);
                    if (cost < best - 1e-12 ||
                        (std::abs(cost - best) <= 1e-12 && std::abs(d) < std::abs(shift))) {
                        best = cost;
                        shift = d;
                    }
                }
                if (best < 2.7 && std::abs(shift) < 60)
                    observations[k].push_back({r, shift / 20., e.weight});
            }
        }
    std::array<double, 2> out{};
    for (int k = 0; k < 2; ++k) {
        const auto &obs = observations[k];
        if (obs.size() < 12)
            continue;
        double estimate = 0, cutoff = std::numeric_limits<double>::infinity();
        for (int iteration = 0; iteration < 3; ++iteration) {
            double numerator = 0, denominator = 0;
            std::size_t retained = 0;
            for (auto o : obs)
                if (std::abs(o.delta - estimate * o.r) <= cutoff) {
                    numerator += o.weight * o.delta * o.r;
                    denominator += o.weight * o.r * o.r;
                    ++retained;
                }
            if (retained < 8 || denominator == 0)
                break;
            estimate = numerator / denominator;
            std::vector<double> residual;
            for (auto o : obs)
                residual.push_back(std::abs(o.delta - estimate * o.r));
            const auto at = residual.size() / 2;
            std::nth_element(residual.begin(), residual.begin() + at, residual.end());
            cutoff = 2.5 * std::max(.05, residual[at]);
        }
        out[k] = std::clamp(estimate / .003 * 100, -100., 100.);
    }
    return out;
}
} // namespace
Result<std::array<double, 2>> estimateChromaticAberration(ImageView image) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    return detail::allocated([&] { return estimate(dd::decode(image)); });
}
Result<Image> correctOptics(ImageView image, const OpticsOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (!valid(o))
        return std::unexpected(dd::invalid("Invalid manual or embedded lens settings"));
    return detail::allocated([&] {
        if (o.distortion == 0 && o.vignetting == 0 && o.redChromaticAberration == 0 &&
            o.blueChromaticAberration == 0 && !o.autoChromaticAberration && o.purpleDefringe == 0 &&
            o.greenDefringe == 0 && !o.embeddedWarp && !o.embeddedVignette)
            return detail::copy(image);
        auto f = dd::decode(image);
        auto src = f;
        auto ca = o.autoChromaticAberration ? estimate(f) : std::array<double, 2>{};
        ca[0] += o.redChromaticAberration;
        ca[1] += o.blueChromaticAberration;
        const bool moves = o.distortion != 0 || ca[0] != 0 || ca[1] != 0 ||
                           (o.embeddedWarp && o.profileDistortion != 0);
        const auto cx = double(f.width - 1) / 2, cy = double(f.height - 1) / 2,
                   hd = std::hypot(f.width, f.height) / 2,
                   longEdge = double(std::max(f.width, f.height));
        for (std::size_t y = 0; y < f.height; ++y)
            for (std::size_t x = 0; x < f.width; ++x) {
                const auto i = y * f.width + x;
                dd::Rgb rgb{};
                double coverage = f.alpha[i], commonGain = 1;
                for (int k = 0; k < 3; ++k) {
                    double px = x, py = y;
                    const auto dx = px - cx, dy = py - cy, r2 = (dx * dx + dy * dy) / (hd * hd),
                               factor = 1 - .2 * o.distortion / 100 * r2;
                    px = cx + dx * factor;
                    py = cy + dy * factor;
                    if (o.embeddedWarp && o.profileDistortion != 0) {
                        const auto &w = *o.embeddedWarp;
                        const auto centerX = w.centerX * f.width - .5,
                                   centerY = w.centerY * f.height - .5,
                                   radius = w.radius * longEdge, u = (px - centerX) / radius,
                                   v = (py - centerY) / radius, rr = u * u + v * v;
                        const auto coeff = w.planes[k];
                        const auto gain =
                                       coeff[0] + rr * (coeff[1] + rr * (coeff[2] + rr * coeff[3])),
                                   sx = centerX + radius * (u * gain + 2 * coeff[4] * u * v +
                                                            coeff[5] * (rr + 2 * u * u)),
                                   sy = centerY + radius * (v * gain + coeff[4] * (rr + 2 * v * v) +
                                                            2 * coeff[5] * u * v);
                        px += (sx - px) * o.profileDistortion / 100;
                        py += (sy - py) * o.profileDistortion / 100;
                    }
                    const auto aberration = k == 0   ? ca[0] * .003 / 100
                                            : k == 2 ? ca[1] * .003 / 100
                                                     : 0;
                    px = cx + (px - cx) * (1 + aberration);
                    py = cy + (py - cy) * (1 + aberration);
                    double gain =
                        std::exp2(1.5 * o.vignetting / 100 *
                                  std::pow(std::min(1.5, std::hypot(px - cx, py - cy) / hd),
                                           1 + 6 * o.vignettingMidpoint / 100));
                    if (o.embeddedVignette && o.profileVignetting != 0) {
                        const auto &v = *o.embeddedVignette;
                        const auto u = (px + .5 - v.centerX * f.width) / (v.radius * longEdge),
                                   z = (py + .5 - v.centerY * f.height) / (v.radius * longEdge),
                                   rr = u * u + z * z;
                        double value = 0;
                        for (int j = 4; j >= 0; --j)
                            value = (value + v.coefficients[j]) * rr;
                        gain *= std::max(0., 1 + value * o.profileVignetting / 100);
                    }
                    if (k == 1)
                        commonGain = gain;
                    if (moves) {
                        const auto p = sample(src, px, py, k, k != 1);
                        rgb[k] = p.value;
                        if (k == 1)
                            coverage = p.alpha;
                    } else
                        rgb[k] = src.rgb[i][k];
                }
                f.rgb[i] = dd::scale(rgb, commonGain);
                f.alpha[i] = coverage;
            }
        if (o.purpleDefringe > 0 || o.greenDefringe > 0) {
            std::vector<dd::Rgb> lab;
            for (auto c : f.rgb)
                lab.push_back(dd::lab(c));
            const auto radius = static_cast<std::size_t>(
                std::max(1., std::round((1 + std::max(o.purpleDefringe, o.greenDefringe) * .3) *
                                        longEdge / 4000)));
            dd::Plane lightness;
            for (auto c : lab)
                lightness.push_back(c[0]);
            const auto low = dd::extrema(f, lightness, radius, false),
                       high = dd::extrema(f, lightness, radius, true);
            const auto hueWeight = [](double h, double a, double b) {
                const auto d = std::abs(dd::wrap((h - (a + b) / 2) * std::numbers::pi / 180)) *
                               180 / std::numbers::pi;
                return 1 - dd::smooth((b - a) / 2, (b - a) / 2 + 8, d);
            };
            for (std::size_t i = 0; i < f.rgb.size(); ++i) {
                if (f.alpha[i] == 0)
                    continue;
                const auto l = lab[i];
                const auto chroma = std::hypot(l[1], l[2]),
                           hue = std::atan2(l[2], l[1]) * 180 / std::numbers::pi;
                const auto strength =
                    std::max(
                        std::min(1., o.purpleDefringe / 8) *
                            hueWeight(hue, 240 + 1.2 * o.purpleHueLow, 240 + 1.2 * o.purpleHueHigh),
                        std::min(1., o.greenDefringe / 8) *
                            hueWeight(hue, 90 + 1.2 * o.greenHueLow, 90 + 1.2 * o.greenHueHigh)) *
                    dd::smooth(.08, .25, high[i] - low[i]) * dd::smooth(.015, .05, chroma);
                f.rgb[i] = dd::fromLab({l[0], l[1] * (1 - strength), l[2] * (1 - strength)});
            }
        }
        auto out = dd::encode(f, image);
        if (moves)
            for (std::size_t i = 0; i < f.rgb.size(); ++i) {
                out.pixels[i].a = static_cast<float>(detail::clamp(f.alpha[i]));
                if (f.alpha[i] == 0)
                    out.pixels[i] = {0, 0, 0, 0};
            }
        return out;
    });
}
} // namespace iiGraphicAdjust
