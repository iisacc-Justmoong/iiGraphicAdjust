#include "LensFilter.h"
#include "FilterInternal.h"
namespace iiGraphicAdjust {
namespace fd = filter_detail;
namespace {
bool valid(const LensCorrectionOptions &o) {
    for (auto v :
         {o.distortion, o.redCyan, o.blueYellow, o.vignetteAmount, o.vertical, o.horizontal})
        if (!detail::inRange(v, -100, 100))
            return false;
    if (!detail::inRange(o.vignetteMidpoint, 0, 100) || !detail::inRange(o.angle, -180, 180) ||
        !detail::inRange(o.scale, 50, 400) || unsigned(o.edge) > 2)
        return false;
    for (auto v : {o.edgeColor.r, o.edgeColor.g, o.edgeColor.b, o.edgeColor.a})
        if (!detail::inRange(v, 0, 1))
            return false;
    if (o.profile) {
        auto p = *o.profile;
        for (auto v : {p.k1, p.k2, p.p1, p.p2, p.vignette[0], p.vignette[1], p.vignette[2]})
            if (!detail::inRange(v, -4, 4))
                return false;
        for (auto v : {p.caRed, p.caBlue})
            if (!detail::inRange(v, -.05, .05))
                return false;
    }
    return true;
}
struct Mapping {
    std::array<double, 2> green, red, blue;
    double gain;
};
std::optional<Mapping> map(ImageView image, const LensCorrectionOptions &o, double x, double y) {
    auto p = o.profile.value_or(LensFilterProfile{});
    double cx = image.width / 2., cy = image.height / 2.,
           r = std::max(2., std::hypot(image.width, image.height)) / 2;
    double a = o.angle * fd::pi / 180, dx = (x - cx) / (o.scale / 100),
           dy = (y - cy) / (o.scale / 100), nx = (std::cos(a) * dx + std::sin(a) * dy) / r,
           ny = (-std::sin(a) * dx + std::cos(a) * dy) / r;
    double w = 1 + o.horizontal / 200 * nx + o.vertical / 200 * ny;
    if (w <= .05)
        return {};
    double ux = nx / w, uy = ny / w, r2 = ux * ux + uy * uy,
           k1 = (o.correctDistortion ? p.k1 : 0) - o.distortion / 400,
           k2 = o.correctDistortion ? p.k2 : 0, p1 = o.correctDistortion ? p.p1 : 0,
           p2 = o.correctDistortion ? p.p2 : 0;
    double radial = 1 + k1 * r2 + k2 * r2 * r2,
           sx = ux * radial + 2 * p1 * ux * uy + p2 * (r2 + 2 * ux * ux),
           sy = uy * radial + p1 * (r2 + 2 * uy * uy) + 2 * p2 * ux * uy;
    double sr2 = sx * sx + sy * sy,
           falloff = o.correctVignette
                         ? std::max(.05, 1 + p.vignette[0] * sr2 + p.vignette[1] * sr2 * sr2 +
                                             p.vignette[2] * sr2 * sr2 * sr2)
                         : 1;
    double start = (1 - o.vignetteMidpoint / 100) * .9,
           gain = std::exp2(o.vignetteAmount / 100 * 1.5 * fd::smooth(start, 1, std::sqrt(sr2))) /
                  falloff;
    auto point = [&](double k) { return std::array{cx + sx * k * r, cy + sy * k * r}; };
    return Mapping{point(1), point(1 + (o.correctCa ? p.caRed : 0) - o.redCyan * 4e-5),
                   point(1 + (o.correctCa ? p.caBlue : 0) - o.blueYellow * 4e-5), gain};
}
double cubic(double x) {
    x = std::abs(x);
    if (x <= 1)
        return (1.5 * x - 2.5) * x * x + 1;
    if (x < 2)
        return ((-.5 * x + 2.5) * x - 4) * x + 2;
    return 0;
}
fd::Pixel sample(ImageView image, std::array<double, 2> p, const LensCorrectionOptions &o) {
    double x = p[0] - .5, y = p[1] - .5;
    if (o.edge == LensFilterEdge::Extension) {
        x = std::clamp(x, -2., double(image.width + 1));
        y = std::clamp(y, -2., double(image.height + 1));
    } else if (x < -2 || y < -2 || x > double(image.width + 1) || y > double(image.height + 1))
        return o.edge == LensFilterEdge::Color ? fd::premul(o.edgeColor) : fd::Pixel{};
    long long xx = static_cast<long long>(std::floor(x)),
              yy = static_cast<long long>(std::floor(y));
    fd::Pixel out{};
    for (int dy = -1; dy <= 2; ++dy)
        for (int dx = -1; dx <= 2; ++dx) {
            auto px = xx + dx, py = yy + dy;
            auto q = fd::get(image, px, py,
                             o.edge == LensFilterEdge::Extension ? fd::Edge::Repeat
                                                                 : fd::Edge::Transparent);
            if (o.edge == LensFilterEdge::Color &&
                (px < 0 || py < 0 || px >= static_cast<long long>(image.width) ||
                 py >= static_cast<long long>(image.height)))
                q = o.edgeColor;
            auto v = fd::premul(q);
            double k = cubic(x - px) * cubic(y - py);
            for (int c = 0; c < 4; ++c)
                out[c] += v[c] * k;
        }
    return out;
}
} // namespace
Result<LensFilterProfile> genericLensProfile(double focal) {
    if (!detail::inRange(focal, 1, 10000))
        return std::unexpected(fd::invalid());
    constexpr std::array<std::array<double, 5>, 7> table{{{12, -.16, .04, -.60, .15},
                                                          {16, -.11, .025, -.50, .12},
                                                          {24, -.06, .012, -.38, .08},
                                                          {35, -.025, .004, -.28, .05},
                                                          {50, 0, 0, -.22, .04},
                                                          {85, .012, 0, -.16, .02},
                                                          {200, .02, 0, -.10, .01}}};
    double f = std::clamp(focal, 12., 200.);
    std::size_t i = 0;
    while (i < 5 && f > table[i + 1][0])
        ++i;
    auto a = table[i], b = table[i + 1];
    double t = (std::log(f) - std::log(a[0])) / (std::log(b[0]) - std::log(a[0]));
    auto lerp = [&](int c) { return a[c] + t * (b[c] - a[c]); };
    return LensFilterProfile{lerp(1), lerp(2), 0, 0, {lerp(3), lerp(4), 0}, 0, 0};
}
Result<Image> lensCorrection(ImageView image, const LensCorrectionOptions &o) {
    if (!valid(o))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        const auto p = o.profile.value_or(LensFilterProfile{});
        const bool distortionOff =
            !o.correctDistortion || (p.k1 == 0 && p.k2 == 0 && p.p1 == 0 && p.p2 == 0);
        const bool vignetteOff = !o.correctVignette || p.vignette == std::array<double, 3>{};
        const bool caOff = !o.correctCa || (p.caRed == 0 && p.caBlue == 0);
        if (distortionOff && vignetteOff && caOff && o.distortion == 0 && o.redCyan == 0 &&
            o.blueYellow == 0 && o.vignetteAmount == 0 && o.vertical == 0 && o.horizontal == 0 &&
            o.angle == 0 && o.scale == 100)
            return out;
        for (std::size_t i = 0; i < out.pixels.size(); ++i) {
            double x = i % image.width + .5, y = i / image.width + .5;
            auto m = map(image, o, x, y);
            if (!m) {
                out.pixels[i] = o.edge == LensFilterEdge::Color ? o.edgeColor : Rgba{0, 0, 0, 0};
                continue;
            }
            if (m->green == std::array{x, y} && m->red == m->green && m->blue == m->green &&
                m->gain == 1)
                continue;
            auto g = fd::straight(sample(image, m->green, o));
            if (g.a == 0) {
                out.pixels[i] = {0, 0, 0, 0};
                continue;
            }
            auto r = fd::straight(sample(image, m->red, o)),
                 b = fd::straight(sample(image, m->blue, o));
            out.pixels[i] = {float(detail::clamp((r.a > 0 ? r.r : g.r) * m->gain)),
                             float(detail::clamp(g.g * m->gain)),
                             float(detail::clamp((b.a > 0 ? b.b : g.b) * m->gain)), g.a};
        }
        return out;
    });
}
Result<double> autoLensScale(ImageView image, const LensCorrectionOptions &o) {
    if (!valid(o))
        return std::unexpected(fd::invalid());
    if (auto v = detail::validate(image); !v)
        return std::unexpected(v.error());
    auto covers = [&](double scale) {
        auto p = o;
        p.scale = scale;
        for (int i = 0; i <= 40; ++i) {
            double t = i / 40.;
            const double xEdge = .5 + t * (image.width - 1), yEdge = .5 + t * (image.height - 1);
            for (auto [x, y] : std::array<std::array<double, 2>, 4>{{{xEdge, .5},
                                                                     {xEdge, image.height - .5},
                                                                     {.5, yEdge},
                                                                     {image.width - .5, yEdge}}}) {
                auto m = map(image, p, x, y);
                if (!m || m->green[0] < .5 - 1e-9 || m->green[1] < .5 - 1e-9 ||
                    m->green[0] > image.width - .5 + 1e-9 || m->green[1] > image.height - .5 + 1e-9)
                    return false;
            }
        }
        return true;
    };
    if (covers(o.scale))
        return o.scale;
    if (!covers(400))
        return std::unexpected(
            Error{ErrorCode::InvalidParameter, "No covering lens scale at or below 400 percent"});
    double low = o.scale, high = 400;
    for (int i = 0; i < 30; ++i) {
        double mid = (low + high) / 2;
        if (covers(mid))
            high = mid;
        else
            low = mid;
    }
    return high;
}
} // namespace iiGraphicAdjust
