// Algorithms adapted from LightCraft; see third_party/lightcraft/LICENSE-MIT.
#include "DevelopInternal.h"
#include <iiGraphicAdjust/DevelopEffects.h>

namespace iiGraphicAdjust {
namespace dd = develop_detail;
namespace {
double valueNoise(double x, double y, std::uint32_t seed) {
    const auto xi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(std::floor(x))),
               yi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(std::floor(y)));
    const auto hash = [&](std::uint32_t a, std::uint32_t b) {
        auto v = a * 0x8da6b343U ^ b * 0xd8163841U ^ seed * 0xcb1ab31fU;
        v ^= v >> 13;
        v *= 0x5bd1e995U;
        v ^= v >> 15;
        return (v & 0xffffU) / 32768. - 1;
    };
    const auto fx = x - std::floor(x), fy = y - std::floor(y), u = fx * fx * (3 - 2 * fx),
               v = fy * fy * (3 - 2 * fy);
    const auto a = hash(xi, yi), b = hash(xi, yi + 1);
    return a + (hash(xi + 1, yi) - a) * u +
           (b + (hash(xi + 1, yi + 1) - b) * u - a - (hash(xi + 1, yi) - a) * u) * v;
}
} // namespace
Result<Image> developEffects(ImageView image, const DevelopEffectsOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (!detail::inRange(o.texture, -100, 100) || !detail::inRange(o.clarity, -100, 100) ||
        !detail::inRange(o.dehaze, -100, 100))
        return std::unexpected(dd::invalid("Effects sliders outside supported ranges"));
    return detail::allocated([&] {
        if (o.texture == 0 && o.clarity == 0 && o.dehaze == 0)
            return detail::copy(image);
        auto f = dd::decode(image);
        const auto longEdge = double(std::max(f.width, f.height));
        const auto logs = dd::logs(f);
        dd::Plane texture, clarity, dark;
        double air = 1;
        if (o.texture != 0)
            texture = dd::gaussian(f, logs, std::max(.6, .0018 * longEdge));
        if (o.clarity != 0)
            clarity = dd::guided(f, logs, std::max(1., .012 * longEdge), .8);
        if (o.dehaze != 0) {
            dark.reserve(f.rgb.size());
            for (auto c : f.rgb)
                dark.push_back(std::min({c[0], c[1], c[2]}));
            dark = dd::gaussian(f, dark, std::max(1., .02 * longEdge));
            dd::Plane sample;
            for (std::size_t i = 0; i < dark.size(); ++i)
                if (f.alpha[i] > 0)
                    sample.push_back(dark[i]);
            if (!sample.empty()) {
                const auto at =
                    std::min(sample.size() - 1, static_cast<std::size_t>(sample.size() * .995));
                std::nth_element(sample.begin(), sample.begin() + at, sample.end());
                air = std::max(.05, sample[at]);
            }
        }
        for (std::size_t i = 0; i < f.rgb.size(); ++i)
            if (f.alpha[i] > 0) {
                auto c = f.rgb[i];
                double delta = 0;
                if (o.dehaze != 0) {
                    const auto d = detail::clamp(dark[i] / air), dz = o.dehaze / 100;
                    if (dz > 0) {
                        const auto t = std::max(.12, 1 - .95 * dz * d);
                        for (auto &v : c)
                            v = std::max(0., (v - air * (1 - t)) / t);
                    } else {
                        const auto k = -dz * .7 * (.35 + .65 * d);
                        for (auto &v : c)
                            v += (air * .9 - v) * k;
                    }
                }
                if (o.clarity != 0) {
                    const auto det = std::clamp(logs[i] - clarity[i], -2.5, 2.5),
                               mid = std::exp(-std::pow(logs[i] / 3.2, 2));
                    delta += o.clarity / 100 * .85 * det * (.35 + .65 * mid);
                }
                if (o.texture != 0) {
                    const auto det = logs[i] - texture[i],
                               tame = 1 - .6 * dd::smooth(.4, 1.6, std::abs(det));
                    delta += o.texture / 100 * 1.1 * std::clamp(det, -1., 1.) * tame;
                }
                f.rgb[i] = dd::scale(c, std::exp2(delta));
            }
        return dd::encode(f, image);
    });
}
Result<Image> postCropVignette(ImageView image, const PostCropVignetteOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (!detail::inRange(o.amount, -100, 100) || !detail::inRange(o.midpoint, 0, 100) ||
        !detail::inRange(o.roundness, -100, 100) || !detail::inRange(o.feather, 0, 100) ||
        !detail::inRange(o.highlights, 0, 100) ||
        (o.style != VignetteStyle::HighlightPriority && o.style != VignetteStyle::ColorPriority &&
         o.style != VignetteStyle::PaintOverlay))
        return std::unexpected(dd::invalid("Invalid vignette settings"));
    return detail::allocated([&] {
        if (o.amount == 0)
            return detail::copy(image);
        auto f = dd::decode(image);
        const auto aspect = double(f.width) / f.height, mix = (o.roundness / 100 + 1) / 2,
                   power = o.roundness >= 0 ? 2 : 2 - o.roundness / 100 * 6;
        const auto sx = 1 + (aspect - 1) * mix, sy = 1 + (1 / aspect - 1) * mix;
        for (std::size_t y = 0; y < f.height; ++y)
            for (std::size_t x = 0; x < f.width; ++x) {
                const auto i = y * f.width + x;
                if (f.alpha[i] == 0)
                    continue;
                const auto ax = std::abs(((x + .5) / f.width * 2 - 1) * std::max(1., sx) /
                                         std::max(sx, sy)),
                           ay = std::abs(((y + .5) / f.height * 2 - 1) * std::max(1., sy) /
                                         std::max(sx, sy));
                const auto d = std::pow(std::pow(ax, power) + std::pow(ay, power), 1 / power),
                           t = dd::smooth(.15 + o.midpoint / 100 * .95,
                                          .20 + o.midpoint / 100 * .95 + o.feather / 100 * 1.1, d);
                if (o.amount < 0) {
                    auto gain = 1 + o.amount / 100 * t;
                    if (o.style == VignetteStyle::HighlightPriority)
                        gain += (1 - gain) * o.highlights / 100 *
                                dd::smooth(.4, 1, dd::luminance(f.rgb[i]));
                    f.rgb[i] = dd::scale(f.rgb[i], gain);
                } else
                    for (auto &v : f.rgb[i])
                        v += (1 - v) * o.amount / 100 * t * .85;
            }
        return dd::encode(f, image);
    });
}
Result<Image> filmGrain(ImageView image, const FilmGrainOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (!detail::inRange(o.amount, 0, 100) || !detail::inRange(o.size, 0, 100) ||
        !detail::inRange(o.roughness, 0, 100))
        return std::unexpected(dd::invalid("Invalid grain settings"));
    return detail::allocated([&] {
        auto out = detail::copy(image);
        if (o.amount == 0)
            return out;
        const auto cell = std::max(.6, (.0006 + o.size / 100 * .0024) *
                                           std::max(image.width, image.height)),
                   rough = o.roughness / 100;
        for (std::size_t y = 0; y < image.height; ++y)
            for (std::size_t x = 0; x < image.width; ++x) {
                auto &p = out.pixels[y * image.width + x];
                if (p.a == 0)
                    continue;
                const auto gx = (x + .5) / cell, gy = (y + .5) / cell;
                const auto noise = valueNoise(gx, gy, o.seed) * (1 - rough * .5) +
                                   valueNoise(gx * 2.3, gy * 2.3, o.seed ^ 0x55U) * rough * .7;
                const auto lum = .2126 * p.r + .7152 * p.g + .0722 * p.b,
                           k = o.amount / 100 * .13 * noise * (.35 + 2.6 * lum * (1 - lum));
                p.r = static_cast<float>(detail::clamp(p.r + k));
                p.g = static_cast<float>(detail::clamp(p.g + k));
                p.b = static_cast<float>(detail::clamp(p.b + k));
            }
        return out;
    });
}
Result<Image> developDetail(ImageView image, const DevelopDetailOptions &o) {
    if (auto c = detail::validate(image); !c)
        return std::unexpected(c.error());
    if (!detail::inRange(o.sharpenAmount, 0, 150) || !detail::inRange(o.sharpenMasking, 0, 100) ||
        !detail::inRange(o.luminanceNoise, 0, 100) || !detail::inRange(o.luminanceDetail, 0, 100) ||
        !detail::inRange(o.colorNoise, 0, 100) || !detail::inRange(o.colorDetail, 0, 100) ||
        !detail::inRange(o.colorSmoothness, 0, 100))
        return std::unexpected(dd::invalid("Invalid detail settings"));
    return detail::allocated([&] {
        if (o.sharpenAmount == 0 && o.luminanceNoise == 0 && o.colorNoise == 0)
            return detail::copy(image);
        auto f = dd::decode(image);
        if (o.luminanceNoise > 0) {
            const auto logs = dd::logs(f);
            const auto amount = o.luminanceNoise / 100;
            const auto smooth =
                dd::guided(f, logs, 1 + 2.5 * amount,
                           .002 + amount * amount * .25 * (1 - .7 * o.luminanceDetail / 100));
            for (std::size_t i = 0; i < f.rgb.size(); ++i)
                if (f.alpha[i] > 0)
                    f.rgb[i] =
                        dd::scale(f.rgb[i], std::exp2((smooth[i] - logs[i]) * std::sqrt(amount)));
        }
        if (o.colorNoise > 0) {
            std::array<dd::Plane, 3> chroma;
            for (auto c : f.rgb) {
                const auto y = std::max(1e-6, dd::luminance(c));
                for (int k = 0; k < 3; ++k)
                    chroma[k].push_back(c[k] / y);
            }
            const auto sigma = (1.5 + 6 * o.colorNoise / 100) * (1 + o.colorSmoothness / 100),
                       t = o.colorNoise / 100 * (1 - o.colorDetail / 100 * .5);
            for (int k = 0; k < 3; ++k) {
                const auto blurred = dd::gaussian(f, chroma[k], sigma);
                for (std::size_t i = 0; i < f.rgb.size(); ++i)
                    chroma[k][i] += (blurred[i] - chroma[k][i]) * t;
            }
            for (std::size_t i = 0; i < f.rgb.size(); ++i)
                if (f.alpha[i] > 0) {
                    const auto y = dd::luminance(f.rgb[i]);
                    for (int k = 0; k < 3; ++k)
                        f.rgb[i][k] = std::max(0., chroma[k][i] * y);
                }
        }
        if (o.sharpenAmount > 0) {
            const auto logs = dd::logs(f),
                       blurred =
                           dd::gaussian(f, logs, std::max(.6, .0018 * std::max(f.width, f.height)));
            for (std::size_t i = 0; i < f.rgb.size(); ++i)
                if (f.alpha[i] > 0) {
                    const auto det = logs[i] - blurred[i],
                               mask = o.sharpenMasking > 0
                                          ? dd::smooth(o.sharpenMasking / 100 * .25,
                                                       o.sharpenMasking / 100 * .25 + .15,
                                                       std::abs(det))
                                          : 1;
                    f.rgb[i] = dd::scale(f.rgb[i], std::exp2(o.sharpenAmount / 150 * 1.3 *
                                                             std::clamp(det, -.8, .8) * mask));
                }
        }
        return dd::encode(f, image);
    });
}
} // namespace iiGraphicAdjust
