#include "PixelRenderInternal.h"
#include <iiGraphicAdjust/PixelFilters.h>
#include <map>

namespace iiGraphicAdjust {
namespace {
using namespace pixelrender;
using Key = std::pair<std::int64_t, std::int64_t>;
template <class F> Result<Image> process(ImageView in, F f) {
    if (auto r = pixelrender::validate(in); !r)
        return std::unexpected(r.error());
    return detail::allocated([&] { return f(); });
}
Rgba preserve(Color c, Rgba original) {
    return original.a > 0 ? color(c, original.a) : original;
}
std::int64_t floorDiv(std::int64_t x, std::int64_t n) {
    auto q = x / n, r = x % n;
    return q - (r < 0);
}
double mezzo(MezzotintPattern p, std::int64_t x, std::int64_t y, std::uint32_t c,
             std::uint32_t seed) {
    auto lines = [&](int len) {
        auto phase = std::int64_t(hash(0, y, c + 40, seed) * len);
        return hash(floorDiv(x + phase, len), y, c, seed);
    };
    auto strokes = [&](int len) {
        auto d = x - y, phase = std::int64_t(hash(d, 0, c + 50, seed) * len);
        return hash(floorDiv(x + y + phase, len), d, c + 60, seed);
    };
    switch (p) {
    case MezzotintPattern::FineDots:
        return hash(x, y, c, seed);
    case MezzotintPattern::MediumDots:
        return hash(floorDiv(x, 2), floorDiv(y, 2), c, seed);
    case MezzotintPattern::GrainyDots:
        return .5 * hash(x, y, c, seed) + .5 * hash(floorDiv(x, 3), floorDiv(y, 3), c + 7, seed);
    case MezzotintPattern::CoarseDots:
        return hash(floorDiv(x, 3), floorDiv(y, 3), c, seed);
    case MezzotintPattern::ShortLines:
        return lines(6);
    case MezzotintPattern::MediumLines:
        return lines(14);
    case MezzotintPattern::LongLines:
        return lines(32);
    case MezzotintPattern::ShortStrokes:
        return strokes(5);
    case MezzotintPattern::MediumStrokes:
        return strokes(10);
    case MezzotintPattern::LongStrokes:
        return strokes(20);
    }
    return 0;
}
} // namespace

Result<Image> colorHalftone(ImageView in, const ColorHalftoneOptions &o) {
    if (!detail::inRange(o.maximumRadius, 4, 127))
        return bad();
    for (auto a : o.angles)
        if (!detail::inRange(a, -360, 360))
            return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        double cell = o.maximumRadius * std::sqrt(2.);
        std::array<double, 3> sine{}, cosine{};
        for (int ch = 0; ch < 3; ++ch) {
            const auto angle = radians(o.angles[ch]);
            sine[ch] = std::sin(angle);
            cosine[ch] = std::cos(angle);
        }
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                auto original = in.pixels[i];
                if (original.a <= 0)
                    continue;
                Color vals{};
                for (int ch = 0; ch < 3; ++ch) {
                    double s = sine[ch], co = cosine[ch],
                           u = (x + .5) * co + (y + .5) * s, v = -(x + .5) * s + (y + .5) * co,
                           cov = 0;
                    auto cu = std::int64_t(std::floor(u / cell)),
                         cv = std::int64_t(std::floor(v / cell));
                    for (int dj = -1; dj <= 1; ++dj)
                        for (int di = -1; di <= 1; ++di) {
                            double mu = (cu + di + .5) * cell, mv = (cv + dj + .5) * cell,
                                   d = std::hypot(u - mu, v - mv);
                            if (d >= o.maximumRadius + .5)
                                continue;
                            double dx = mu * co - mv * s, dy = mu * s + mv * co, half = cell * .25;
                            Average a;
                            for (auto [sx, sy] :
                                 std::array<std::pair<double, double>, 5>{{{0, 0},
                                                                           {-half, -half},
                                                                           {half, -half},
                                                                           {-half, half},
                                                                           {half, half}}})
                                a.add(at(in, std::int64_t(std::floor(dx + sx)),
                                         std::int64_t(std::floor(dy + sy))));
                            double raw = a.alpha > 0 ? a.rgbMean()[ch] : rgb(original)[ch],
                                   ink = 1 - raw, touch = pi / 4;
                            double rad = ink <= touch ? cell * std::sqrt(ink / pi)
                                                      : cell * (.5 + (ink - touch) / (1 - touch) *
                                                                         (1 / std::sqrt(2.) - .5));
                            cov = std::max(cov,
                                           std::min(detail::clamp(rad - d + .5), pi * rad * rad));
                        }
                    vals[ch] = 1 - cov;
                }
                out.pixels[i] = preserve(vals, original);
            }
        return out;
    });
}
Result<Image> crystallize(ImageView in, const CrystallizeOptions &o) {
    if (!detail::inRange(o.cellSize, 3, 300))
        return bad();
    return process(in, [&] {
        auto out = detail::output(in);
        std::vector<Key> owners(in.pixels.size());
        std::map<Key, Average> cells;
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                double fx = x + .5, fy = y + .5, best = std::numeric_limits<double>::max();
                auto cx = std::int64_t(std::floor(fx / o.cellSize)),
                     cy = std::int64_t(std::floor(fy / o.cellSize));
                Key key{cx, cy};
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        auto [px, py] = cellPoint(cx + dx, cy + dy, o.cellSize, o.seed);
                        double d = std::pow(px - fx, 2) + std::pow(py - fy, 2);
                        if (d < best) {
                            best = d;
                            key = {cx + dx, cy + dy};
                        }
                    }
                auto i = y * in.width + x;
                owners[i] = key;
                cells[key].add(in.pixels[i]);
            }
        for (std::size_t i = 0; i < owners.size(); ++i)
            out.pixels[i] = cells.at(owners[i]).result();
        return out;
    });
}
Result<Image> mosaic(ImageView in, double cellSize) {
    if (!detail::inRange(cellSize, 2, 200))
        return bad();
    return process(in, [&] {
        auto out = detail::output(in);
        auto s = std::size_t(std::lround(cellSize));
        for (std::size_t y = 0; y < in.height; y += s)
            for (std::size_t x = 0; x < in.width; x += s) {
                Average a;
                auto x1 = std::min(in.width, x + s), y1 = std::min(in.height, y + s);
                for (auto yy = y; yy < y1; ++yy)
                    for (auto xx = x; xx < x1; ++xx)
                        a.add(in.pixels[yy * in.width + xx]);
                auto p = a.result();
                for (auto yy = y; yy < y1; ++yy)
                    for (auto xx = x; xx < x1; ++xx)
                        out.pixels[yy * in.width + xx] = p;
            }
        return out;
    });
}
Result<Image> facet(ImageView in) {
    return process(in, [&] {
        auto out = detail::copy(in);
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto original = in.pixels[y * in.width + x];
                if (original.a <= 0)
                    continue;
                double best = std::numeric_limits<double>::max();
                Color col = rgb(original);
                for (auto [qx, qy] :
                     std::array<std::pair<int, int>, 4>{{{-2, -2}, {0, -2}, {-2, 0}, {0, 0}}}) {
                    Average a;
                    double sum = 0, sum2 = 0, mass = 0;
                    for (int dy = 0; dy <= 2; ++dy)
                        for (int dx = 0; dx <= 2; ++dx) {
                            auto p = at(in, std::int64_t(x) + qx + dx, std::int64_t(y) + qy + dy);
                            double l = lum(p);
                            a.add(p);
                            sum += l * p.a;
                            sum2 += l * l * p.a;
                            mass += p.a;
                        }
                    if (mass <= 0)
                        continue;
                    double var = std::max(0., sum2 / mass - std::pow(sum / mass, 2));
                    if (var < best) {
                        best = var;
                        col = a.rgbMean();
                    }
                }
                out.pixels[y * in.width + x] = preserve(col, original);
            }
        return out;
    });
}
Result<Image> fragment(ImageView in) {
    return process(in, [&] {
        auto out = detail::output(in);
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                Average a;
                for (auto [dx, dy] :
                     std::array<std::pair<int, int>, 4>{{{-4, 0}, {4, 0}, {0, -4}, {0, 4}}})
                    a.add(at(in, std::int64_t(x) + dx, std::int64_t(y) + dy));
                out.pixels[y * in.width + x] = a.result();
            }
        return out;
    });
}
Result<Image> mezzotint(ImageView in, const MezzotintOptions &o) {
    if (o.pattern < MezzotintPattern::FineDots || o.pattern > MezzotintPattern::LongStrokes)
        return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        for (std::size_t i = 0; i < in.pixels.size(); ++i) {
            auto p = in.pixels[i];
            if (p.a <= 0)
                continue;
            auto c = rgb(p);
            for (int ch = 0; ch < 3; ++ch)
                c[ch] = c[ch] > mezzo(o.pattern, i % in.width, i / in.width, ch, o.seed) ? 1 : 0;
            out.pixels[i] = preserve(c, p);
        }
        return out;
    });
}
Result<Image> pointillize(ImageView in, const PointillizeOptions &o) {
    if (!detail::inRange(o.cellSize, 3, 300) || !validColor(o.background))
        return bad();
    return process(in, [&] {
        auto out = detail::output(in);
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                double fx = x + .5, fy = y + .5;
                auto cx = std::int64_t(std::floor(fx / o.cellSize)),
                     cy = std::int64_t(std::floor(fy / o.cellSize));
                double best = -.5, bx = 0, by = 0;
                Key key{};
                bool hit = false;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        auto gx = cx + dx, gy = cy + dy;
                        auto [px, py] = cellPoint(gx, gy, o.cellSize, o.seed);
                        double rad = o.cellSize * (.45 + .35 * hash(gx, gy, 13, o.seed)),
                               depth = rad - std::hypot(px - fx, py - fy);
                        if (depth > best) {
                            best = depth;
                            bx = px;
                            by = py;
                            key = {gx, gy};
                            hit = true;
                        }
                    }
                Rgba canvas = o.background;
                canvas.a = std::max(canvas.a, in.pixels[y * in.width + x].a);
                canvas = clean(canvas);
                if (hit) {
                    auto p =
                        clean(at(in, std::int64_t(std::floor(bx)), std::int64_t(std::floor(by))));
                    double j = (hash(key.first, key.second, 17, o.seed) - .5) * .08;
                    auto c = rgb(p);
                    if (p.a > 0)
                        for (auto &v : c)
                            v = detail::clamp(v + j);
                    Average a;
                    double k = detail::clamp(best + .5);
                    a.add(canvas, 1 - k);
                    a.add(color(c, p.a), k);
                    canvas = a.result();
                }
                out.pixels[y * in.width + x] = canvas;
            }
        return out;
    });
}
Result<Image> emboss(ImageView in, const EmbossOptions &o) {
    if (!detail::inRange(o.angle, -180, 180) || !detail::inRange(o.height, 1, 100) ||
        !detail::inRange(o.amount, 1, 500))
        return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        auto dx = std::int64_t(std::lround(std::cos(radians(o.angle)) * o.height)),
             dy = std::int64_t(std::lround(-std::sin(radians(o.angle)) * o.height));
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                auto p = in.pixels[i];
                if (p.a <= 0)
                    continue;
                double a = lum(visibleAt(in, std::int64_t(x) + dx, std::int64_t(y) + dy, p)),
                       b = lum(visibleAt(in, std::int64_t(x) - dx, std::int64_t(y) - dy, p));
                double v = .5 + o.amount / 100 * (a - b);
                out.pixels[i] = preserve({v, v, v}, p);
            }
        return out;
    });
}
Result<Image> findEdges(ImageView in) {
    return process(in, [&] {
        auto out = detail::copy(in);
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                auto p = in.pixels[i];
                if (p.a <= 0)
                    continue;
                Color c{};
                for (int ch = 0; ch < 3; ++ch) {
                    auto g = [&](int dx, int dy) {
                        return rgb(
                            visibleAt(in, std::int64_t(x) + dx, std::int64_t(y) + dy, p))[ch];
                    };
                    double gx = g(1, -1) + 2 * g(1, 0) + g(1, 1) - g(-1, -1) - 2 * g(-1, 0) -
                                g(-1, 1),
                           gy = g(-1, 1) + 2 * g(0, 1) + g(1, 1) - g(-1, -1) - 2 * g(0, -1) -
                                g(1, -1);
                    c[ch] = 1 - std::hypot(gx, gy) / 4;
                }
                out.pixels[i] = preserve(c, p);
            }
        return out;
    });
}
Result<Image> solarize(ImageView in) {
    return process(in, [&] {
        auto out = detail::copy(in);
        for (auto &p : out.pixels) {
            if (p.a <= 0)
                continue;
            auto c = rgb(p);
            for (auto &v : c)
                if (v > .5)
                    v = 1 - v;
            p = preserve(c, p);
        }
        return out;
    });
}
Result<Image> diffuse(ImageView in, const DiffuseOptions &o) {
    if (o.mode < DiffuseMode::Normal || o.mode > DiffuseMode::Anisotropic)
        return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        constexpr std::array<Key, 8> ring{
            {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}};
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                auto p = in.pixels[i];
                if (o.mode == DiffuseMode::Anisotropic) {
                    if (p.a <= 0)
                        continue;
                    auto start = std::size_t(hash(x, y, 3, o.seed) * 8);
                    double best = std::numeric_limits<double>::max();
                    Rgba nb = p;
                    for (std::size_t k = 0; k < 8; ++k) {
                        auto [dx, dy] = ring[(start + k) % 8];
                        auto q = at(in, std::int64_t(x) + dx, std::int64_t(y) + dy);
                        if (q.a <= 0)
                            continue;
                        double d = std::abs(lum(q) - lum(p));
                        if (d < best) {
                            best = d;
                            nb = q;
                        }
                    }
                    Average a;
                    a.add(p);
                    a.add(nb);
                    out.pixels[i] = preserve(a.rgbMean(), p);
                } else {
                    auto dx = std::int64_t(hash(x, y, 1, o.seed) * 3) - 1,
                         dy = std::int64_t(hash(x, y, 2, o.seed) * 3) - 1;
                    auto nb = clean(at(in, std::int64_t(x) + dx, std::int64_t(y) + dy));
                    if (o.mode == DiffuseMode::Normal ||
                        (nb.a > 0 && (o.mode == DiffuseMode::DarkenOnly ? lum(nb) < lum(clean(p))
                                                                        : lum(nb) > lum(clean(p)))))
                        out.pixels[i] = nb;
                }
            }
        return out;
    });
}
Result<Image> tiles(ImageView in, const TilesOptions &o) {
    if (o.count < 1 || o.count > 99 || !detail::inRange(o.maximumOffset, 0, 99) ||
        o.fill < TileFill::Background || o.fill > TileFill::Unaltered ||
        !validColor(o.foreground) || !validColor(o.background))
        return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        double ts = std::max(1., double(std::min(in.width, in.height)) / o.count),
               m = o.maximumOffset / 100 * ts;
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                double fx = x + .5, fy = y + .5;
                auto gx = std::int64_t(std::floor(fx / ts)), gy = std::int64_t(std::floor(fy / ts));
                bool hit = false;
                Key order{};
                double oxBest = 0, oyBest = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        auto cx = gx + dx, cy = gy + dy;
                        double ox = (hash(cx, cy, 31, o.seed) * 2 - 1) * m,
                               oy = (hash(cx, cy, 32, o.seed) * 2 - 1) * m, u = fx - ox - cx * ts,
                               v = fy - oy - cy * ts;
                        Key k{cy, cx};
                        if (u >= 0 && v >= 0 && u < ts && v < ts && cx >= 0 && cy >= 0 &&
                            cx * ts < in.width && cy * ts < in.height && (!hit || k > order)) {
                            hit = true;
                            order = k;
                            oxBest = ox;
                            oyBest = oy;
                        }
                    }
                auto i = y * in.width + x;
                if (hit)
                    out.pixels[i] = clean(at(in, std::int64_t(std::floor(fx - oxBest)),
                                             std::int64_t(std::floor(fy - oyBest))));
                else if (o.fill == TileFill::Foreground)
                    out.pixels[i] = clean(o.foreground);
                else if (o.fill == TileFill::Background)
                    out.pixels[i] = clean(o.background);
                else if (o.fill == TileFill::Inverse && out.pixels[i].a > 0) {
                    auto c = rgb(out.pixels[i]);
                    for (auto &v : c)
                        v = 1 - v;
                    out.pixels[i] = preserve(c, out.pixels[i]);
                }
            }
        return out;
    });
}
Result<Image> traceContour(ImageView in, const TraceContourOptions &o) {
    if (!detail::inRange(o.level, 0, 255))
        return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        double t = o.level / 255;
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                auto p = in.pixels[i];
                if (p.a <= 0)
                    continue;
                Color c{};
                for (int ch = 0; ch < 3; ++ch) {
                    bool me = rgb(p)[ch] > t, line = false;
                    if (me == o.upper)
                        for (auto [dx, dy] : std::array<Key, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}})
                            if ((rgb(visibleAt(in, std::int64_t(x) + dx, std::int64_t(y) + dy,
                                               p))[ch] > t) != me)
                                line = true;
                    c[ch] = line ? 0 : 1;
                }
                out.pixels[i] = preserve(c, p);
            }
        return out;
    });
}
Result<Image> wind(ImageView in, const WindOptions &o) {
    if (o.method < WindMethod::Wind || o.method > WindMethod::Stagger)
        return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        int length = o.method == WindMethod::Wind    ? 16
                     : o.method == WindMethod::Blast ? 40
                                                     : 12,
            d = o.fromRight ? -1 : 1;
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                auto p = in.pixels[i];
                if (o.method == WindMethod::Stagger) {
                    auto len = 8 + std::int64_t(hash(0, y, 41, o.seed) * 32),
                         phase = std::int64_t(hash(1, y, 42, o.seed) * len),
                         seg = floorDiv(std::int64_t(x) * d + phase, len),
                         shift = std::int64_t(std::pow(hash(seg, y, 43, o.seed), 2) * length);
                    out.pixels[i] = clean(at(in, std::int64_t(x) - d * shift, y));
                    continue;
                }
                double best = 0;
                std::int64_t source = x;
                for (int k = 1; k <= length; ++k) {
                    auto s = std::int64_t(x) - d * k;
                    double len = std::max(1., std::pow(hash(s, y, 44, o.seed), 2) * length);
                    if (k > len)
                        continue;
                    auto a = clean(at(in, s, y)), b = clean(at(in, s - d, y));
                    double edge = std::min(1., std::abs(lum(a) - lum(b)) *
                                                   (o.method == WindMethod::Blast ? 8 : 4)),
                           weight = edge * (1 - k / (len + 1));
                    if (weight > best) {
                        best = weight;
                        source = s;
                    }
                }
                if (best > 0) {
                    Average a;
                    a.add(p, 1 - best);
                    a.add(clean(at(in, source, y)), best);
                    out.pixels[i] = a.result();
                }
            }
        return out;
    });
}
Result<Image> extrude(ImageView in, const ExtrudeOptions &o) {
    if (o.shape < ExtrudeShape::Blocks || o.shape > ExtrudeShape::Pyramids ||
        !detail::inRange(o.size, 2, 255) || !detail::inRange(o.depth, 1, 255))
        return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        auto cs = std::int64_t(std::lround(o.size));
        auto gw = (in.width + cs - 1) / cs, gh = (in.height + cs - 1) / cs;
        std::vector<Rgba> cols(gw * gh);
        std::vector<double> heights(gw * gh);
        for (std::size_t gy = 0; gy < gh; ++gy)
            for (std::size_t gx = 0; gx < gw; ++gx) {
                Average a;
                auto step = std::size_t(std::max(std::int64_t{1}, cs / 8));
                for (auto y = gy * cs; y < std::min<std::size_t>(in.height, (gy + 1) * cs);
                     y += step)
                    for (auto x = gx * cs; x < std::min<std::size_t>(in.width, (gx + 1) * cs);
                         x += step)
                        a.add(in.pixels[y * in.width + x]);
                auto i = gy * gw + gx;
                cols[i] = a.result();
                const auto mean = a.rgbMean();
                heights[i] = o.levelBased ? .299 * mean[0] + .587 * mean[1] + .114 * mean[2]
                                          : hash(gx, gy, 21, o.seed);
            }
        double cx = in.width / 2., cy = in.height / 2.,
               diag = std::max(1., std::hypot(double(in.width), double(in.height)) / 2),
               reach = o.depth * .25;
        auto incomplete = [&](std::int64_t gx, std::int64_t gy) {
            return (gx + 1) * cs > std::int64_t(in.width) ||
                   (gy + 1) * cs > std::int64_t(in.height);
        };
        auto shade = [&](Rgba c, double k) {
            auto a = rgb(c);
            for (auto &v : a)
                v *= k;
            return color(a, c.a);
        };
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto gx = std::int64_t(x) / cs, gy = std::int64_t(y) / cs;
                auto i = y * in.width + x;
                double fx = x + .5, fy = y + .5;
                if (o.shape == ExtrudeShape::Pyramids) {
                    if (o.maskIncompleteBlocks && incomplete(gx, gy))
                        continue;
                    auto ci = std::size_t(gy) * gw + gx;
                    double u = fx - (gx + .5) * cs, v = fy - (gy + .5) * cs,
                           hgt = heights[ci] * o.depth / 255 * cs, half = cs / 2.;
                    double nx = std::abs(u) >= std::abs(v) ? std::copysign(hgt, u) : 0,
                           ny = std::abs(u) >= std::abs(v) ? 0 : std::copysign(hgt, v);
                    double lambert = std::max(0., (-.5 * nx - .5 * ny + half / std::sqrt(2.)) /
                                                      std::hypot(nx, ny, half));
                    out.pixels[i] = shade(cols[ci], .35 + .9 * lambert);
                    continue;
                }
                auto k = std::int64_t(std::ceil(reach / cs)) + 1;
                double best = -1;
                std::int64_t bx = 0, by = 0;
                int face = 0;
                bool hit = false;
                for (auto dy = -k; dy <= k; ++dy)
                    for (auto dx = -k; dx <= k; ++dx) {
                        auto tx = gx + dx, ty = gy + dy;
                        if (tx < 0 || ty < 0 || tx >= std::int64_t(gw) || ty >= std::int64_t(gh) ||
                            (o.maskIncompleteBlocks && incomplete(tx, ty)))
                            continue;
                        auto ci = std::size_t(ty) * gw + tx;
                        double h = heights[ci];
                        if (h <= best)
                            continue;
                        double mx = (tx + .5) * cs, my = (ty + .5) * cs, off = reach * h / diag,
                               ox = (mx - cx) * off, oy = (my - cy) * off, half = cs / 2.;
                        auto interval = [&](double p, double m, double shift) {
                            if (std::abs(shift) < 1e-9)
                                return std::abs(p - m) <= half ? std::pair{0., 1.}
                                                               : std::pair{1., 0.};
                            double a = (p - m - half) / shift, b = (p - m + half) / shift;
                            return std::pair{std::max(0., std::min(a, b)),
                                             std::min(1., std::max(a, b))};
                        };
                        auto ix = interval(fx, mx, ox), iy = interval(fy, my, oy);
                        if (std::max(ix.first, iy.first) > std::min(ix.second, iy.second))
                            continue;
                        face = std::min(ix.second, iy.second) >= 1 ? 0
                               : ix.second < iy.second             ? (ox > 0 ? 1 : 2)
                                                                   : (oy > 0 ? 3 : 4);
                        best = h;
                        bx = tx;
                        by = ty;
                        hit = true;
                    }
                if (!hit)
                    continue;
                auto ci = std::size_t(by) * gw + bx;
                if (face == 0 && !o.solidFrontFaces) {
                    double mx = (bx + .5) * cs, my = (by + .5) * cs, off = reach * best / diag;
                    auto sx = std::clamp(std::int64_t(std::floor(fx - (mx - cx) * off)), bx * cs,
                                         (bx + 1) * cs - 1),
                         sy = std::clamp(std::int64_t(std::floor(fy - (my - cy) * off)), by * cs,
                                         (by + 1) * cs - 1);
                    out.pixels[i] = clean(at(in, sx, sy));
                } else
                    out.pixels[i] =
                        shade(cols[ci], std::array<double, 5>{1, .85, .55, .95, .45}[face]);
            }
        return out;
    });
}
Result<Image> oilPaint(ImageView in, const OilPaintOptions &o) {
    if (!detail::inRange(o.stylization, .1, 10) || !detail::inRange(o.cleanliness, 0, 10) ||
        !detail::inRange(o.scale, .1, 10) || !detail::inRange(o.bristleDetail, 0, 10) ||
        !detail::inRange(o.angle, -180, 180) || !detail::inRange(o.shine, 0, 10))
        return bad();
    return process(in, [&] {
        auto out = detail::copy(in);
        auto n = in.pixels.size();
        std::array<std::vector<double>, 4> planes;
        for (auto &p : planes)
            p.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            auto p = in.pixels[i];
            auto c = rgb(p);
            for (int ch = 0; ch < 3; ++ch)
                planes[ch][i] = c[ch] * p.a;
            planes[3][i] = p.a;
        }
        for (auto &p : planes)
            p = develop_detail::gaussianRaw(p, in.width, in.height, o.cleanliness * .25);
        std::vector<double> luminance(n);
        for (std::size_t i = 0; i < n; ++i)
            luminance[i] = (planes[0][i] + planes[1][i] + planes[2][i]) / 3;
        auto pa = [&](const std::vector<double> &p, std::int64_t x, std::int64_t y) {
            x = std::clamp<std::int64_t>(x, 0, in.width - 1);
            y = std::clamp<std::int64_t>(y, 0, in.height - 1);
            return p[std::size_t(y) * in.width + x];
        };
        std::array<std::vector<double>, 3> tensor;
        for (auto &p : tensor)
            p.resize(n);
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto l = [&](int dx, int dy) {
                    return pa(luminance, std::int64_t(x) + dx, std::int64_t(y) + dy);
                };
                double gx = l(1, -1) + 2 * l(1, 0) + l(1, 1) - l(-1, -1) - 2 * l(-1, 0) - l(-1, 1),
                       gy = l(-1, 1) + 2 * l(0, 1) + l(1, 1) - l(-1, -1) - 2 * l(0, -1) - l(1, -1);
                auto i = y * in.width + x;
                tensor[0][i] = gx * gx;
                tensor[1][i] = gx * gy;
                tensor[2][i] = gy * gy;
            }
        for (auto &p : tensor)
            p = develop_detail::gaussianRaw(p, in.width, in.height, .8 + o.scale * .6);
        std::vector<std::pair<double, double>> tangent(n);
        for (std::size_t i = 0; i < n; ++i) {
            double e = tensor[0][i], f = tensor[1][i], g = tensor[2][i],
                   l = .5 * (e + g + std::hypot(e - g, 2 * f)), tx = l - e, ty = -f,
                   m = std::hypot(tx, ty);
            tangent[i] = m < 1e-6 || 2 * l - e - g < 1e-6
                             ? std::pair{1 / std::sqrt(2.), 1 / std::sqrt(2.)}
                             : std::pair{tx / m, ty / m};
        }
        auto samplePlane = [&](const std::vector<double> &p, double x, double y) {
            x = std::clamp(x, 0., double(in.width - 1));
            y = std::clamp(y, 0., double(in.height - 1));
            auto x0 = std::int64_t(std::floor(x)), y0 = std::int64_t(std::floor(y));
            double ax = x - x0, ay = y - y0;
            return (pa(p, x0, y0) * (1 - ax) + pa(p, x0 + 1, y0) * ax) * (1 - ay) +
                   (pa(p, x0, y0 + 1) * (1 - ax) + pa(p, x0 + 1, y0 + 1) * ax) * ay;
        };
        std::array<std::vector<double>, 4> cols;
        for (auto &p : cols)
            p.resize(n);
        std::vector<double> bristle(n);
        double len = 1.5 + o.stylization * 2, h = std::max(1., len / 10);
        int steps = int(std::ceil(len / h));
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                std::array<double, 4> acc{};
                double mass = 1, nsum = hash(x, y, 71, 0);
                for (int ch = 0; ch < 4; ++ch)
                    acc[ch] = planes[ch][i];
                for (double dir : std::array<double, 2>{1, -1}) {
                    double qx = x, qy = y, prevx = 0, prevy = 0;
                    for (int k = 1; k <= steps; ++k) {
                        auto txi = std::size_t(std::clamp(std::lround(qx), 0L, long(in.width - 1))),
                             tyi =
                                 std::size_t(std::clamp(std::lround(qy), 0L, long(in.height - 1)));
                        auto [tx, ty] = tangent[tyi * in.width + txi];
                        if (k == 1) {
                            tx *= dir;
                            ty *= dir;
                        } else if (tx * prevx + ty * prevy < 0) {
                            tx = -tx;
                            ty = -ty;
                        }
                        prevx = tx;
                        prevy = ty;
                        qx += tx * h;
                        qy += ty * h;
                        double weight = 1 - double(k) / (steps + 1);
                        for (int ch = 0; ch < 4; ++ch)
                            acc[ch] += samplePlane(planes[ch], qx, qy) * weight;
                        mass += weight;
                        nsum += weight * hash(std::lround(qx), std::lround(qy), 71, 0);
                    }
                }
                for (int ch = 0; ch < 4; ++ch)
                    cols[ch][i] = acc[ch] / mass;
                bristle[i] = nsum / mass - .5;
            }
        std::vector<double> relief(n);
        for (std::size_t i = 0; i < n; ++i)
            relief[i] = (cols[0][i] + cols[1][i] + cols[2][i]) / 3 * 2 +
                        bristle[i] * (o.bristleDetail / 10) * 6;
        double a = radians(o.angle);
        Color light{std::cos(a) / std::sqrt(2.), -std::sin(a) / std::sqrt(2.), 1 / std::sqrt(2.)},
            half = norm({light[0], light[1], light[2] + 1});
        double flat = std::pow(half[2], 40);
        for (std::size_t y = 0; y < in.height; ++y)
            for (std::size_t x = 0; x < in.width; ++x) {
                auto i = y * in.width + x;
                auto original = in.pixels[i];
                if (original.a <= 0)
                    continue;
                Color c{};
                for (int ch = 0; ch < 3; ++ch)
                    c[ch] = cols[3][i] > 0 ? cols[ch][i] / cols[3][i] : rgb(original)[ch];
                if (o.lighting) {
                    double hx = pa(relief, std::int64_t(x) + 1, y) -
                                pa(relief, std::int64_t(x) - 1, y),
                           hy = pa(relief, x, std::int64_t(y) + 1) -
                                pa(relief, x, std::int64_t(y) - 1);
                    auto nv = norm({-.5 * hx, -.5 * hy, 1});
                    double shade = 1 + (dot(nv, light) - light[2]) * 1.2,
                           spec = o.shine / 10 *
                                  std::max(0., std::pow(std::max(0., dot(nv, half)), 40) - flat) *
                                  .8;
                    for (auto &v : c)
                        v = v * shade + spec;
                }
                out.pixels[i] = preserve(c, original);
            }
        return out;
    });
}
} // namespace iiGraphicAdjust
