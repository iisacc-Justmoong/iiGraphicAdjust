#include "DistortionFilters.h"
#include "FilterInternal.h"
namespace iiGraphicAdjust {
namespace fd = filter_detail;
namespace {
fd::Edge edge(FilterEdge e) {
    return e == FilterEdge::Wrap     ? fd::Edge::Wrap
           : e == FilterEdge::Repeat ? fd::Edge::Repeat
                                     : fd::Edge::Transparent;
}
bool validEdge(FilterEdge e) {
    return unsigned(e) <= 2;
}
double sphere(double r, double amount) {
    return r + std::abs(amount) *
                   ((amount >= 0 ? std::asin(r) * 2 / fd::pi : std::sin(r * fd::pi / 2)) - r);
}
double waveShape(WaveType type, double phase) {
    double p = std::fmod(phase / (2 * fd::pi), 1);
    if (p < 0)
        p += 1;
    return type == WaveType::Sine       ? std::sin(phase)
           : type == WaveType::Triangle ? 1 - 4 * std::abs(p - .5)
           : p < .5                     ? 1
                                        : -1;
}
double curve(std::span<const ShearPoint> points, double t) {
    if (points.empty())
        return 0;
    if (points.size() == 1 || t <= points.front().position)
        return points.front().offset;
    if (t >= points.back().position)
        return points.back().offset;
    auto hi = std::upper_bound(points.begin(), points.end(), t,
                               [](double v, auto p) { return v < p.position; });
    auto i = std::size_t(hi - points.begin() - 1), last = points.size() - 1;
    auto p1 = points[i], p2 = points[i + 1], p0 = i ? points[i - 1] : p1,
         p3 = i + 2 <= last ? points[i + 2] : p2;
    double span = p2.position - p1.position, s = (t - p1.position) / span,
           m1 = i ? (p2.offset - p0.offset) / (p2.position - p0.position) * span
                  : p2.offset - p1.offset,
           m2 = i + 2 <= last ? (p3.offset - p1.offset) / (p3.position - p1.position) * span
                              : p2.offset - p1.offset;
    return (2 * s * s * s - 3 * s * s + 1) * p1.offset + (s * s * s - 2 * s * s + s) * m1 +
           (-2 * s * s * s + 3 * s * s) * p2.offset + (s * s * s - s * s) * m2;
}
} // namespace
Result<Image> offset(ImageView image, std::int32_t horizontal, std::int32_t vertical,
                     FilterEdge e) {
    if (!validEdge(e))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::output(image);
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            out.pixels[i] = fd::get(image, static_cast<long long>(i % image.width) - horizontal,
                                    static_cast<long long>(i / image.width) - vertical, edge(e));
        return out;
    });
}
Result<Image> twirl(ImageView image, double angle) {
    if (!detail::inRange(angle, -999, 999))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (angle == 0)
            return detail::copy(image);
        double cx = image.width / 2., cy = image.height / 2.,
               rr = std::min(image.width, image.height) / 2.;
        return fd::remap(image, fd::Edge::Repeat, [&](double x, double y) {
            double dx = x - cx, dy = y - cy, r = std::hypot(dx, dy);
            if (r >= rr)
                return std::array{x, y};
            double t = angle * fd::pi / 180 * (1 - r / rr);
            return std::array{cx + dx * std::cos(t) - dy * std::sin(t),
                              cy + dx * std::sin(t) + dy * std::cos(t)};
        });
    });
}
Result<Image> pinch(ImageView image, double amount) {
    if (!detail::inRange(amount, -100, 100))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (amount == 0)
            return detail::copy(image);
        double cx = image.width / 2., cy = image.height / 2.,
               rr = std::min(image.width, image.height) / 2.;
        return fd::remap(image, fd::Edge::Repeat, [&](double x, double y) {
            double dx = x - cx, dy = y - cy, r = std::hypot(dx, dy);
            if (r >= rr || r == 0)
                return std::array{x, y};
            double rs = rr * std::pow(r / rr, 1 - amount / 200);
            return std::array{cx + dx * rs / r, cy + dy * rs / r};
        });
    });
}
Result<Image> spherize(ImageView image, const SpherizeOptions &o) {
    if (!detail::inRange(o.amount, -100, 100) || unsigned(o.mode) > 2)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (o.amount == 0)
            return detail::copy(image);
        double cx = image.width / 2., cy = image.height / 2., rr = std::min(cx, cy);
        return fd::remap(image, fd::Edge::Repeat, [&](double x, double y) {
            double dx = x - cx, dy = y - cy;
            if (o.mode == SpherizeMode::Normal) {
                double r = std::hypot(dx, dy);
                if (r >= rr || r == 0)
                    return std::array{x, y};
                double rs = rr * sphere(r / rr, o.amount / 100);
                return std::array{cx + dx * rs / r, cy + dy * rs / r};
            }
            if (o.mode == SpherizeMode::HorizontalOnly)
                return std::array{
                    cx + std::copysign(cx * sphere(std::min(1., std::abs(dx) / cx), o.amount / 100),
                                       dx),
                    y};
            return std::array{
                x, cy + std::copysign(cy * sphere(std::min(1., std::abs(dy) / cy), o.amount / 100),
                                      dy)};
        });
    });
}
Result<Image> wave(ImageView image, const WaveOptions &o) {
    if (o.generators < 1 || o.generators > 64 || !detail::inRange(o.wavelengthMin, 1, 999) ||
        !detail::inRange(o.wavelengthMax, 1, 999) || o.wavelengthMin > o.wavelengthMax ||
        !detail::inRange(o.amplitudeMin, 0, 999) || !detail::inRange(o.amplitudeMax, 0, 999) ||
        o.amplitudeMin > o.amplitudeMax || unsigned(o.type) > 2 || !validEdge(o.edge))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (o.amplitudeMax == 0)
            return detail::copy(image);
        std::vector<std::array<double, 4>> gens;
        for (unsigned i = 0; i < o.generators; ++i)
            gens.push_back(
                {o.wavelengthMin + (o.wavelengthMax - o.wavelengthMin) * fd::hash(i, 1, 7, o.seed),
                 o.amplitudeMin + (o.amplitudeMax - o.amplitudeMin) * fd::hash(i, 2, 7, o.seed),
                 2 * fd::pi * fd::hash(i, 3, 7, o.seed), 2 * fd::pi * fd::hash(i, 4, 7, o.seed)});
        return fd::remap(image, edge(o.edge), [&](double x, double y) {
            double dx = 0, dy = 0;
            for (auto [len, amp, p1, p2] : gens) {
                dx += amp * waveShape(o.type, 2 * fd::pi * y / len + p1);
                dy += amp * waveShape(o.type, 2 * fd::pi * x / len + p2);
            }
            return std::array{x + dx / gens.size(), y + dy / gens.size()};
        });
    });
}
Result<Image> ripple(ImageView image, const RippleOptions &o) {
    if (!detail::inRange(o.amount, -999, 999) || unsigned(o.size) > 2)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (o.amount == 0)
            return detail::copy(image);
        double len = std::array{8., 16., 32.}[unsigned(o.size)], a = o.amount / 100 * len / 8;
        return fd::remap(image, fd::Edge::Repeat, [&](double x, double y) {
            return std::array{x + a * std::sin(2 * fd::pi * y / len),
                              y + a * std::sin(2 * fd::pi * x / len)};
        });
    });
}
Result<Image> polarCoordinates(ImageView image, PolarMode mode) {
    if (unsigned(mode) > 1)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        double w = image.width, h = image.height, cx = w / 2, cy = h / 2, rmax = std::min(w, h) / 2;
        return fd::remap(image, fd::Edge::Transparent, [&](double x, double y) {
            if (mode == PolarMode::RectangularToPolar) {
                double th = std::atan2(x - cx, cy - y);
                if (th < 0)
                    th += 2 * fd::pi;
                return std::array{th / (2 * fd::pi) * w, std::hypot(x - cx, y - cy) / rmax * h};
            }
            double theta = x / w * 2 * fd::pi, r = y / h * rmax;
            return std::array{cx + r * std::sin(theta), cy - r * std::cos(theta)};
        });
    });
}
Result<Image> displace(ImageView image, ImageView map, const DisplaceOptions &o) {
    if (!detail::inRange(o.horizontal, -999, 999) || !detail::inRange(o.vertical, -999, 999) ||
        !validEdge(o.edge))
        return std::unexpected(fd::invalid());
    if (auto v = detail::validate(map); !v)
        return std::unexpected(v.error());
    return fd::guarded(image, [&] {
        if (o.horizontal == 0 && o.vertical == 0)
            return detail::copy(image);
        return fd::remap(image, edge(o.edge), [&](double x, double y) {
            double u = o.stretch ? x / image.width * map.width : std::fmod(x, double(map.width)),
                   v = o.stretch ? y / image.height * map.height : std::fmod(y, double(map.height));
            auto p = fd::straight(fd::sample(map, u, v));
            double mx = p.a > 0 ? p.r : .5, my = p.a > 0 ? p.g : .5;
            return std::array{x + (mx - .5) * 2 * o.horizontal / 100 * 128,
                              y + (my - .5) * 2 * o.vertical / 100 * 128};
        });
    });
}
Result<Image> shear(ImageView image, std::span<const ShearPoint> points, FilterEdge e) {
    if (!validEdge(e) || points.size() > 4096)
        return std::unexpected(fd::invalid());
    for (std::size_t i = 0; i < points.size(); ++i)
        if (!detail::inRange(points[i].position, 0, 1) ||
            !detail::inRange(points[i].offset, -1, 1) ||
            (i && points[i].position - points[i - 1].position < 1e-9))
            return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (std::all_of(points.begin(), points.end(), [](auto p) { return p.offset == 0; }))
            return detail::copy(image);
        return fd::remap(image, edge(e), [&](double x, double y) {
            return std::array{x - curve(points, y / image.height) * image.width / 2, y};
        });
    });
}
Result<Image> zigZag(ImageView image, const ZigZagOptions &o) {
    if (!detail::inRange(o.amount, -100, 100) || !detail::inRange(o.ridges, 0, 20) ||
        unsigned(o.style) > 2)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        if (o.amount == 0)
            return detail::copy(image);
        double cx = image.width / 2., cy = image.height / 2., rr = std::min(cx, cy),
               k = std::max(.5, o.ridges);
        return fd::remap(image, fd::Edge::Repeat, [&](double x, double y) {
            double dx = x - cx, dy = y - cy, r = std::hypot(dx, dy);
            if (r >= rr || r == 0)
                return std::array{x, y};
            double wave = std::sin(2 * fd::pi * k * r / rr) * (1 - r / rr), radial = 0, angular = 0,
                   a = o.amount / 100;
            if (o.style == ZigZagStyle::OutFromCenter)
                radial = a * wave * rr / (4 * k);
            else if (o.style == ZigZagStyle::AroundCenter)
                angular = a * wave * fd::pi / (2 * k);
            else {
                radial = a * wave * rr / (8 * k);
                angular = a * wave * fd::pi / (4 * k);
            }
            double th = std::atan2(dy, dx) + angular, rs = std::max(0., r + radial);
            return std::array{cx + rs * std::cos(th), cy + rs * std::sin(th)};
        });
    });
}
} // namespace iiGraphicAdjust
