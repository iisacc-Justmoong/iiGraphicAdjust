// Adapted from Photocraft camera_raw.rs; see third_party/photocraft/LICENSE-MIT.
#include "FilterInternal.h"
#include <iiGraphicAdjust/CameraRawFilter.h>
#include <numeric>

namespace iiGraphicAdjust {
namespace {
using Scalar = std::vector<double>;
using Rgb = std::array<double, 3>;
using Pixels = std::vector<Rgb>;
constexpr std::array<double, 8> bandCenters{0, 30, 60, 120, 180, 240, 270, 300};
double luma(Rgb p) {
    return .2126 * p[0] + .7152 * p[1] + .0722 * p[2];
}
double smooth(double a, double b, double x) {
    return filter_detail::smooth(a, b, x);
}
double encodeExtended(double v) {
    return v <= .0031308 ? v * 12.92 : 1.055 * std::pow(v, 1 / 2.4) - .055;
}
double wrapHue(double h) {
    auto v = std::fmod(h, 360.);
    return v < 0 ? v + 360 : v;
}

bool linearCurve(const std::vector<std::array<double, 2>> &points) {
    return points.empty() || std::all_of(points.begin(), points.end(),
                                         [](auto p) { return std::abs(p[0] - p[1]) < 1e-3; });
}
bool curveNeutral(const CameraRawOptions &p) {
    return p.curveHighlights == 0 && p.curveLights == 0 && p.curveDarks == 0 &&
           p.curveShadows == 0 && linearCurve(p.pointCurve) && linearCurve(p.pointCurveRed) &&
           linearCurve(p.pointCurveGreen) && linearCurve(p.pointCurveBlue);
}
bool toneNeutral(const CameraRawOptions &p) {
    return p.contrast == 0 && p.highlights == 0 && p.shadows == 0 && p.whites == 0 &&
           p.blacks == 0 && p.texture == 0 && p.clarity == 0;
}
bool hslNeutral(const CameraRawOptions &p) {
    for (std::size_t i = 0; i < 8; ++i)
        if (p.hslHue[i] != 0 || p.hslSaturation[i] != 0 || p.hslLuminance[i] != 0)
            return false;
    return true;
}
bool gradeNeutral(const CameraRawOptions &p) {
    for (auto wheel : {p.gradeShadows, p.gradeMidtones, p.gradeHighlights, p.gradeGlobal})
        if (wheel.saturation != 0 || wheel.luminance != 0)
            return false;
    return true;
}
bool identity(const CameraRawOptions &p) {
    return p.temperature == 0 && p.tint == 0 && p.exposure == 0 && toneNeutral(p) &&
           p.dehaze == 0 && p.vibrance == 0 && p.saturation == 0 && curveNeutral(p) &&
           hslNeutral(p) && gradeNeutral(p) && p.sharpenAmount == 0 && p.noiseLuminance == 0 &&
           p.noiseColor == 0 && p.grainAmount == 0 && p.vignetteAmount == 0;
}
bool validOptions(const CameraRawOptions &p) {
    for (double v :
         {p.temperature, p.tint, p.contrast, p.highlights, p.shadows, p.whites, p.blacks, p.texture,
          p.clarity, p.dehaze, p.vibrance, p.saturation, p.curveHighlights, p.curveLights,
          p.curveDarks, p.curveShadows, p.gradeBalance, p.vignetteAmount, p.vignetteRoundness})
        if (!detail::inRange(v, -100, 100))
            return false;
    for (double v :
         {p.gradeBlending, p.sharpenDetail, p.sharpenMasking, p.noiseLuminance,
          p.noiseLuminanceDetail, p.noiseColor, p.noiseColorDetail, p.grainAmount, p.grainSize,
          p.grainRoughness, p.vignetteMidpoint, p.vignetteFeather, p.vignetteHighlights})
        if (!detail::inRange(v, 0, 100))
            return false;
    if (!detail::inRange(p.exposure, -5, 5) || !detail::inRange(p.sharpenAmount, 0, 150) ||
        !detail::inRange(p.sharpenRadius, .5, 3) || !detail::inRange(p.pixelScale, .01, 1))
        return false;
    for (std::size_t i = 0; i < 3; ++i)
        if (!detail::inRange(p.curveSplits[i], 5, 95) ||
            (i && p.curveSplits[i] <= p.curveSplits[i - 1]))
            return false;
    for (auto points : {&p.pointCurve, &p.pointCurveRed, &p.pointCurveGreen, &p.pointCurveBlue}) {
        if (points->size() == 1 || points->size() > 16)
            return false;
        for (std::size_t i = 0; i < points->size(); ++i)
            if (!detail::inRange((*points)[i][0], 0, 255) ||
                !detail::inRange((*points)[i][1], 0, 255) ||
                (i && (*points)[i][0] - (*points)[i - 1][0] < 1))
                return false;
    }
    for (std::size_t i = 0; i < 8; ++i)
        if (!detail::inRange(p.hslHue[i], -100, 100) ||
            !detail::inRange(p.hslSaturation[i], -100, 100) ||
            !detail::inRange(p.hslLuminance[i], -100, 100))
            return false;
    for (auto wheel : {p.gradeShadows, p.gradeMidtones, p.gradeHighlights, p.gradeGlobal})
        if (!detail::inRange(wheel.hue, 0, 360) || !detail::inRange(wheel.saturation, 0, 100) ||
            !detail::inRange(wheel.luminance, -100, 100))
            return false;
    return p.vignetteStyle == CameraRawVignetteStyle::HighlightPriority ||
           p.vignetteStyle == CameraRawVignetteStyle::ColorPriority ||
           p.vignetteStyle == CameraRawVignetteStyle::PaintOverlay;
}

// Edge-repeated running means preserve the source's box-guided and three-box algorithms.
Scalar box(const Scalar &v, std::size_t w, std::size_t h, std::size_t r) {
    if (!r)
        return v;
    Scalar out = v, next(v.size());
    const double norm = 1. / (2. * r + 1);
    for (bool horizontal : {true, false}) {
        const auto length = horizontal ? w : h, lines = horizontal ? h : w;
        auto at = [&](std::size_t line, std::size_t position) {
            return horizontal ? line * w + position : position * w + line;
        };
        for (std::size_t line = 0; line < lines; ++line) {
            double sum = out[at(line, 0)] * (r + 1.);
            for (std::size_t j = 1; j <= std::min(r, length - 1); ++j)
                sum += out[at(line, j)];
            if (r >= length)
                sum += out[at(line, length - 1)] * (r - length + 1.);
            for (std::size_t j = 0; j < length; ++j) {
                next[at(line, j)] = sum * norm;
                const auto add = j + r + 1 >= length ? length - 1 : j + r + 1;
                const auto sub = j >= r ? j - r : 0;
                sum += out[at(line, add)] - out[at(line, sub)];
            }
        }
        out.swap(next);
    }
    return out;
}
Scalar weightedBox(const Scalar &v, const Scalar &alpha, std::size_t w, std::size_t h,
                   std::size_t r) {
    Scalar numerator(v.size());
    for (std::size_t i = 0; i < v.size(); ++i)
        numerator[i] = v[i] * alpha[i];
    numerator = box(numerator, w, h, r);
    const auto denominator = box(alpha, w, h, r);
    for (std::size_t i = 0; i < v.size(); ++i)
        numerator[i] = denominator[i] > 0 ? numerator[i] / denominator[i] : 0;
    return numerator;
}
Scalar gaussian(const Scalar &v, const Scalar &alpha, std::size_t w, std::size_t h, double sigma) {
    if (sigma < .3)
        return v;
    int low = std::max(1, int(std::floor(std::sqrt(4 * sigma * sigma + 1))));
    if (low % 2 == 0)
        --low;
    const int m = std::clamp(
        int(std::round((12 * sigma * sigma - 3. * low * low - 12. * low - 9) / (-4. * low - 4))), 0,
        3);
    std::array<std::size_t, 3> radii{};
    for (int i = 0; i < 3; ++i)
        radii[i] = std::size_t((low + (i < m ? 0 : 2) - 1) / 2);
    if (radii[0] == 0 && radii[1] == 0 && radii[2] == 0)
        radii[0] = 1; // Photocraft #706, small-sigma real blur.
    Scalar numerator(v.size()), denominator = alpha;
    for (std::size_t i = 0; i < v.size(); ++i)
        numerator[i] = v[i] * alpha[i];
    for (auto r : radii) {
        numerator = box(numerator, w, h, r);
        denominator = box(denominator, w, h, r);
    }
    for (std::size_t i = 0; i < v.size(); ++i)
        numerator[i] = denominator[i] > 0 ? numerator[i] / denominator[i] : 0;
    return numerator;
}
std::pair<Scalar, Scalar> coefficients(const Scalar &guide, const Scalar &input,
                                       const Scalar &alpha, std::size_t w, std::size_t h,
                                       std::size_t r, double epsilon) {
    const auto mi = weightedBox(guide, alpha, w, h, r), mp = weightedBox(input, alpha, w, h, r);
    Scalar ip(input.size()), ii(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        ip[i] = guide[i] * input[i];
        ii[i] = guide[i] * guide[i];
    }
    const auto mip = weightedBox(ip, alpha, w, h, r), mii = weightedBox(ii, alpha, w, h, r);
    Scalar a(input.size()), b(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        a[i] = (mip[i] - mi[i] * mp[i]) / (std::max(0., mii[i] - mi[i] * mi[i]) + epsilon);
        b[i] = mp[i] - a[i] * mi[i];
    }
    return {weightedBox(a, alpha, w, h, r), weightedBox(b, alpha, w, h, r)};
}
double bilinear(const Scalar &v, std::size_t w, std::size_t h, double x, double y) {
    x = std::clamp(x, 0., double(w - 1));
    y = std::clamp(y, 0., double(h - 1));
    const auto x0 = std::size_t(std::floor(x)), y0 = std::size_t(std::floor(y));
    const auto x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
    const auto tx = x - x0, ty = y - y0;
    auto top = v[y0 * w + x0] + (v[y0 * w + x1] - v[y0 * w + x0]) * tx;
    auto bottom = v[y1 * w + x0] + (v[y1 * w + x1] - v[y1 * w + x0]) * tx;
    return top + (bottom - top) * ty;
}
Scalar guided(const Scalar &guide, const Scalar &input, const Scalar &alpha, std::size_t w,
              std::size_t h, std::size_t r, double epsilon, bool fast = false) {
    const auto step = fast ? std::clamp<std::size_t>(r / 8, 1, 8) : 1;
    if (step == 1) {
        auto [a, b] = coefficients(guide, input, alpha, w, h, r, epsilon);
        for (std::size_t i = 0; i < input.size(); ++i)
            a[i] = a[i] * guide[i] + b[i];
        return a;
    }
    const auto sw = (w + step - 1) / step, sh = (h + step - 1) / step;
    Scalar si(sw * sh), sp(sw * sh), sa(sw * sh), count(sw * sh);
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x) {
            const auto j = (y / step) * sw + x / step, i = y * w + x;
            si[j] += guide[i] * alpha[i];
            sp[j] += input[i] * alpha[i];
            sa[j] += alpha[i];
            count[j] += 1;
        }
    for (std::size_t i = 0; i < si.size(); ++i) {
        si[i] = sa[i] > 0 ? si[i] / sa[i] : 0;
        sp[i] = sa[i] > 0 ? sp[i] / sa[i] : 0;
        sa[i] /= count[i];
    }
    auto [a, b] = coefficients(si, sp, sa, sw, sh, std::max<std::size_t>(1, r / step), epsilon);
    Scalar output(input.size());
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x) {
            const auto fx = (x + .5) / step - .5, fy = (y + .5) / step - .5;
            output[y * w + x] =
                bilinear(a, sw, sh, fx, fy) * guide[y * w + x] + bilinear(b, sw, sh, fx, fy);
        }
    return output;
}

Scalar pointLut(const std::vector<std::array<double, 2>> &curve) {
    constexpr std::size_t n = 1024;
    Scalar out(n);
    if (linearCurve(curve)) {
        for (std::size_t i = 0; i < n; ++i)
            out[i] = double(i) / (n - 1);
        return out;
    }
    auto p = curve;
    for (auto &v : p) {
        v[0] /= 255;
        v[1] /= 255;
    }
    Scalar d(p.size() - 1), t(p.size());
    for (std::size_t i = 0; i < d.size(); ++i)
        d[i] = (p[i + 1][1] - p[i][1]) / (p[i + 1][0] - p[i][0]);
    t.front() = d.front();
    t.back() = d.back();
    for (std::size_t i = 1; i + 1 < p.size(); ++i)
        t[i] = d[i - 1] * d[i] <= 0 ? 0 : (d[i - 1] + d[i]) / 2;
    for (std::size_t i = 0; i < d.size(); ++i) {
        if (d[i] == 0) {
            t[i] = 0;
            t[i + 1] = 0;
            continue;
        }
        const auto a = t[i] / d[i], b = t[i + 1] / d[i], s = a * a + b * b;
        if (s > 9) {
            const auto tau = 3 / std::sqrt(s);
            t[i] = tau * a * d[i];
            t[i + 1] = tau * b * d[i];
        }
    }
    for (std::size_t i = 0; i < n; ++i) {
        const auto x = double(i) / (n - 1);
        if (x <= p.front()[0]) {
            out[i] = p.front()[1];
            continue;
        }
        if (x >= p.back()[0]) {
            out[i] = p.back()[1];
            continue;
        }
        std::size_t k = 0;
        while (x > p[k + 1][0])
            ++k;
        const auto h = p[k + 1][0] - p[k][0], s = (x - p[k][0]) / h;
        const auto h00 = 2 * s * s * s - 3 * s * s + 1, h10 = s * s * s - 2 * s * s + s;
        const auto h01 = -2 * s * s * s + 3 * s * s, h11 = s * s * s - s * s;
        out[i] =
            detail::clamp(h00 * p[k][1] + h10 * h * t[k] + h01 * p[k + 1][1] + h11 * h * t[k + 1]);
    }
    return out;
}
Scalar parametricLut(const CameraRawOptions &p) {
    const auto s = p.curveSplits;
    const std::array<double, 4> center{s[0] / 200, (s[0] + s[1]) / 200, (s[1] + s[2]) / 200,
                                       (s[2] + 100) / 200};
    const std::array<double, 4> width{std::max(.1, s[0] / 100), std::max(.1, (s[1] - s[0]) / 100),
                                      std::max(.1, (s[2] - s[1]) / 100),
                                      std::max(.1, (100 - s[2]) / 100)};
    const std::array<double, 4> amount{p.curveShadows, p.curveDarks, p.curveLights,
                                       p.curveHighlights};
    Scalar lut(1024);
    for (std::size_t i = 0; i < lut.size(); ++i) {
        const auto x = double(i) / (lut.size() - 1);
        auto y = x;
        for (std::size_t k = 0; k < 4; ++k) {
            const auto d = (x - center[k]) / (width[k] * 1.2);
            y += amount[k] / 100 * .25 * std::exp(-d * d * 2) * x * (1 - x) * 4;
        }
        lut[i] = detail::clamp(y);
        if (i)
            lut[i] = std::max(lut[i], lut[i - 1]);
    }
    return lut;
}
double lutValue(const Scalar &lut, double v) {
    if (v < 0)
        return v + lut.front();
    if (v > 1)
        return lut.back() + v - 1;
    const auto x = v * (lut.size() - 1);
    const auto i = std::min(std::size_t(x), lut.size() - 2);
    const auto t = x - i;
    return lut[i] + (lut[i + 1] - lut[i]) * t;
}
Rgb hsv(Rgb c) {
    const auto maximum = *std::max_element(c.begin(), c.end()),
               minimum = *std::min_element(c.begin(), c.end()), d = maximum - minimum;
    double h = 0;
    if (d > 1e-9) {
        if (maximum == c[0])
            h = wrapHue(60 * (c[1] - c[2]) / d);
        else if (maximum == c[1])
            h = 60 * ((c[2] - c[0]) / d + 2);
        else
            h = 60 * ((c[0] - c[1]) / d + 4);
    }
    return {h, maximum > 1e-9 ? d / maximum : 0, maximum};
}
Rgb rgb(double hue, double saturation, double value) {
    const auto h = wrapHue(hue) / 60, c = value * saturation,
               x = c * (1 - std::abs(std::fmod(h, 2.) - 1)), m = value - c;
    Rgb out;
    switch (int(h)) {
    case 0:
        out = {c, x, 0};
        break;
    case 1:
        out = {x, c, 0};
        break;
    case 2:
        out = {0, c, x};
        break;
    case 3:
        out = {0, x, c};
        break;
    case 4:
        out = {x, 0, c};
        break;
    default:
        out = {c, 0, x};
        break;
    }
    for (auto &v : out)
        v += m;
    return out;
}
std::array<double, 8> bands(double hue) {
    std::array<double, 8> weights{};
    for (std::size_t i = 0; i < 8; ++i) {
        const auto d = wrapHue(hue - bandCenters[i] + 180) - 180;
        const auto span = d >= 0 ? wrapHue(bandCenters[(i + 1) % 8] - bandCenters[i])
                                 : wrapHue(bandCenters[i] - bandCenters[(i + 7) % 8]);
        weights[i] = std::max(0., 1 - std::abs(d) / span);
    }
    return weights;
}

void haze(Pixels &pixels, const Scalar &alpha, std::size_t w, std::size_t h, double amount) {
    const auto step = (std::max(w, h) + 767) / 768, sw = (w + step - 1) / step,
               sh = (h + step - 1) / step;
    Pixels small(sw * sh);
    Scalar sa(sw * sh), count(sw * sh);
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x) {
            const auto j = (y / step) * sw + x / step, i = y * w + x;
            for (std::size_t c = 0; c < 3; ++c)
                small[j][c] += detail::clamp(pixels[i][c]) * alpha[i];
            sa[j] += alpha[i];
            count[j] += 1;
        }
    for (std::size_t i = 0; i < small.size(); ++i) {
        for (auto &c : small[i])
            c = sa[i] > 0 ? c / sa[i] : 0;
        sa[i] /= count[i];
    }
    const auto radius = std::max<std::size_t>(2, std::max(sw, sh) / 60);
    auto minimum = [&](const Scalar &value) {
        Scalar a(value.size()), b(value.size());
        for (std::size_t y = 0; y < sh; ++y)
            for (std::size_t x = 0; x < sw; ++x) {
                double v = std::numeric_limits<double>::infinity();
                for (auto j = x > radius ? x - radius : 0; j <= std::min(x + radius, sw - 1); ++j)
                    v = std::min(v, value[y * sw + j]);
                a[y * sw + x] = v;
            }
        for (std::size_t y = 0; y < sh; ++y)
            for (std::size_t x = 0; x < sw; ++x) {
                double v = std::numeric_limits<double>::infinity();
                for (auto j = y > radius ? y - radius : 0; j <= std::min(y + radius, sh - 1); ++j)
                    v = std::min(v, a[j * sw + x]);
                b[y * sw + x] = std::isfinite(v) ? v : 0;
            }
        return b;
    };
    Scalar raw(small.size());
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < small.size(); ++i) {
        raw[i] = sa[i] > 0 ? *std::min_element(small[i].begin(), small[i].end())
                           : std::numeric_limits<double>::infinity();
        if (sa[i] > 0)
            indices.push_back(i);
    }
    if (indices.empty())
        return;
    const auto dark = minimum(raw);
    std::stable_sort(indices.begin(), indices.end(),
                     [&](auto a, auto b) { return dark[a] > dark[b]; });
    const auto top = std::max<std::size_t>(1, indices.size() / 1000);
    Rgb air{};
    double total = 0;
    for (std::size_t i = 0; i < top; ++i) {
        auto j = indices[i];
        total += sa[j];
        for (std::size_t c = 0; c < 3; ++c)
            air[c] += small[j][c] * sa[j];
    }
    for (auto &c : air)
        c = std::max(.2, c / total);
    Scalar guide(small.size());
    for (std::size_t i = 0; i < small.size(); ++i) {
        raw[i] = sa[i] > 0
                     ? std::min({small[i][0] / air[0], small[i][1] / air[1], small[i][2] / air[2]})
                     : std::numeric_limits<double>::infinity();
        guide[i] = luma(small[i]);
    }
    raw = minimum(raw);
    for (auto &v : raw)
        v = 1 - .95 * v;
    const auto transmission = guided(guide, raw, sa, sw, sh, radius * 2, 1e-3);
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x) {
            const auto t = std::clamp(
                bilinear(transmission, sw, sh, (x + .5) / step - .5, (y + .5) / step - .5), .1, 1.);
            for (std::size_t c = 0; c < 3; ++c) {
                auto &value = pixels[y * w + x][c];
                if (amount >= 0)
                    value += ((value - air[c]) / t + air[c] - value) * amount;
                else
                    value += (air[c] - value) * -amount * .5 * (1 - t * .5);
            }
        }
}

void tone(Pixels &pixels, const Scalar &alpha, std::size_t w, std::size_t h,
          const CameraRawOptions &p) {
    Scalar l(pixels.size()), lc(pixels.size()), base, clarity, texture;
    for (std::size_t i = 0; i < l.size(); ++i) {
        l[i] = std::max(0., luma(pixels[i]));
        lc[i] = std::min(1., l[i]);
    }
    const auto longest = double(std::max(w, h));
    if (p.highlights != 0 || p.shadows != 0)
        base = guided(lc, lc, alpha, w, h, std::max<std::size_t>(2, std::size_t(longest * .015)),
                      .02, true);
    if (p.clarity != 0)
        clarity = guided(lc, lc, alpha, w, h, std::max<std::size_t>(3, std::size_t(longest * .02)),
                         .005, true);
    if (p.texture != 0)
        texture = gaussian(lc, alpha, w, h, std::max(longest * .002, 1.5 * p.pixelScale));
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        auto lv = lc[i];
        if (!base.empty()) {
            const auto b = detail::clamp(base[i]), hm = smooth(.45, .95, b),
                       sm = 1 - smooth(.05, .5, b);
            lv *= std::exp2(p.highlights / 100 * 1.2 * hm + p.shadows / 100 * 1.4 * sm);
        }
        if (!clarity.empty())
            lv += p.clarity / 100 * .8 * (lc[i] - clarity[i]) *
                  (1 - std::pow(2 * detail::clamp(lv) - 1, 2));
        if (!texture.empty())
            lv += p.texture / 100 * .9 * std::clamp(lc[i] - texture[i], -.1, .1);
        if (p.whites != 0)
            lv += p.whites / 100 * .25 * smooth(.4, 1, lv);
        if (p.blacks != 0)
            lv += p.blacks / 100 * .15 * (1 - smooth(0, .45, lv));
        if (p.contrast != 0)
            lv -= p.contrast / 100 * .6 * std::sin(2 * filter_detail::pi * detail::clamp(lv)) /
                  (2 * filter_detail::pi);
        lv = std::max(0., lv) + l[i] - lc[i];
        for (auto &c : pixels[i])
            c = l[i] > 1e-5 ? c * lv / l[i] : c + lv - l[i];
    }
}

void color(Pixels &pixels, const CameraRawOptions &p) {
    const auto parametric =
        p.curveHighlights != 0 || p.curveLights != 0 || p.curveDarks != 0 || p.curveShadows != 0
            ? parametricLut(p)
            : Scalar{};
    const auto master = !linearCurve(p.pointCurve) ? pointLut(p.pointCurve) : Scalar{};
    const std::array<Scalar, 3> channels{
        !linearCurve(p.pointCurveRed) ? pointLut(p.pointCurveRed) : Scalar{},
        !linearCurve(p.pointCurveGreen) ? pointLut(p.pointCurveGreen) : Scalar{},
        !linearCurve(p.pointCurveBlue) ? pointLut(p.pointCurveBlue) : Scalar{}};
    const std::array<CameraRawWheel, 4> wheels{p.gradeShadows, p.gradeMidtones, p.gradeHighlights,
                                               p.gradeGlobal};
    std::array<Rgb, 4> tints;
    for (std::size_t i = 0; i < 4; ++i) {
        auto c = rgb(wheels[i].hue, 1, 1);
        const auto y = luma(c);
        for (auto &v : c)
            v = (v - y) * wheels[i].saturation / 100 * .3;
        tints[i] = c;
    }
    const auto blend = .15 + p.gradeBlending / 100 * .35, balance = p.gradeBalance / 100 * .25;
    const auto useHsl = !hslNeutral(p), useGrade = !gradeNeutral(p);
    for (auto &c : pixels) {
        if (p.vibrance != 0 || p.saturation != 0) {
            const auto y = luma(c), mx = *std::max_element(c.begin(), c.end()),
                       mn = *std::min_element(c.begin(), c.end());
            const auto sat = mx > 1e-6 ? (mx - mn) / mx : 0;
            const auto skin = std::clamp(1 - std::abs(hsv(c)[0] - 25) / 25, 0., 1.) * .5;
            const auto k =
                std::max(0., 1 + p.saturation / 100 + p.vibrance / 100 * (1 - sat) * (1 - skin));
            for (auto &v : c)
                v = y + (v - y) * k;
        }
        for (auto &v : c) {
            if (!parametric.empty())
                v = lutValue(parametric, v);
            if (!master.empty())
                v = lutValue(master, v);
        }
        for (std::size_t i = 0; i < 3; ++i)
            if (!channels[i].empty())
                c[i] = lutValue(channels[i], c[i]);
        if (useHsl) {
            auto nonnegative = c;
            for (auto &v : nonnegative)
                v = std::max(0., v);
            const auto hs = hsv(nonnegative);
            if (hs[1] > 1e-4) {
                const auto weights = bands(hs[0]);
                double dh = 0, ds = 0, dl = 0;
                for (std::size_t i = 0; i < 8; ++i) {
                    dh += weights[i] * p.hslHue[i];
                    ds += weights[i] * p.hslSaturation[i];
                    dl += weights[i] * p.hslLuminance[i];
                }
                auto out = rgb(hs[0] + dh / 100 * 30, detail::clamp(hs[1] * (1 + ds / 100)), hs[2]);
                const auto l0 = luma(c), l1 = luma(out);
                if (l1 > 1e-6)
                    for (auto &v : out)
                        v *= l0 / l1;
                for (std::size_t i = 0; i < 3; ++i)
                    c[i] = out[i] * std::exp2(dl / 100 * hs[1]);
            }
        }
        if (useGrade) {
            const auto y = detail::clamp(luma(c)), sh = 1 - smooth(0, .5 + balance + blend, y),
                       hi = smooth(.5 + balance - blend, 1, y);
            const std::array<double, 4> masks{sh, std::max(0., 1 - sh - hi), hi, 1};
            for (std::size_t i = 0; i < 4; ++i) {
                for (std::size_t j = 0; j < 3; ++j)
                    c[j] += masks[i] * tints[i][j];
                if (wheels[i].luminance != 0)
                    for (auto &v : c)
                        v *= std::exp2(masks[i] * wheels[i].luminance / 100 * .6);
            }
        }
    }
}

void detailStage(Pixels &pixels, const Scalar &alpha, std::size_t w, std::size_t h,
                 const CameraRawOptions &p) {
    Scalar y(pixels.size()), cb(pixels.size()), cr(pixels.size());
    for (std::size_t i = 0; i < y.size(); ++i) {
        y[i] = luma(pixels[i]);
        cb[i] = pixels[i][2] - y[i];
        cr[i] = pixels[i][0] - y[i];
    }
    if (p.noiseLuminance > 0) {
        const auto n = p.noiseLuminance / 100;
        const auto radius = std::size_t(std::max(1., std::round((1 + 3 * n) * p.pixelScale)));
        const auto filtered = guided(y, y, alpha, w, h, radius, std::pow(n * .06, 2) + 1e-6);
        const auto keep = p.noiseLuminanceDetail / 100 * .5;
        for (std::size_t i = 0; i < y.size(); ++i)
            y[i] = filtered[i] + (y[i] - filtered[i]) * keep;
    }
    if (p.noiseColor > 0) {
        const auto sigma = std::max(.3, p.noiseColor / 100 * 6 * p.pixelScale);
        const auto bcb = gaussian(cb, alpha, w, h, sigma), bcr = gaussian(cr, alpha, w, h, sigma);
        const auto keep = p.noiseColorDetail / 100 * .3;
        for (std::size_t i = 0; i < y.size(); ++i) {
            cb[i] = bcb[i] + (cb[i] - bcb[i]) * keep;
            cr[i] = bcr[i] + (cr[i] - bcr[i]) * keep;
        }
    }
    if (p.sharpenAmount > 0) {
        const auto blur = gaussian(y, alpha, w, h, std::max(.3, p.sharpenRadius * p.pixelScale));
        Scalar mask;
        if (p.sharpenMasking > 0) {
            const auto smoothed = gaussian(y, alpha, w, h, std::max(.5, p.pixelScale));
            const auto t = p.sharpenMasking / 100 * .08;
            mask.resize(y.size());
            for (std::size_t yy = 0; yy < h; ++yy)
                for (std::size_t x = 0; x < w; ++x) {
                    const auto gx = smoothed[yy * w + std::min(x + 1, w - 1)] -
                                    smoothed[yy * w + (x ? x - 1 : 0)];
                    const auto gy = smoothed[std::min(yy + 1, h - 1) * w + x] -
                                    smoothed[(yy ? yy - 1 : 0) * w + x];
                    mask[yy * w + x] = smooth(t * .5, t + 1e-4, std::hypot(gx, gy));
                }
        }
        const auto amount = p.sharpenAmount / 100 * 1.2, d = p.sharpenDetail / 100,
                   limit = .03 + d * .3;
        for (std::size_t i = 0; i < y.size(); ++i)
            y[i] += amount * std::clamp(y[i] - blur[i], -limit, limit) * (.5 + .5 * d) *
                    (mask.empty() ? 1 : mask[i]);
    }
    for (std::size_t i = 0; i < y.size(); ++i) {
        const auto r = cr[i] + y[i], b = cb[i] + y[i];
        pixels[i] = {r, (y[i] - .2126 * r - .0722 * b) / .7152, b};
    }
}

void vignette(Pixels &pixels, std::size_t w, std::size_t h, const CameraRawOptions &p) {
    const auto amount = p.vignetteAmount / 100, mid = p.vignetteMidpoint / 100,
               feather = p.vignetteFeather / 100, round = p.vignetteRoundness / 100;
    const auto cx = double(w) / 2, cy = double(h) / 2;
    const auto radius = std::min(cx, cy) + (std::max(cx, cy) - std::min(cx, cy)) * (1 - round);
    const auto ax = round >= 0 && cx >= cy ? radius : cx, ay = round >= 0 && cy > cx ? radius : cy;
    const auto power = 2 + std::max(0., -round) * 6, inner = 1 - mid * .8;
    const auto edge0 = inner - feather * .5 * inner,
               edge1 = inner + feather * .5 * (1.4 - inner) + .02;
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x) {
            const auto dx = std::abs((x + .5 - cx) / ax), dy = std::abs((y + .5 - cy) / ay);
            const auto m = smooth(edge0, edge1,
                                  std::pow(std::pow(dx, power) + std::pow(dy, power), 1 / power));
            if (m <= 0)
                continue;
            auto &c = pixels[y * w + x];
            const auto l = detail::clamp(luma(c));
            if (p.vignetteStyle == CameraRawVignetteStyle::PaintOverlay) {
                const auto target = amount < 0 ? 0 : 1;
                for (auto &v : c)
                    v += (target - v) * m * std::abs(amount);
            } else {
                auto k = std::exp2(amount * 2 * m);
                if (amount < 0 && p.vignetteHighlights > 0)
                    k += (1 - k) * smooth(.6, 1, l) * p.vignetteHighlights / 100;
                for (auto &v : c)
                    v = p.vignetteStyle == CameraRawVignetteStyle::ColorPriority ? v + l * k - l
                                                                                 : v * k;
            }
        }
}
double hash(std::int64_t x, std::int64_t y, std::uint64_t seed) {
    auto z = std::uint64_t(x) * 0x9e3779b97f4a7c15ULL ^ std::uint64_t(y) * 0xc2b2ae3d27d4eb4fULL ^
             seed * 0x165667b19e3779f9ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z ^= z >> 31;
    return double(z >> 40) / 16777216.;
}
double valueNoise(double x, double y, std::uint64_t seed) {
    const auto xi = std::int64_t(std::floor(x)), yi = std::int64_t(std::floor(y));
    const auto tx = x - xi, ty = y - yi, sx = tx * tx * (3 - 2 * tx), sy = ty * ty * (3 - 2 * ty);
    const auto top = hash(xi, yi, seed) + (hash(xi + 1, yi, seed) - hash(xi, yi, seed)) * sx;
    const auto bottom =
        hash(xi, yi + 1, seed) + (hash(xi + 1, yi + 1, seed) - hash(xi, yi + 1, seed)) * sx;
    return top + (bottom - top) * sy;
}
} // namespace

Result<Image> cameraRawFilter(ImageView image, const CameraRawOptions &options) {
    if (!validOptions(options))
        return std::unexpected(filter_detail::invalid());
    const auto count = detail::pixelCount(image.width, image.height);
    if (!count)
        return std::unexpected(count.error());
    if (image.width > 65536 || image.height > 65536 || *count > 64 * 1024 * 1024)
        return std::unexpected(Error{ErrorCode::InvalidDimensions,
                                     "Camera Raw dimensions exceed bounded processing storage"});
    return filter_detail::guarded(image, [&]() {
        if (identity(options))
            return detail::copy(image);
        const auto &p = options;
        const auto w = image.width, h = image.height;
        Pixels pixels(image.pixels.size());
        Scalar alpha(image.pixels.size());
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            const auto q = image.pixels[i];
            alpha[i] = q.a;
            if (q.a > 0)
                pixels[i] = {q.r, q.g, q.b};
        }
        if (p.temperature != 0 || p.tint != 0 || p.exposure != 0) {
            Rgb gain{1 + .35 * p.temperature / 100, 1 - .25 * p.tint / 100,
                     1 - .35 * p.temperature / 100};
            const auto norm = luma(gain);
            for (auto &c : gain)
                c = c / norm * std::exp2(p.exposure);
            for (auto &q : pixels)
                for (std::size_t c = 0; c < 3; ++c)
                    q[c] = encodeExtended(std::max(0., detail::linear(q[c]) * gain[c]));
        }
        if (p.dehaze != 0)
            haze(pixels, alpha, w, h, p.dehaze / 100);
        if (!toneNeutral(p))
            tone(pixels, alpha, w, h, p);
        if (p.vibrance != 0 || p.saturation != 0 || !curveNeutral(p) || !hslNeutral(p) ||
            !gradeNeutral(p))
            color(pixels, p);
        if (p.noiseLuminance > 0 || p.noiseColor > 0 || p.sharpenAmount > 0)
            detailStage(pixels, alpha, w, h, p);
        if (p.vignetteAmount != 0)
            vignette(pixels, w, h, p);
        if (p.grainAmount > 0) {
            const auto cell = 1 + p.grainSize / 25, rough = p.grainRoughness / 100,
                       amount = p.grainAmount / 100 * .12;
            for (std::size_t y = 0; y < h; ++y)
                for (std::size_t x = 0; x < w; ++x) {
                    const auto fx = x / p.pixelScale / cell, fy = y / p.pixelScale / cell;
                    const auto n =
                        valueNoise(fx, fy, p.seed) * (1 - rough) +
                        valueNoise(fx * 2.3, fy * 2.3, std::uint64_t(p.seed) + 7) * rough;
                    const auto l = detail::clamp(luma(pixels[y * w + x])),
                               g = (n - .5) * 2 * amount * (.4 + 2.4 * l * (1 - l));
                    for (auto &v : pixels[y * w + x])
                        v += g;
                }
        }
        auto output = detail::copy(image);
        for (std::size_t i = 0; i < pixels.size(); ++i)
            if (alpha[i] > 0)
                output.pixels[i] = {float(detail::clamp(pixels[i][0])),
                                    float(detail::clamp(pixels[i][1])),
                                    float(detail::clamp(pixels[i][2])), image.pixels[i].a};
        return output;
    });
}
} // namespace iiGraphicAdjust
