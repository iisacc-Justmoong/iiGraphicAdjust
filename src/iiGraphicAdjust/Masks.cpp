// Algorithms adapted from LightCraft; see third_party/lightcraft/LICENSE-MIT.
#include "DevelopInternal.h"
#include <iiGraphicAdjust/Masks.h>

namespace iiGraphicAdjust {
namespace dd = develop_detail;
namespace {
Result<void> validate(MaskView m) {
    const auto count = detail::pixelCount(m.width, m.height);
    if (!count)
        return std::unexpected(count.error());
    if (*count != m.coverage.size())
        return std::unexpected(Error{ErrorCode::InvalidBuffer, "Mask count must match dimensions"});
    for (auto v : m.coverage)
        if (!detail::inRange(v, 0, 1))
            return std::unexpected(dd::invalid("Mask coverage must be finite and normalized"));
    return {};
}
bool point(MaskPoint p) {
    return detail::inRange(p.x, -2, 3) && detail::inRange(p.y, -2, 3);
}
Mask make(std::size_t w, std::size_t h) {
    return {w, h, std::vector<float>(w * h)};
}
dd::Plane crossGuided(const dd::Frame &f, const dd::Plane &guide, const dd::Plane &p, double sigma,
                      double epsilon) {
    const auto mi = dd::gaussian(f, guide, sigma), mp = dd::gaussian(f, p, sigma);
    auto ip = p, ii = guide;
    for (std::size_t i = 0; i < p.size(); ++i) {
        ip[i] *= guide[i];
        ii[i] *= guide[i];
    }
    ip = dd::gaussian(f, ip, sigma);
    ii = dd::gaussian(f, ii, sigma);
    auto a = p, b = p;
    for (std::size_t i = 0; i < p.size(); ++i) {
        a[i] = (ip[i] - mi[i] * mp[i]) / (std::max(0., ii[i] - mi[i] * mi[i]) + epsilon);
        b[i] = mp[i] - a[i] * mi[i];
    }
    a = dd::gaussian(f, a, sigma);
    b = dd::gaussian(f, b, sigma);
    for (std::size_t i = 0; i < p.size(); ++i)
        a[i] = detail::clamp(a[i] * guide[i] + b[i]);
    return a;
}
dd::Rgb selectionLab(dd::Rgb c) {
    for (auto &v : c)
        v /= 1 + v;
    return dd::lab(c);
}
bool valid(const LocalAdjustmentOptions &o) {
    if (!detail::inRange(o.exposure, -5, 5) || !detail::inRange(o.defringe, 0, 100) ||
        !detail::inRange(o.colorHue, 0, 360) || !detail::inRange(o.colorSaturation, 0, 100) ||
        !detail::inRange(o.amount, 0, 200))
        return false;
    for (auto v :
         {o.temperature, o.tint, o.contrast, o.highlights, o.shadows, o.whites, o.blacks, o.texture,
          o.clarity, o.dehaze, o.hue, o.saturation, o.sharpness, o.noise, o.moire})
        if (!detail::inRange(v, -100, 100))
            return false;
    return true;
}
} // namespace
Result<Mask> linearMask(std::size_t w, std::size_t h, const LinearMaskOptions &o) {
    if (auto c = detail::pixelCount(w, h); !c)
        return std::unexpected(c.error());
    if (!point(o.start) || !point(o.end) ||
        std::hypot(o.end.x - o.start.x, o.end.y - o.start.y) < 1e-9)
        return std::unexpected(
            dd::invalid("Linear mask endpoints must be separated by at least 1e-9"));
    return detail::allocated([&] {
        auto out = make(w, h);
        const auto ax = o.start.x * w, ay = o.start.y * h, dx = (o.end.x - o.start.x) * w,
                   dy = (o.end.y - o.start.y) * h, len = dx * dx + dy * dy;
        for (std::size_t y = 0; y < h; ++y)
            for (std::size_t x = 0; x < w; ++x)
                out.coverage[y * w + x] = static_cast<float>(
                    1 - dd::smooth(0, 1, ((x + .5 - ax) * dx + (y + .5 - ay) * dy) / len));
        return out;
    });
}
Result<Mask> radialMask(std::size_t w, std::size_t h, const RadialMaskOptions &o) {
    if (auto c = detail::pixelCount(w, h); !c)
        return std::unexpected(c.error());
    if (!point(o.center) || !detail::inRange(o.radiusX, 1e-6, 2) ||
        !detail::inRange(o.radiusY, 1e-6, 2) || !detail::inRange(o.angle, -180, 180) ||
        !detail::inRange(o.feather, 0, 100))
        return std::unexpected(dd::invalid("Invalid radial mask settings"));
    return detail::allocated([&] {
        auto out = make(w, h);
        const auto longEdge = double(std::max(w, h)), angle = -o.angle / 180 * std::numbers::pi,
                   co = std::cos(angle), si = std::sin(angle);
        for (std::size_t y = 0; y < h; ++y)
            for (std::size_t x = 0; x < w; ++x) {
                const auto dx = (x + .5 - o.center.x * w) / longEdge,
                           dy = (y + .5 - o.center.y * h) / longEdge,
                           r = std::hypot((dx * co - dy * si) / o.radiusX,
                                          (dx * si + dy * co) / o.radiusY);
                const auto a = 1 - dd::smooth(1 - std::max(.01, o.feather / 100), 1, r);
                out.coverage[y * w + x] = static_cast<float>(o.invert ? 1 - a : a);
            }
        return out;
    });
}
Result<Mask> brushMask(ImageView image, std::span<const BrushStroke> strokes) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    std::size_t points = 0;
    for (const auto &s : strokes) {
        if (!detail::inRange(s.radius, 1e-6, 2) || !detail::inRange(s.feather, 0, 100) ||
            !detail::inRange(s.flow, 0, 100) || !detail::inRange(s.density, 0, 100) ||
            s.points.size() > 4096 - points)
            return std::unexpected(dd::invalid("Invalid brush settings or too many points"));
        points += s.points.size();
        for (auto p : s.points)
            if (!point(p))
                return std::unexpected(dd::invalid("Invalid brush coordinates"));
    }
    auto result = detail::allocated([&]() -> Result<Mask> {
        auto out = make(image.width, image.height);
        auto f = dd::decode(image);
        const auto logs = dd::logs(f);
        const auto longEdge = double(std::max(image.width, image.height));
        std::size_t stamps = 0;
        for (const auto &s : strokes) {
            const auto r = s.radius * longEdge, hard = r * (1 - s.feather / 100),
                       spacing = std::max(.5, r * .25);
            dd::Plane paint(image.pixels.size());
            std::vector<MaskPoint> dabs;
            for (std::size_t i = 0; i < s.points.size(); ++i) {
                const MaskPoint b{s.points[i].x * image.width, s.points[i].y * image.height};
                if (i == 0)
                    dabs.push_back(b);
                else {
                    const MaskPoint a{s.points[i - 1].x * image.width,
                                      s.points[i - 1].y * image.height};
                    const auto steps = static_cast<std::size_t>(
                        std::max(1., std::ceil(std::hypot(b.x - a.x, b.y - a.y) / spacing)));
                    if (steps > 1000000 - dabs.size())
                        return std::unexpected(dd::invalid("Brush stroke exceeds stamp limit"));
                    for (std::size_t j = 1; j <= steps; ++j)
                        dabs.push_back(
                            {a.x + (b.x - a.x) * j / steps, a.y + (b.y - a.y) * j / steps});
                }
            }
            if (dabs.size() > 1000000 - stamps)
                return std::unexpected(dd::invalid("Brush strokes exceed stamp limit"));
            stamps += dabs.size();
            for (auto d : dabs) {
                if (d.x + r < 0 || d.y + r < 0 || d.x - r >= image.width || d.y - r >= image.height)
                    continue;
                const auto x0 = static_cast<std::size_t>(
                               std::clamp(std::floor(d.x - r), 0., double(image.width))),
                           x1 = static_cast<std::size_t>(
                               std::clamp(std::ceil(d.x + r), 0., double(image.width))),
                           y0 = static_cast<std::size_t>(
                               std::clamp(std::floor(d.y - r), 0., double(image.height))),
                           y1 = static_cast<std::size_t>(
                               std::clamp(std::ceil(d.y + r), 0., double(image.height)));
                const auto anchor =
                    static_cast<std::size_t>(std::clamp(d.y, 0., double(image.height - 1))) *
                        image.width +
                    static_cast<std::size_t>(std::clamp(d.x, 0., double(image.width - 1)));
                const auto reference =
                    dd::scale(f.rgb[anchor], 1 / std::max(1e-6, dd::luminance(f.rgb[anchor])));
                for (std::size_t y = y0; y < y1; ++y)
                    for (std::size_t x = x0; x < x1; ++x) {
                        const auto distance = std::hypot(x + .5 - d.x, y + .5 - d.y);
                        if (distance > r)
                            continue;
                        const auto i = y * image.width + x;
                        double a = distance <= hard ? 1 : 1 - dd::smooth(hard, r, distance);
                        if (s.autoMask) {
                            if (f.alpha[anchor] == 0 || f.alpha[i] == 0)
                                a = 0;
                            else {
                                const auto color = dd::scale(
                                    f.rgb[i], 1 / std::max(1e-6, dd::luminance(f.rgb[i])));
                                const auto dl = (logs[i] - logs[anchor]) / .5;
                                const auto dc = std::sqrt(std::pow(color[0] - reference[0], 2) +
                                                          std::pow(color[1] - reference[1], 2) +
                                                          std::pow(color[2] - reference[2], 2)) /
                                                .25;
                                a *= 1 - dd::smooth(.5, 1, std::hypot(dl, dc));
                            }
                        }
                        paint[i] =
                            std::min(s.density / 100, paint[i] + a * s.flow / 100 * (1 - paint[i]));
                    }
            }
            if (s.autoMask) {
                const auto refined = crossGuided(f, logs, paint, std::max(1., r * .25), .02);
                for (std::size_t i = 0; i < paint.size(); ++i)
                    paint[i] = std::max(paint[i], refined[i]);
            }
            for (std::size_t i = 0; i < paint.size(); ++i)
                out.coverage[i] =
                    static_cast<float>(s.erase ? out.coverage[i] * (1 - paint[i])
                                               : std::max(double(out.coverage[i]), paint[i]));
        }
        return out;
    });
    if (!result)
        return std::unexpected(result.error());
    return std::move(*result);
}
Result<Mask> luminanceRangeMask(ImageView image, const LuminanceRangeOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (!detail::inRange(o.low, 0, 1) || !detail::inRange(o.high, 0, 1) || o.low > o.high ||
        !detail::inRange(o.lowFeather, 0, 1) || !detail::inRange(o.highFeather, 0, 1) ||
        !detail::inRange(o.exposure, -5, 5))
        return std::unexpected(dd::invalid("Invalid luminance range"));
    return detail::allocated([&] {
        auto out = make(image.width, image.height);
        for (std::size_t i = 0; i < image.pixels.size(); ++i) {
            const auto p = image.pixels[i];
            const auto y = detail::clamp(
                (dd::logLum({detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)}) +
                 o.exposure + 8) /
                12);
            out.coverage[i] = static_cast<float>(
                dd::smooth(o.low - std::max(.001, o.lowFeather), o.low, y) *
                (1 - dd::smooth(o.high, o.high + std::max(.001, o.highFeather), y)));
        }
        return out;
    });
}
Result<Mask> colorRangeMask(ImageView image, const ColorRangeOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (o.samples.empty() || o.samples.size() > 8 || !detail::inRange(o.range, 0, 100) ||
        !detail::inRange(o.exposure, -5, 5))
        return std::unexpected(dd::invalid("Invalid color range"));
    std::array<dd::Rgb, 8> samples{};
    for (std::size_t i = 0; i < o.samples.size(); ++i) {
        const auto p = o.samples[i];
        if (!detail::inRange(p.r, 0, 1) || !detail::inRange(p.g, 0, 1) ||
            !detail::inRange(p.b, 0, 1) || !detail::inRange(p.a, 0, 1) || p.a == 0)
            return std::unexpected(
                dd::invalid("Color range samples must be visible normalized colors"));
        samples[i] = selectionLab({detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)});
    }
    return detail::allocated([&] {
        auto out = make(image.width, image.height);
        const auto tolerance = .04 + .16 * o.range / 100;
        for (std::size_t i = 0; i < image.pixels.size(); ++i) {
            const auto p = image.pixels[i];
            const auto lab = selectionLab(
                dd::scale({detail::linear(p.r), detail::linear(p.g), detail::linear(p.b)},
                          std::exp2(o.exposure)));
            double nearest = 1e9;
            for (std::size_t j = 0; j < o.samples.size(); ++j)
                nearest = std::min(nearest, std::sqrt(std::pow(lab[1] - samples[j][1], 2) +
                                                      std::pow(lab[2] - samples[j][2], 2) +
                                                      .25 * std::pow(lab[0] - samples[j][0], 2)));
            out.coverage[i] =
                static_cast<float>(1 - dd::smooth(tolerance * .5, tolerance, nearest));
        }
        return out;
    });
}
Result<Mask> combineMasks(MaskView a, MaskView b, MaskCombine operation) {
    if (auto c = validate(a); !c)
        return std::unexpected(c.error());
    if (auto c = validate(b); !c)
        return std::unexpected(c.error());
    if (a.width != b.width || a.height != b.height)
        return std::unexpected(Error{ErrorCode::SizeMismatch, "Mask dimensions must match"});
    if (operation != MaskCombine::Add && operation != MaskCombine::Subtract &&
        operation != MaskCombine::Intersect)
        return std::unexpected(dd::invalid("Unknown mask combination"));
    return detail::allocated([&] {
        auto out = make(a.width, a.height);
        for (std::size_t i = 0; i < a.coverage.size(); ++i)
            out.coverage[i] = operation == MaskCombine::Add ? std::max(a.coverage[i], b.coverage[i])
                              : operation == MaskCombine::Subtract
                                  ? a.coverage[i] * (1 - b.coverage[i])
                                  : a.coverage[i] * b.coverage[i];
        return out;
    });
}
Result<Mask> invertMask(MaskView m) {
    if (auto c = validate(m); !c)
        return std::unexpected(c.error());
    return detail::allocated([&] {
        auto out = make(m.width, m.height);
        for (std::size_t i = 0; i < m.coverage.size(); ++i)
            out.coverage[i] = 1 - m.coverage[i];
        return out;
    });
}
Result<Mask> refineMask(ImageView image, MaskView m, double amount) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (auto c = validate(m); !c)
        return std::unexpected(c.error());
    if (image.width != m.width || image.height != m.height)
        return std::unexpected(
            Error{ErrorCode::SizeMismatch, "Mask and image dimensions must match"});
    if (!detail::inRange(amount, 0, 100))
        return std::unexpected(dd::invalid("Invalid mask refinement amount"));
    return detail::allocated([&] {
        Mask out{m.width, m.height, {m.coverage.begin(), m.coverage.end()}};
        if (amount == 0)
            return out;
        auto f = dd::decode(image);
        const dd::Plane p{m.coverage.begin(), m.coverage.end()};
        const auto refined = crossGuided(
            f, dd::logs(f), p, std::max(1., .04 * std::max(f.width, f.height) * amount / 100), .02);
        for (std::size_t i = 0; i < p.size(); ++i)
            out.coverage[i] = static_cast<float>(
                detail::clamp(p[i] + (refined[i] - p[i]) * std::sqrt(amount / 100)));
        return out;
    });
}
Result<Image> applyLocalAdjustments(ImageView image, std::span<const LocalAdjustment> adjustments) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (adjustments.size() > 32)
        return std::unexpected(dd::invalid("At most 32 local masks are supported"));
    bool tone = false, clarity = false, fine = false, dehaze = false, chroma = false,
         active = false;
    for (const auto &a : adjustments) {
        if (auto c = validate(a.mask); !c)
            return std::unexpected(c.error());
        if (a.mask.width != image.width || a.mask.height != image.height)
            return std::unexpected(
                Error{ErrorCode::SizeMismatch, "Local masks must match image dimensions"});
        if (!valid(a.options))
            return std::unexpected(dd::invalid("Invalid local adjustment settings"));
        const auto &o = a.options;
        if (o.amount == 0)
            continue;
        tone = tone || o.highlights != 0 || o.shadows != 0;
        clarity = clarity || o.clarity != 0;
        fine = fine || o.texture != 0 || o.sharpness != 0 || o.noise != 0 || o.defringe != 0;
        dehaze = dehaze || o.dehaze != 0;
        chroma = chroma || o.moire != 0 || o.noise > 0;
        active = active || o.exposure != 0 || o.temperature != 0 || o.tint != 0 ||
                 o.contrast != 0 || o.highlights != 0 || o.shadows != 0 || o.whites != 0 ||
                 o.blacks != 0 || o.texture != 0 || o.clarity != 0 || o.dehaze != 0 || o.hue != 0 ||
                 o.saturation != 0 || o.sharpness != 0 || o.noise != 0 || o.moire != 0 ||
                 o.defringe != 0 || o.colorSaturation != 0;
    }
    return detail::allocated([&] {
        if (!active)
            return detail::copy(image);
        auto f = dd::decode(image);
        const auto logs = dd::logs(f);
        const auto longEdge = double(std::max(f.width, f.height));
        const auto base = tone ? dd::guided(f, logs, std::max(1., .015 * longEdge), .35) : logs;
        dd::Plane clar, tex, dark;
        std::array<dd::Plane, 3> blurredChroma;
        double air = 1;
        if (clarity)
            clar = dd::guided(f, logs, std::max(1., .012 * longEdge), .8);
        if (fine)
            tex = dd::gaussian(f, logs, std::max(.6, .0018 * longEdge));
        if (dehaze) {
            for (auto c : f.rgb)
                dark.push_back(std::min({c[0], c[1], c[2]}));
            dark = dd::gaussian(f, dark, std::max(1., .02 * longEdge));
            dd::Plane samples;
            for (std::size_t i = 0; i < dark.size(); ++i)
                if (f.alpha[i] > 0)
                    samples.push_back(dark[i]);
            if (!samples.empty()) {
                const auto at =
                    std::min(samples.size() - 1, static_cast<std::size_t>(samples.size() * .995));
                std::nth_element(samples.begin(), samples.begin() + at, samples.end());
                air = std::max(.05, samples[at]);
            }
        }
        if (chroma) {
            for (auto c : f.rgb) {
                const auto y = std::max(1e-6, dd::luminance(c));
                for (int k = 0; k < 3; ++k)
                    blurredChroma[k].push_back(c[k] / y);
            }
            for (auto &p : blurredChroma)
                p = dd::gaussian(f, p, std::max(1., .004 * longEdge));
        }
        auto out = detail::copy(image);
        for (std::size_t i = 0; i < f.rgb.size(); ++i)
            if (f.alpha[i] > 0) {
                std::array<double, 17> t{};
                double colorHue = 0, colorAmount = 0;
                for (const auto &a : adjustments) {
                    const auto &o = a.options;
                    const auto weight = a.mask.coverage[i] * o.amount / 100;
                    if (weight == 0)
                        continue;
                    const std::array<double, 17> terms{
                        o.exposure,         o.temperature / 100, o.tint / 100,   o.contrast / 100,
                        o.highlights / 100, o.shadows / 100,     o.whites / 100, o.blacks / 100,
                        o.texture / 100,    o.clarity / 100,     o.dehaze / 100, o.saturation / 100,
                        o.hue / 100 * .6,   o.sharpness / 100,   o.noise / 100,  o.moire / 100,
                        o.defringe / 100};
                    for (int k = 0; k < 17; ++k)
                        t[k] += weight * terms[k];
                    if (o.colorSaturation > 0) {
                        colorHue = o.colorHue;
                        colorAmount = weight * o.colorSaturation / 100;
                    }
                }
                if (std::all_of(t.begin(), t.end(), [](auto v) { return v == 0; }) &&
                    colorAmount == 0)
                    continue;
                auto c = f.rgb[i];
                const auto mt = std::clamp(t[15] + .5 * std::max(0., t[14]), -1., 1.);
                if (mt != 0) {
                    const auto y = dd::luminance(c);
                    for (int k = 0; k < 3; ++k) {
                        const auto ch = c[k] / std::max(1e-6, y);
                        c[k] = std::max(0., (ch + (blurredChroma[k][i] - ch) * mt) * y);
                    }
                }
                if (t[16] > 0) {
                    const auto det = logs[i] - tex[i], y = std::max(1e-6, dd::luminance(c)),
                               purple = (std::min(c[0], c[2]) - c[1]) / y,
                               green = (c[1] - std::max(c[0], c[2])) / y,
                               k = std::min(1., t[16]) * dd::smooth(.04, .3, std::abs(det)) *
                                   std::max(dd::smooth(.02, .2, purple),
                                            dd::smooth(.02, .2, green));
                    for (auto &v : c)
                        v += (y - v) * k;
                }
                if (t[10] != 0) {
                    const auto d = detail::clamp(dark[i] / air);
                    if (t[10] > 0) {
                        const auto transmission = std::max(.12, 1 - .95 * std::min(1., t[10]) * d);
                        for (auto &v : c)
                            v = std::max(0., (v - air * (1 - transmission)) / transmission);
                    } else {
                        const auto k = std::min(1., -t[10]) * .7 * (.35 + .65 * d);
                        for (auto &v : c)
                            v += (air * .9 - v) * k;
                    }
                }
                c = dd::scale(c, std::exp2(t[0]));
                if (t[1] != 0 || t[2] != 0) {
                    const auto y = dd::luminance(c);
                    c[0] *= std::max(0., 1 + .3 * t[1]);
                    c[1] *= std::max(0., 1 - .22 * t[2]);
                    c[2] *= std::max(0., 1 - .3 * t[1]);
                    const auto after = dd::luminance(c);
                    if (after > 0)
                        c = dd::scale(c, y / after);
                }
                const auto current = dd::logLum(c), b = base[i] + current - logs[i],
                           ws = 1 - dd::smooth(-4.8, .3, b), wh = dd::smooth(-1, 2.8, b);
                double delta = t[5] * 1.7 * ws * std::sqrt(ws) + t[4] * 1.7 * wh +
                               t[6] * .8 * dd::smooth(.5, 3, current) +
                               t[7] * .8 * (1 - dd::smooth(-6, -1.5, current)) +
                               t[3] * .14 * std::clamp(current, -6., 4.);
                if (t[9] != 0)
                    delta += t[9] * .85 * std::clamp(logs[i] - clar[i], -2.5, 2.5) *
                             (.35 + .65 * std::exp(-std::pow(b / 3.2, 2)));
                if (t[8] != 0 || t[13] != 0 || t[14] != 0) {
                    const auto det = logs[i] - tex[i];
                    delta += t[8] * 1.1 * std::clamp(det, -1., 1.) *
                                 (1 - .6 * dd::smooth(.4, 1.6, std::abs(det))) +
                             t[13] * .6 * 1.3 * std::clamp(det, -.8, .8) -
                             std::clamp(t[14], -1., 1.) * .9 * det *
                                 (1 - dd::smooth(.1, .5, std::abs(det)));
                }
                c = dd::scale(c, std::exp2(delta));
                auto lab = dd::lab(c);
                const auto ch = std::hypot(lab[1], lab[2]) * std::max(0., 1 + t[11]),
                           h = std::atan2(lab[2], lab[1]) + t[12];
                lab[1] = ch * std::cos(h);
                lab[2] = ch * std::sin(h);
                if (colorAmount > 0) {
                    const auto hp = colorHue / 60, x = 1 - std::abs(std::fmod(hp, 2) - 1);
                    dd::Rgb hue{};
                    switch (static_cast<int>(hp) % 6) {
                    case 0:
                        hue = {1, x, 0};
                        break;
                    case 1:
                        hue = {x, 1, 0};
                        break;
                    case 2:
                        hue = {0, 1, x};
                        break;
                    case 3:
                        hue = {0, x, 1};
                        break;
                    case 4:
                        hue = {x, 0, 1};
                        break;
                    default:
                        hue = {1, 0, x};
                    }
                    for (auto &v : hue)
                        v = detail::linear(v);
                    const auto direction = dd::lab(hue);
                    const auto angle = std::atan2(direction[2], direction[1]);
                    lab[1] += .08 * colorAmount * std::cos(angle);
                    lab[2] += .08 * colorAmount * std::sin(angle);
                }
                const auto mapped = dd::gamut(dd::fromLab(lab));
                out.pixels[i] = {detail::encoded(mapped[0]), detail::encoded(mapped[1]),
                                 detail::encoded(mapped[2]), image.pixels[i].a};
            }
        return out;
    });
}
} // namespace iiGraphicAdjust
