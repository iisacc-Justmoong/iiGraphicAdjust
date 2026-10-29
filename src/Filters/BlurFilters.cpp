#include "BlurFilters.h"
#include "FilterInternal.h"

namespace iiGraphicAdjust {
namespace fd = filter_detail;
namespace {
bool range(double v, double lo, double hi) {
    return detail::inRange(v, lo, hi);
}
template <class F> Image averages(ImageView image, F samples, fd::Edge edge = fd::Edge::Repeat) {
    auto out = detail::output(image);
    std::vector<std::array<double, 2>> pts;
    for (std::size_t y = 0; y < image.height; ++y)
        for (std::size_t x = 0; x < image.width; ++x) {
            pts.clear();
            samples(x + .5, y + .5, pts);
            fd::Pixel acc{};
            for (auto p : pts) {
                auto s = fd::sample(image, p[0], p[1], edge);
                for (int c = 0; c < 4; ++c)
                    acc[c] += s[c] / pts.size();
            }
            out.pixels[y * image.width + x] = fd::straight(acc);
        }
    return out;
}
template <class F> Result<Image> sharpenWith(ImageView image, double radius, F process) {
    return fd::guarded(image, [&] {
        auto blurred = fd::gauss(image, radius);
        auto out = detail::copy(image);
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            if (out.pixels[i].a > 0) {
                const auto b = fd::unpremulRgb(blurred[i]);
                auto p = out.pixels[i];
                out.pixels[i] = {float(detail::clamp(process(p.r, b[0]))),
                                 float(detail::clamp(process(p.g, b[1]))),
                                 float(detail::clamp(process(p.b, b[2]))), p.a};
            }
        return out;
    });
}
struct Span {
    int y, a, b;
};
template <class Inside> std::vector<Span> spans(double radius, Inside inside) {
    if (radius < .5)
        return {{0, 0, 0}};
    std::vector<Span> out;
    int r = int(std::ceil(radius));
    for (int y = -r; y <= r; ++y) {
        int start = 0;
        bool active = false;
        for (int x = -r; x <= r + 1; ++x) {
            bool hit = x <= r && inside(x / radius, y / radius);
            if (hit && !active) {
                start = x;
                active = true;
            }
            if (!hit && active) {
                out.push_back({y, start, x - 1});
                active = false;
            }
        }
    }
    if (out.empty())
        out.push_back({0, 0, 0});
    return out;
}
struct Prefix {
    std::size_t w, h;
    std::vector<fd::Pixel> rows;
    explicit Prefix(ImageView image, double brightness = 0, double threshold = 255)
        : w(image.width), h(image.height), rows((w + 1) * h) {
        for (std::size_t y = 0; y < h; ++y)
            for (std::size_t x = 0; x < w; ++x) {
                auto p = image.pixels[y * w + x];
                auto v = fd::premul(p);
                double gain =
                    1 + brightness * .04 *
                            fd::smooth(threshold / 255, threshold / 255 + .02, fd::luma(p));
                for (int c = 0; c < 3; ++c)
                    v[c] *= gain;
                for (int c = 0; c < 4; ++c)
                    rows[y * (w + 1) + x + 1][c] = rows[y * (w + 1) + x][c] + v[c];
            }
    }
    fd::Pixel mean(const std::vector<Span> &ss, long long x, long long y) const {
        fd::Pixel acc{};
        double count = 0;
        for (auto s : ss) {
            auto yy = std::size_t(std::clamp<long long>(y + s.y, 0, h - 1));
            auto a = x + s.a, b = x + s.b;
            auto lo = std::size_t(std::clamp<long long>(a, 0, w));
            auto hi = std::size_t(std::clamp<long long>(b + 1, 0, w));
            auto left = std::max(0LL, std::min(b + 1, 0LL) - a),
                 right = std::max(0LL, b - std::max(a, static_cast<long long>(w)) + 1);
            for (int c = 0; c < 4; ++c)
                acc[c] += rows[yy * (w + 1) + hi][c] - rows[yy * (w + 1) + lo][c] +
                          left * (rows[yy * (w + 1) + 1][c] - rows[yy * (w + 1)][c]) +
                          right * (rows[yy * (w + 1) + w][c] - rows[yy * (w + 1) + w - 1][c]);
            count += s.b - s.a + 1.;
        }
        for (auto &v : acc)
            v /= count;
        return acc;
    }
};
double polygonRadius(double theta, unsigned n, double rotation) {
    double seg = 2 * fd::pi / n;
    auto a = std::fmod(theta - rotation, seg);
    if (a < 0)
        a += seg;
    a -= seg / 2;
    return std::cos(fd::pi / n) / std::cos(a);
}
bool shapeHit(BlurShape shape, double u, double v) {
    switch (shape) {
    case BlurShape::Circle:
        return u * u + v * v <= 1;
    case BlurShape::Ring:
        return u * u + v * v >= .36 && u * u + v * v <= 1;
    case BlurShape::Square:
        return true;
    case BlurShape::Diamond:
        return std::abs(u) + std::abs(v) <= 1;
    case BlurShape::Triangle:
        return v <= .75 && std::abs(u) <= (v + 1) / 1.75;
    case BlurShape::Hexagon:
        return std::hypot(u, v) <= polygonRadius(std::atan2(v, u), 6, 0);
    case BlurShape::Heart: {
        double x = u * 1.2, y = -v * 1.2 + .15;
        return std::pow(x * x + y * y - 1, 3) - x * x * y * y * y <= 0;
    }
    case BlurShape::Cross:
        return std::abs(u) <= .3 || std::abs(v) <= .3;
    case BlurShape::Star: {
        bool in = false;
        double xj = 0, yj = -1;
        for (int i = 1; i <= 10; ++i) {
            int j = i % 10;
            double r = j % 2 ? .42 : 1, a = -fd::pi / 2 + j * fd::pi / 5;
            double xi = r * std::cos(a), yi = r * std::sin(a);
            if ((yi > v) != (yj > v) && u < (xj - xi) * (v - yi) / (yj - yi) + xi)
                in = !in;
            xj = xi;
            yj = yi;
        }
        return in;
    }
    }
    return false;
}
template <class Sigma> Image variable(ImageView image, double maximum, Sigma sigma) {
    if (maximum < .3)
        return detail::copy(image);
    std::array<double, 7> levels{};
    for (int k = 1; k < 7; ++k)
        levels[k] = maximum / std::exp2(6 - k);
    std::vector<double> sig(image.pixels.size());
    fd::Buffer acc(sig.size());
    for (std::size_t i = 0; i < sig.size(); ++i)
        sig[i] = std::clamp(sigma(i % image.width + .5, i / image.width + .5), 0., maximum);
    for (int k = 0; k < 7; ++k) {
        auto weight = [&](double s) {
            if (k > 0 && s >= levels[k - 1] && s <= levels[k])
                return (s - levels[k - 1]) / (levels[k] - levels[k - 1]);
            if (k < 6 && s >= levels[k] && s <= levels[k + 1])
                return (levels[k + 1] - s) / (levels[k + 1] - levels[k]);
            return k == 6 && s >= levels[k] ? 1. : 0.;
        };
        if (std::none_of(sig.begin(), sig.end(), [&](double s) { return weight(s) > 0; }))
            continue;
        auto b = fd::gauss(image, levels[k]);
        for (std::size_t i = 0; i < sig.size(); ++i)
            for (int c = 0; c < 4; ++c)
                acc[i][c] += b[i][c] * weight(sig[i]);
    }
    auto out = fd::write(image, acc);
    for (std::size_t i = 0; i < sig.size(); ++i)
        if (sig[i] == 0)
            out.pixels[i] = image.pixels[i];
    return out;
}
std::array<double, 3> pinDistance(double x, double y, ImageView image, double px, double py,
                                  double rx, double ry, double angle, double power = 2) {
    double ss = std::min(image.width, image.height), a = angle * fd::pi / 180,
           dx = x - px * image.width, dy = y - py * image.height;
    double u = (dx * std::cos(a) + dy * std::sin(a)) / (rx * ss),
           v = (-dx * std::sin(a) + dy * std::cos(a)) / (ry * ss);
    return {std::pow(std::pow(std::abs(u), power) + std::pow(std::abs(v), power), 1 / power), u, v};
}
} // namespace
Result<Image> filterGaussianBlur(ImageView image, double sigma) {
    if (!range(sigma, 0, 1000))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        return sigma == 0 ? detail::copy(image) : fd::write(image, fd::gauss(image, sigma));
    });
}
Result<Image> boxBlur(ImageView image, double radius) {
    if (!range(radius, 0, 2000))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        return radius < .5 ? detail::copy(image)
                           : fd::write(image, fd::boxed(image, std::size_t(std::round(radius))));
    });
}
Result<Image> blur(ImageView image) {
    return filterGaussianBlur(image, .6);
}
Result<Image> blurMore(ImageView image) {
    return filterGaussianBlur(image, 1.4);
}
Result<Image> motionBlur(ImageView image, const MotionBlurOptions &o) {
    if (!range(o.angle, -360, 360) || !range(o.distance, 0, 2000))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (o.distance < .5)
            return detail::copy(image);
        int n = int(std::ceil(o.distance));
        double a = o.angle * fd::pi / 180;
        return averages(image, [&](double x, double y, auto &pts) {
            for (int j = 0; j <= n; ++j) {
                double t = double(j) / n - .5;
                pts.push_back({x + std::cos(a) * o.distance * t, y - std::sin(a) * o.distance * t});
            }
        });
    });
}
Result<Image> radialBlur(ImageView image, const RadialBlurOptions &o) {
    if (!range(o.amount, 0, 100) || !range(o.centerX, 0, 1) || !range(o.centerY, 0, 1) ||
        (o.method != RadialBlurMethod::Spin && o.method != RadialBlurMethod::Zoom))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (o.amount == 0)
            return detail::copy(image);
        double cx = o.centerX * image.width, cy = o.centerY * image.height;
        return averages(image, [&](double x, double y, auto &pts) {
            double dx = x - cx, dy = y - cy, r = std::hypot(dx, dy), a = o.amount * fd::pi / 180;
            int n = std::clamp(
                int(std::ceil((o.method == RadialBlurMethod::Spin ? a : o.amount / 200) * r)), 1,
                64);
            for (int j = 0; j <= n; ++j)
                if (o.method == RadialBlurMethod::Spin) {
                    double t = (double(j) / n - .5) * a;
                    pts.push_back({cx + dx * std::cos(t) - dy * std::sin(t),
                                   cy + dx * std::sin(t) + dy * std::cos(t)});
                } else {
                    double k = 1 - o.amount / 200 * j / n;
                    pts.push_back({cx + dx * k, cy + dy * k});
                }
        });
    });
}
Result<Image> surfaceBlur(ImageView image, const SurfaceBlurOptions &o) {
    if (!range(o.radius, 0, 100) || !range(o.threshold, 0, 255))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        int r = int(std::round(o.radius));
        double t = o.threshold / 255 * 2.5;
        if (r == 0 || t == 0)
            return out;
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            if (image.pixels[i].a > 0) {
                auto p = image.pixels[i];
                std::array<double, 3> sums{}, weights{};
                long long x = i % image.width, y = i / image.width;
                for (int dy = -r; dy <= r; ++dy)
                    for (int dx = -r; dx <= r; ++dx) {
                        auto q = fd::get(image, x + dx, y + dy);
                        if (q.a == 0)
                            continue;
                        std::array<double, 3> v{q.r, q.g, q.b}, me{p.r, p.g, p.b};
                        for (int c = 0; c < 3; ++c) {
                            double w = q.a * std::max(0., 1 - std::abs(v[c] - me[c]) / t);
                            sums[c] += w * v[c];
                            weights[c] += w;
                        }
                    }
                out.pixels[i] = {float(sums[0] / weights[0]), float(sums[1] / weights[1]),
                                 float(sums[2] / weights[2]), p.a};
            }
        return out;
    });
}
Result<Image> unsharpMask(ImageView image, const UnsharpMaskOptions &o) {
    if (!range(o.amount, 0, 500) || !range(o.radius, 0, 1000) || !range(o.threshold, 0, 255))
        return std::unexpected(fd::invalid());
    return sharpenWith(image, o.radius, [&](double p, double b) {
        double d = p - b;
        return std::abs(d) + 1e-6 >= o.threshold / 255 ? p + o.amount / 100 * d : p;
    });
}
Result<Image> filterSharpen(ImageView i) {
    return unsharpMask(i, {60, .5, 0});
}
Result<Image> sharpenMore(ImageView i) {
    return unsharpMask(i, {150, .6, 0});
}
Result<Image> sharpenEdges(ImageView i) {
    return unsharpMask(i, {100, .8, 6});
}
Result<Image> smartSharpen(ImageView image, const SmartSharpenOptions &o) {
    if (!range(o.amount, 0, 500) || !range(o.radius, 0, 64) || !range(o.reduceNoise, 0, 100))
        return std::unexpected(fd::invalid());
    return sharpenWith(image, o.radius, [&](double p, double b) {
        double d = p - b;
        return p + o.amount / 100 * d * (1 - o.reduceNoise / 100 * std::exp(-std::abs(d) * 40));
    });
}
Result<Image> highPass(ImageView image, double radius) {
    if (!range(radius, 0, 1000))
        return std::unexpected(fd::invalid());
    return sharpenWith(image, radius, [](double p, double b) { return .5 + p - b; });
}
Result<Image> despeckle(ImageView image) {
    return surfaceBlur(image, {2, 12});
}
Result<Image> smartBlur(ImageView image, const SmartBlurOptions &o) {
    if (!range(o.radius, .1, 100) || !range(o.threshold, .1, 100) || unsigned(o.quality) > 2 ||
        unsigned(o.mode) > 2)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        int r = int(std::ceil(o.radius)), target = std::array{4, 7, 12}[unsigned(o.quality)],
            stride = std::max(1, (r + target - 1) / target);
        double t = o.threshold / 255;
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            if (image.pixels[i].a > 0) {
                long long x = i % image.width, y = i / image.width;
                auto p = image.pixels[i];
                auto differs = [&](Rgba q) {
                    return q.a > 0 && (std::abs(q.r - p.r) > t || std::abs(q.g - p.g) > t ||
                                       std::abs(q.b - p.b) > t);
                };
                if (o.mode != SmartBlurMode::Normal) {
                    bool edge = false;
                    for (auto [dx, dy] :
                         std::array<std::array<int, 2>, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}})
                        edge |= differs(fd::get(image, x + dx, y + dy));
                    if (edge)
                        out.pixels[i] = {1, 1, 1, p.a};
                    else if (o.mode == SmartBlurMode::EdgeOnly)
                        out.pixels[i] = {0, 0, 0, p.a};
                    continue;
                }
                fd::Pixel acc{};
                double count = 0;
                for (int dy = -r; dy <= r; dy += stride)
                    for (int dx = -r; dx <= r; dx += stride)
                        if (dx * dx + dy * dy <= o.radius * o.radius + .5) {
                            auto q = fd::get(image, x + dx, y + dy);
                            if (q.a == 0 || differs(q))
                                continue;
                            auto v = fd::premul(q);
                            for (int c = 0; c < 4; ++c)
                                acc[c] += v[c];
                            ++count;
                        }
                if (count > 0) {
                    for (auto &v : acc)
                        v /= count;
                    const auto color = fd::unpremulRgb(acc);
                    out.pixels[i] = {color[0], color[1], color[2], p.a};
                }
            }
        return out;
    });
}
Result<Image> shapeBlur(ImageView image, const ShapeBlurOptions &o) {
    if (!range(o.radius, 0, 1000) || unsigned(o.shape) > 8)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (o.radius < .5)
            return detail::copy(image);
        auto ss = spans(o.radius, [&](double x, double y) { return shapeHit(o.shape, x, y); });
        Prefix p(image);
        auto out = detail::output(image);
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            out.pixels[i] = fd::straight(p.mean(ss, i % image.width, i / image.width));
        return out;
    });
}
Result<Image> lensBlur(ImageView image, const LensBlurOptions &o) {
    if (!range(o.radius, 0, 100) || o.blades < 3 || o.blades > 8 || !range(o.curvature, 0, 100) ||
        !range(o.rotation, 0, 360) || unsigned(o.depth) > 2 || !range(o.focalDistance, 0, 255) ||
        !range(o.brightness, 0, 100) || !range(o.threshold, 0, 255) || !range(o.noise, 0, 100) ||
        unsigned(o.distribution) > 1)
        return std::unexpected(fd::invalid());
    if (o.depth == LensDepthSource::Map && !o.depthMap)
        return std::unexpected(fd::invalid());
    if (o.depthMap) {
        auto v = detail::validate(*o.depthMap);
        if (!v)
            return std::unexpected(v.error());
        if (o.depthMap->width != image.width || o.depthMap->height != image.height)
            return std::unexpected(Error{ErrorCode::SizeMismatch, "Depth map must match image"});
    }
    return fd::guarded(image, [&] {
        if (o.radius < .5 && o.brightness == 0 && o.noise == 0)
            return detail::copy(image);
        std::array<std::vector<Span>, 9> kernels;
        for (int k = 0; k < 9; ++k)
            kernels[k] = spans(o.radius * k / 8, [&](double u, double v) {
                double p = polygonRadius(std::atan2(v, u), o.blades,
                                         o.rotation * fd::pi / 180 - fd::pi / 2);
                return std::hypot(u, v) <= p + (1 - p) * o.curvature / 100;
            });
        Prefix p(image, o.brightness, o.threshold);
        auto out = detail::output(image);
        for (std::size_t i = 0; i < out.pixels.size(); ++i) {
            double lv = 8;
            if (o.depth != LensDepthSource::None) {
                if (o.depth == LensDepthSource::Map && o.depthMap->pixels[i].a == 0) {
                    lv = 0;
                } else {
                    double d = o.depth == LensDepthSource::Transparency ? image.pixels[i].a
                                                                        : o.depthMap->pixels[i].r;
                    if (o.invertDepth)
                        d = 1 - d;
                    lv = std::min(8., std::abs(d - o.focalDistance / 255) * 8);
                }
            }
            if (lv == 0 && o.brightness == 0 && o.noise == 0) {
                out.pixels[i] = image.pixels[i];
                continue;
            }
            int low = int(lv);
            auto acc = p.mean(kernels[low], i % image.width, i / image.width);
            if (low < 8 && lv > low) {
                auto next = p.mean(kernels[low + 1], i % image.width, i / image.width);
                for (int c = 0; c < 4; ++c)
                    acc[c] += (next[c] - acc[c]) * (lv - low);
            }
            auto q = fd::straight(acc);
            if (q.a > 0 && o.noise > 0) {
                std::array<float *, 3> cs{&q.r, &q.g, &q.b};
                for (int c = 0; c < 3; ++c) {
                    auto ch = o.monochromatic ? 0 : c;
                    bool gauss = o.distribution == NoiseDistribution::Gaussian;
                    double n =
                        gauss
                            ? std::sqrt(-2 * std::log(std::max(
                                                 1e-7, fd::hash(i % image.width, i / image.width,
                                                                ch * 2 + 301, o.seed)))) *
                                  std::cos(2 * fd::pi *
                                           fd::hash(i % image.width, i / image.width, ch * 2 + 302,
                                                    o.seed)) *
                                  .5
                            : 2 * fd::hash(i % image.width, i / image.width, ch + 200, o.seed) - 1;
                    *cs[c] = float(detail::clamp(*cs[c] + n * o.noise / 400));
                }
            }
            out.pixels[i] = q;
        }
        return out;
    });
}
Result<Image> tiltShift(ImageView image, const TiltShiftOptions &o) {
    if (!range(o.blur, 0, 500) || !range(o.centerX, 0, 1) || !range(o.centerY, 0, 1) ||
        !range(o.angle, -90, 90) || !range(o.focus, 0, 1) || !range(o.transition, .01, 1))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        double a = o.angle * fd::pi / 180, ss = std::min(image.width, image.height);
        return variable(image, .6 * o.blur, [&](double x, double y) {
            double d = std::abs((x - o.centerX * image.width) * std::sin(a) +
                                (y - o.centerY * image.height) * std::cos(a)) /
                       ss;
            return .6 * o.blur * fd::smooth(o.focus, o.focus + o.transition, d);
        });
    });
}
Result<Image> irisBlur(ImageView image, std::span<const IrisBlurPin> pins) {
    if (pins.size() > 64)
        return std::unexpected(fd::invalid());
    double max = 0;
    for (auto p : pins) {
        if (!range(p.x, 0, 1) || !range(p.y, 0, 1) || !range(p.radiusX, .001, 2) ||
            !range(p.radiusY, .001, 2) || !range(p.angle, -180, 180) ||
            !range(p.roundness, 0, 100) || !range(p.feather, 0, .999) || !range(p.blur, 0, 500))
            return std::unexpected(fd::invalid());
        max = std::max(max, p.blur * .6);
    }
    return fd::guarded(image, [&] {
        return variable(image, max, [&](double x, double y) {
            double s = max;
            for (auto p : pins) {
                auto d = pinDistance(x, y, image, p.x, p.y, p.radiusX, p.radiusY, p.angle,
                                     2 + p.roundness * .06)[0];
                s = std::min(s, p.blur * .6 * fd::smooth(p.feather, 1, d));
            }
            return s;
        });
    });
}
Result<Image> fieldBlur(ImageView image, std::span<const FieldBlurPin> pins) {
    if (pins.size() > 64)
        return std::unexpected(fd::invalid());
    double max = 0;
    for (auto p : pins) {
        if (!range(p.x, 0, 1) || !range(p.y, 0, 1) || !range(p.blur, 0, 500))
            return std::unexpected(fd::invalid());
        max = std::max(max, p.blur * .6);
    }
    return fd::guarded(image, [&] {
        return variable(image, max, [&](double x, double y) {
            double ws = 0, s = 0;
            for (auto p : pins) {
                double dx = x - p.x * image.width, dy = y - p.y * image.height,
                       w = 1 / (dx * dx + dy * dy + 1);
                ws += w;
                s += w * p.blur * .6;
            }
            return s / ws;
        });
    });
}
Result<Image> spinBlur(ImageView image, std::span<const SpinBlurPin> pins) {
    if (pins.size() > 64)
        return std::unexpected(fd::invalid());
    for (auto p : pins)
        if (!range(p.x, 0, 1) || !range(p.y, 0, 1) || !range(p.radiusX, .001, 2) ||
            !range(p.radiusY, .001, 2) || !range(p.angle, -180, 180) || !range(p.blurAngle, 0, 360))
            return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        for (std::size_t i = 0; i < out.pixels.size(); ++i) {
            double x = i % image.width + .5, y = i / image.width + .5;
            for (auto p : pins) {
                auto [d, u, v] = pinDistance(x, y, image, p.x, p.y, p.radiusX, p.radiusY, p.angle);
                if (d >= 1 || p.blurAngle == 0)
                    continue;
                double theta = p.blurAngle * fd::pi / 180, ss = std::min(image.width, image.height),
                       a = p.angle * fd::pi / 180;
                int n = std::clamp(
                    int(std::ceil(theta * d * std::max(p.radiusX, p.radiusY) * ss / 2)), 2, 64);
                fd::Pixel acc{};
                for (int j = 0; j < n; ++j) {
                    double phi = theta * (double(j) / (n - 1) - .5),
                           ru = (u * std::cos(phi) - v * std::sin(phi)) * p.radiusX * ss,
                           rv = (u * std::sin(phi) + v * std::cos(phi)) * p.radiusY * ss;
                    auto q =
                        fd::sample(image, p.x * image.width + ru * std::cos(a) - rv * std::sin(a),
                                   p.y * image.height + ru * std::sin(a) + rv * std::cos(a));
                    for (int c = 0; c < 4; ++c)
                        acc[c] += q[c] / n;
                }
                double k = 1 - fd::smooth(.85, 1, d);
                auto original = fd::premul(image.pixels[i]);
                for (int c = 0; c < 4; ++c)
                    acc[c] = original[c] + k * (acc[c] - original[c]);
                out.pixels[i] = fd::straight(acc);
                break;
            }
        }
        return out;
    });
}
Result<Image> pathBlur(ImageView image, std::span<const BlurPath> paths) {
    if (paths.size() > 64)
        return std::unexpected(fd::invalid());
    std::size_t total = 0;
    for (auto &p : paths) {
        total += p.points.size();
        if (p.points.size() < 2 || total > 4096 || !range(p.speed, 0, 500) ||
            !range(p.taper, 0, 100))
            return std::unexpected(fd::invalid());
        for (auto q : p.points)
            if (!range(q.x, 0, 1) || !range(q.y, 0, 1))
                return std::unexpected(fd::invalid());
    }
    return fd::guarded(image, [&] {
        struct Seg {
            double ax, ay, dx, dy, t0, t1;
            std::size_t path;
        };
        std::vector<Seg> segs;
        for (std::size_t k = 0; k < paths.size(); ++k) {
            auto &p = paths[k];
            double totalLength = 0;
            for (std::size_t j = 1; j < p.points.size(); ++j)
                totalLength += std::hypot((p.points[j].x - p.points[j - 1].x) * image.width,
                                          (p.points[j].y - p.points[j - 1].y) * image.height);
            if (totalLength < 1e-9)
                continue;
            double run = 0;
            for (std::size_t j = 1; j < p.points.size(); ++j) {
                auto a = p.points[j - 1], b = p.points[j];
                double dx = (b.x - a.x) * image.width, dy = (b.y - a.y) * image.height,
                       l = std::hypot(dx, dy);
                if (l > 1e-9)
                    segs.push_back({a.x * image.width, a.y * image.height, dx, dy,
                                    run / totalLength, (run + l) / totalLength, k});
                run += l;
            }
        }
        auto out = detail::copy(image);
        if (segs.empty())
            return out;
        for (std::size_t i = 0; i < out.pixels.size(); ++i) {
            double x = i % image.width + .5, y = i / image.width + .5, dx = 0, dy = 0, len = 0,
                   ws = 0;
            for (auto s : segs) {
                double l2 = s.dx * s.dx + s.dy * s.dy,
                       t = std::clamp(((x - s.ax) * s.dx + (y - s.ay) * s.dy) / l2, 0., 1.),
                       ex = x - s.ax - s.dx * t, ey = y - s.ay - s.dy * t,
                       w = 1 / (ex * ex + ey * ey + 1), l = std::sqrt(l2);
                auto &p = paths[s.path];
                dx += w * s.dx / l;
                dy += w * s.dy / l;
                len += w * p.speed * (1 - p.taper / 100 * (s.t0 + (s.t1 - s.t0) * t));
                ws += w;
            }
            double m = std::hypot(dx, dy);
            len /= ws;
            if (m < 1e-6 || len < .5)
                continue;
            int n = std::clamp(int(std::ceil(len / 1.5)) + 1, 2, 64);
            fd::Pixel acc{};
            for (int j = 0; j < n; ++j) {
                double t = len * (double(j) / (n - 1) - .5);
                auto q = fd::sample(image, x + dx / m * t, y + dy / m * t);
                for (int c = 0; c < 4; ++c)
                    acc[c] += q[c] / n;
            }
            out.pixels[i] = fd::straight(acc);
        }
        return out;
    });
}
} // namespace iiGraphicAdjust
