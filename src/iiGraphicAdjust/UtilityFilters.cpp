#include "UtilityFilters.h"
#include "DevelopInternal.h"
#include "FilterInternal.h"
namespace iiGraphicAdjust {
namespace fd = filter_detail;
namespace {
std::vector<double> guided(const std::vector<double> &g, const std::vector<double> &p,
                           const std::vector<double> &alpha, std::size_t w, std::size_t h,
                           std::size_t radius, double eps) {
    fd::Buffer m(g.size()), weights(g.size()), scratch;
    for (std::size_t i = 0; i < g.size(); ++i) {
        m[i] = {g[i] * alpha[i], p[i] * alpha[i], g[i] * g[i] * alpha[i], g[i] * p[i] * alpha[i]};
        weights[i][0] = alpha[i];
    }
    for (bool horizontal : {true, false}) {
        fd::boxAxis(m, scratch, w, h, radius, horizontal);
        fd::boxAxis(weights, scratch, w, h, radius, horizontal);
    }
    fd::Buffer ab(g.size());
    for (std::size_t i = 0; i < g.size(); ++i) {
        if (weights[i][0] == 0)
            continue;
        auto v = m[i];
        for (auto &x : v)
            x /= weights[i][0];
        double a = (v[3] - v[0] * v[1]) / (std::max(0., v[2] - v[0] * v[0]) + eps);
        ab[i] = {a * alpha[i], (v[1] - a * v[0]) * alpha[i], alpha[i], 0};
    }
    for (bool horizontal : {true, false})
        fd::boxAxis(ab, scratch, w, h, radius, horizontal);
    auto out = p;
    for (std::size_t i = 0; i < p.size(); ++i)
        if (ab[i][2] > 0)
            out[i] = (ab[i][0] * g[i] + ab[i][1]) / ab[i][2];
    return out;
}
std::array<double, 3> hsv(std::array<double, 3> c, bool hsl) {
    double mx = std::max({c[0], c[1], c[2]}), mn = std::min({c[0], c[1], c[2]}), d = mx - mn, h = 0;
    if (d > 0) {
        h = mx == c[0]   ? (c[1] - c[2]) / d
            : mx == c[1] ? (c[2] - c[0]) / d + 2
                         : (c[0] - c[1]) / d + 4;
        h = std::fmod(h / 6 + 1, 1.);
    }
    if (!hsl)
        return {h, mx > 0 ? d / mx : 0, mx};
    double l = (mx + mn) / 2;
    return {h, d > 0 ? d / (2 * std::min(l, 1 - l)) : 0, l};
}
std::array<double, 3> rgb(std::array<double, 3> c, bool hsl) {
    double h = std::fmod(c[0], 1.) * 6, ch = (hsl ? 2 * std::min(c[2], 1 - c[2]) : c[2]) * c[1],
           x = ch * (1 - std::abs(std::fmod(h, 2) - 1)), m = c[2] - (hsl ? ch / 2 : ch);
    std::array<double, 3> v;
    switch (int(h)) {
    case 0:
        v = {ch, x, 0};
        break;
    case 1:
        v = {x, ch, 0};
        break;
    case 2:
        v = {0, ch, x};
        break;
    case 3:
        v = {0, x, ch};
        break;
    case 4:
        v = {x, 0, ch};
        break;
    default:
        v = {ch, 0, x};
    }
    for (auto &n : v)
        n += m;
    return v;
}
Result<Image> medianImpl(ImageView image, double radius, double threshold) {
    if (!detail::inRange(radius, 0, 500) || !detail::inRange(threshold, 0, 255))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        int r = int(std::round(radius));
        auto out = detail::copy(image);
        if (r == 0)
            return out;
        std::vector<std::array<int, 2>> offsets;
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (dx * dx + dy * dy <= r * r + r)
                    offsets.push_back({dx, dy});
        std::vector<std::pair<double, double>> values;
        values.reserve(offsets.size());
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            if (image.pixels[i].a > 0) {
                long long x = i % image.width, y = i / image.width;
                auto p = image.pixels[i];
                std::array<float *, 3> dest{&out.pixels[i].r, &out.pixels[i].g, &out.pixels[i].b};
                for (int c = 0; c < 3; ++c) {
                    values.clear();
                    double total = 0;
                    for (auto [dx, dy] : offsets) {
                        auto q = fd::get(image, x + dx, y + dy);
                        if (q.a > 0) {
                            values.emplace_back(std::array{q.r, q.g, q.b}[c], q.a);
                            total += q.a;
                        }
                    }
                    std::sort(values.begin(), values.end());
                    double acc = 0, m = std::array{p.r, p.g, p.b}[c];
                    for (auto [v, a] : values) {
                        acc += a;
                        if (acc >= total / 2) {
                            m = v;
                            break;
                        }
                    }
                    if (std::abs(m - *dest[c]) > threshold / 255)
                        *dest[c] = float(m);
                }
            }
        return out;
    });
}
Result<Image> morph(ImageView image, const MorphologyOptions &o, bool maximum) {
    if (!detail::inRange(o.radius, 0, 500) || unsigned(o.shape) > 1)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        auto radius =
            o.shape == MorphologyShape::Square ? std::round(o.radius) : std::floor(o.radius);
        if (radius == 0)
            return out;
        std::array<std::vector<double>, 4> channel;
        std::vector<double> alpha(image.pixels.size()), all(alpha.size(), 1);
        for (auto &c : channel)
            c.resize(alpha.size());
        for (std::size_t i = 0; i < alpha.size(); ++i) {
            auto p = image.pixels[i];
            channel[0][i] = p.r;
            channel[1][i] = p.g;
            channel[2][i] = p.b;
            channel[3][i] = alpha[i] = p.a;
        }
        if (o.shape == MorphologyShape::Square) {
            develop_detail::Frame frame;
            frame.width = image.width;
            frame.height = image.height;
            for (int c = 0; c < 4; ++c) {
                frame.alpha = c == 3 ? all : alpha;
                channel[c] =
                    develop_detail::extrema(frame, channel[c], std::size_t(radius), maximum);
            }
        } else {
            auto original = channel;
            int r = int(radius);
            for (std::size_t i = 0; i < alpha.size(); ++i) {
                long long x = i % image.width, y = i / image.width;
                for (int c = 0; c < 4; ++c) {
                    double value = maximum ? -std::numeric_limits<double>::infinity()
                                           : std::numeric_limits<double>::infinity();
                    for (int dy = -r; dy <= r; ++dy)
                        for (int dx = -r; dx <= r; ++dx)
                            if (dx * dx + dy * dy <= o.radius * o.radius) {
                                auto q = fd::get(image, x + dx, y + dy);
                                if (c < 3 && q.a == 0)
                                    continue;
                                double v = std::array{q.r, q.g, q.b, q.a}[c];
                                value = maximum ? std::max(value, v) : std::min(value, v);
                            }
                    channel[c][i] = value;
                }
            }
        }
        for (std::size_t i = 0; i < alpha.size(); ++i) {
            float a = o.affectAlpha ? float(channel[3][i]) : image.pixels[i].a;
            if (a == 0)
                continue;
            auto p = image.pixels[i];
            out.pixels[i] = {float(std::isfinite(channel[0][i]) ? channel[0][i] : p.r),
                             float(std::isfinite(channel[1][i]) ? channel[1][i] : p.g),
                             float(std::isfinite(channel[2][i]) ? channel[2][i] : p.b), a};
        }
        // Alpha erosion must still write zero coverage while retaining original hidden color.
        if (o.affectAlpha)
            for (std::size_t i = 0; i < alpha.size(); ++i)
                out.pixels[i].a = float(channel[3][i]);
        return out;
    });
}
} // namespace
Result<Image> average(ImageView image) {
    return fd::guarded(image, [&] {
        fd::Pixel acc{};
        for (auto p : image.pixels) {
            auto v = fd::premul(p);
            for (int c = 0; c < 4; ++c)
                acc[c] += v[c] / image.pixels.size();
        }
        const auto p = fd::unpremulRgb(acc);
        auto out = detail::copy(image);
        for (auto &q : out.pixels)
            if (q.a > 0) {
                q.r = p[0];
                q.g = p[1];
                q.b = p[2];
            }
        return out;
    });
}
Result<Image> filterAddNoise(ImageView image, const FilterNoiseOptions &o) {
    if (!detail::inRange(o.amount, 0, 400) || unsigned(o.distribution) > 1)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            if (out.pixels[i].a > 0) {
                std::array<float *, 3> cs{&out.pixels[i].r, &out.pixels[i].g, &out.pixels[i].b};
                for (int c = 0; c < 3; ++c)
                    *cs[c] = float(detail::clamp(
                        *cs[c] + o.amount / 200 *
                                     fd::noise(i % image.width, i / image.width,
                                               o.monochromatic ? 0 : c, o.seed,
                                               o.distribution == NoiseDistribution::Gaussian)));
            }
        return out;
    });
}
Result<Image> median(ImageView image, double radius) {
    return medianImpl(image, radius, 0);
}
Result<Image> dustAndScratches(ImageView image, const DustAndScratchesOptions &o) {
    return medianImpl(image, o.radius, o.threshold);
}
Result<Image> minimum(ImageView image, const MorphologyOptions &o) {
    return morph(image, o, false);
}
Result<Image> maximum(ImageView image, const MorphologyOptions &o) {
    return morph(image, o, true);
}
Result<Image> customFilter(ImageView image, const CustomFilterOptions &o) {
    if (!detail::inRange(o.scale, 1, 9999) || !detail::inRange(o.offset, -9999, 9999))
        return std::unexpected(fd::invalid());
    for (auto k : o.kernel)
        if (!detail::inRange(k, -9999, 9999))
            return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        if (std::all_of(o.kernel.begin(), o.kernel.end(), [](double k) { return k == 0; }))
            return out;
        for (std::size_t i = 0; i < out.pixels.size(); ++i)
            if (image.pixels[i].a > 0) {
                auto p = image.pixels[i];
                std::array<double, 3> acc{};
                long long x = i % image.width, y = i / image.width;
                for (int k = 0; k < 25; ++k)
                    if (o.kernel[k] != 0) {
                        auto q = fd::get(image, x + k % 5 - 2, y + k / 5 - 2);
                        if (q.a == 0)
                            q = p;
                        std::array v{q.r, q.g, q.b};
                        for (int c = 0; c < 3; ++c)
                            acc[c] += o.kernel[k] * v[c];
                    }
                out.pixels[i] = {float(detail::clamp(acc[0] / o.scale + o.offset / 255)),
                                 float(detail::clamp(acc[1] / o.scale + o.offset / 255)),
                                 float(detail::clamp(acc[2] / o.scale + o.offset / 255)), p.a};
            }
        return out;
    });
}
Result<Image> hsbHsl(ImageView image, FilterColorModel input, FilterColorModel output) {
    if (unsigned(input) > 2 || unsigned(output) > 2)
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        if (input == output)
            return out;
        for (auto &p : out.pixels)
            if (p.a > 0) {
                std::array<double, 3> c{p.r, p.g, p.b};
                if (input != FilterColorModel::Rgb)
                    c = rgb(c, input == FilterColorModel::Hsl);
                if (output != FilterColorModel::Rgb)
                    c = hsv(c, output == FilterColorModel::Hsl);
                p.r = float(detail::clamp(c[0]));
                p.g = float(detail::clamp(c[1]));
                p.b = float(detail::clamp(c[2]));
            }
        return out;
    });
}
Result<Image> deInterlace(ImageView image, bool eliminateEven, bool interpolate) {
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        for (std::size_t y = 0; y < image.height; ++y) {
            if ((y % 2 == 0) == eliminateEven)
                continue;
            auto above = y ? y - 1 : std::min(y + 1, image.height - 1),
                 below = y + 1 < image.height ? y + 1 : above;
            for (std::size_t x = 0; x < image.width; ++x) {
                auto a = fd::premul(image.pixels[above * image.width + x]);
                if (interpolate && y > 0 && y + 1 < image.height) {
                    auto b = fd::premul(image.pixels[below * image.width + x]);
                    for (int c = 0; c < 4; ++c)
                        a[c] = (a[c] + b[c]) / 2;
                }
                out.pixels[y * image.width + x] = fd::straight(a);
            }
        }
        return out;
    });
}
Result<Image> ntscColors(ImageView image) {
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        for (auto &p : out.pixels)
            if (p.a > 0) {
                double y = fd::luma(p), i = .595716 * p.r - .274453 * p.g - .321263 * p.b,
                       q = .211456 * p.r - .522591 * p.g + .311135 * p.b, c = std::hypot(i, q);
                if (c <= 1e-7)
                    continue;
                double k = std::min({1., std::max(0., ((120 - 7.5) / 92.5 - y) / c),
                                     std::max(0., (y - (-20 - 7.5) / 92.5) / c)});
                if (k >= 1)
                    continue;
                i *= k;
                q *= k;
                p.r = float(detail::clamp(y + .9563 * i + .621 * q));
                p.g = float(detail::clamp(y - .2721 * i - .6474 * q));
                p.b = float(detail::clamp(y - 1.107 * i + 1.7046 * q));
            }
        return out;
    });
}
Result<Image> crossGuidedBlur(ImageView image, ImageView guide, const GuidedBlurOptions &o) {
    if (!detail::inRange(o.radius, 0, 1000) || !detail::inRange(o.epsilon, 1e-12, 1))
        return std::unexpected(fd::invalid());
    if (auto v = detail::validate(guide); !v)
        return std::unexpected(v.error());
    if (image.width != guide.width || image.height != guide.height)
        return std::unexpected(Error{ErrorCode::SizeMismatch, "Guide must match image"});
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        auto r = std::size_t(std::round(o.radius));
        if (!r)
            return out;
        std::vector<double> g(out.pixels.size()), p(g.size()), alpha(g.size());
        for (std::size_t i = 0; i < g.size(); ++i) {
            g[i] = fd::luma(guide.pixels[i]);
            alpha[i] = double(image.pixels[i].a) * guide.pixels[i].a;
        }
        for (int c = 0; c < 3; ++c) {
            for (std::size_t i = 0; i < p.size(); ++i)
                p[i] = std::array{image.pixels[i].r, image.pixels[i].g, image.pixels[i].b}[c];
            auto q = guided(g, p, alpha, image.width, image.height, r, o.epsilon);
            for (std::size_t i = 0; i < q.size(); ++i)
                if (alpha[i] > 0) {
                    std::array<float *, 3> dest{&out.pixels[i].r, &out.pixels[i].g,
                                                &out.pixels[i].b};
                    *dest[c] = float(detail::clamp(q[i]));
                }
        }
        return out;
    });
}
Result<Image> guidedBlur(ImageView image, const GuidedBlurOptions &o) {
    return crossGuidedBlur(image, image, o);
}
Result<Image> reduceNoise(ImageView image, const ReduceNoiseOptions &o) {
    if (!detail::inRange(o.strength, 0, 10) || !detail::inRange(o.preserveDetails, 0, 100) ||
        !detail::inRange(o.reduceColorNoise, 0, 100) || !detail::inRange(o.sharpenDetails, 0, 100))
        return std::unexpected(fd::invalid());
    return fd::guarded(image, [&] {
        auto out = detail::copy(image);
        double s = o.strength / 10, col = o.reduceColorNoise / 100, sh = o.sharpenDetails / 100;
        if (s == 0 && col == 0 && sh == 0 && !o.removeJpegArtifact)
            return out;
        std::vector<double> y(out.pixels.size()), cb(y.size()), cr(y.size()), alpha(y.size());
        for (std::size_t i = 0; i < y.size(); ++i) {
            auto p = image.pixels[i];
            y[i] = fd::luma(p);
            cb[i] = p.b - y[i];
            cr[i] = p.r - y[i];
            alpha[i] = p.a;
        }
        if (o.removeJpegArtifact) {
            auto original = y;
            for (std::size_t yy = 1; yy + 1 < image.height; ++yy)
                for (std::size_t xx = 1; xx + 1 < image.width; ++xx)
                    if (xx % 8 == 0 || xx % 8 == 7 || yy % 8 == 0 || yy % 8 == 7) {
                        double sum = 0, weights = 0;
                        for (int dy = -1; dy <= 1; ++dy)
                            for (int dx = -1; dx <= 1; ++dx) {
                                auto k = (yy + dy) * image.width + xx + dx;
                                sum += original[k] * alpha[k];
                                weights += alpha[k];
                            }
                        if (weights > 0)
                            y[yy * image.width + xx] =
                                .5 * original[yy * image.width + xx] + .5 * sum / weights;
                    }
        }
        if (s > 0) {
            auto filtered = guided(y, y, alpha, image.width, image.height,
                                   std::size_t(std::round(1 + o.strength * .4)),
                                   std::pow(.004 + .05 * s, 2) * (1.05 - o.preserveDetails / 100));
            for (std::size_t i = 0; i < y.size(); ++i)
                y[i] += (filtered[i] - y[i]) * std::sqrt(s);
        }
        if (col > 0)
            for (auto *p : {&cb, &cr}) {
                auto filtered =
                    guided(y, *p, alpha, image.width, image.height,
                           std::size_t(std::round(1 + col * 8)), std::pow(.01 + .05 * col, 2));
                for (std::size_t i = 0; i < y.size(); ++i)
                    (*p)[i] += (filtered[i] - (*p)[i]) * col;
            }
        if (sh > 0) {
            auto synthetic = detail::copy(image);
            for (std::size_t i = 0; i < y.size(); ++i)
                synthetic.pixels[i] = {float(detail::clamp(y[i])), float(detail::clamp(y[i])),
                                       float(detail::clamp(y[i])), float(alpha[i])};
            auto b = fd::gauss(synthetic.view(), 1);
            for (std::size_t i = 0; i < y.size(); ++i)
                y[i] += (y[i] - fd::unpremulRgb(b[i])[0]) * sh * 1.2;
        }
        for (std::size_t i = 0; i < y.size(); ++i)
            if (alpha[i] > 0) {
                double r = cr[i] + y[i], b = cb[i] + y[i], g = (y[i] - .299 * r - .114 * b) / .587;
                out.pixels[i] = {float(detail::clamp(r)), float(detail::clamp(g)),
                                 float(detail::clamp(b)), image.pixels[i].a};
            }
        return out;
    });
}
} // namespace iiGraphicAdjust
