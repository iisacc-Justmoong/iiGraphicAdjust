#pragma once
#include "Internal.h"
#include <array>
#include <numbers>

// Encoded-sRGB filter math, separate from the linear-light development pipeline.
namespace iiGraphicAdjust::filter_detail {
using Pixel = std::array<double, 4>; // premultiplied encoded RGB, alpha
using Buffer = std::vector<Pixel>;
enum class Edge { Repeat, Wrap, Transparent };
constexpr double pi = std::numbers::pi;
inline Error invalid() {
    return {ErrorCode::InvalidParameter, "Invalid filter parameter"};
}
inline double smooth(double low, double high, double value) {
    if (high <= low)
        return value >= high ? 1 : 0;
    auto t = std::clamp((value - low) / (high - low), 0., 1.);
    return t * t * (3 - 2 * t);
}
inline double luma(Rgba p) {
    return .299 * p.r + .587 * p.g + .114 * p.b;
}
inline Pixel premul(Rgba p) {
    return {double(p.r) * p.a, double(p.g) * p.a, double(p.b) * p.a, p.a};
}
inline std::array<float, 3> unpremulRgb(Pixel p) {
    if (p[3] <= 0)
        return {};
    return {float(detail::clamp(p[0] / p[3])), float(detail::clamp(p[1] / p[3])),
            float(detail::clamp(p[2] / p[3]))};
}
inline Rgba straight(Pixel p) {
    const auto alpha = float(detail::clamp(p[3]));
    if (alpha <= 0)
        return {0, 0, 0, 0};
    const auto color = unpremulRgb(p);
    return {color[0], color[1], color[2], alpha};
}
inline Buffer read(ImageView image) {
    Buffer out;
    out.reserve(image.pixels.size());
    for (auto p : image.pixels)
        out.push_back(premul(p));
    return out;
}
inline Image write(ImageView image, const Buffer &buffer) {
    Image out = detail::output(image);
    for (std::size_t i = 0; i < buffer.size(); ++i)
        out.pixels[i] = straight(buffer[i]);
    return out;
}
inline long long wrap(long long value, long long size) {
    auto r = value % size;
    return r < 0 ? r + size : r;
}
inline Rgba get(ImageView image, long long x, long long y, Edge edge = Edge::Repeat) {
    const auto w = static_cast<long long>(image.width), h = static_cast<long long>(image.height);
    if (x < 0 || y < 0 || x >= w || y >= h) {
        if (edge == Edge::Transparent)
            return {0, 0, 0, 0};
        if (edge == Edge::Wrap) {
            x = wrap(x, w);
            y = wrap(y, h);
        } else {
            x = std::clamp(x, 0LL, w - 1);
            y = std::clamp(y, 0LL, h - 1);
        }
    }
    return image.pixels[std::size_t(y) * image.width + std::size_t(x)];
}
inline Pixel sample(ImageView image, double x, double y, Edge edge = Edge::Repeat) {
    // Coordinates are pixel centers at .5, .5; normalize huge offsets before integer casts.
    x -= .5;
    y -= .5;
    if (edge == Edge::Wrap) {
        x = std::fmod(x, double(image.width));
        y = std::fmod(y, double(image.height));
    } else if (edge == Edge::Repeat) {
        x = std::clamp(x, 0., double(image.width - 1));
        y = std::clamp(y, 0., double(image.height - 1));
    } else if (x < -1 || y < -1 || x > double(image.width) || y > double(image.height))
        return {};
    const auto xx = static_cast<long long>(std::floor(x)),
               yy = static_cast<long long>(std::floor(y));
    const double ax = x - std::floor(x), ay = y - std::floor(y);
    Pixel out{};
    for (int dy = 0; dy < 2; ++dy)
        for (int dx = 0; dx < 2; ++dx) {
            auto p = premul(get(image, xx + dx, yy + dy, edge));
            const double k = (dx ? ax : 1 - ax) * (dy ? ay : 1 - ay);
            for (int c = 0; c < 4; ++c)
                out[c] += p[c] * k;
        }
    return out;
}
template <class Map> Image remap(ImageView image, Edge edge, Map map) {
    Image out = detail::output(image);
    for (std::size_t y = 0; y < image.height; ++y)
        for (std::size_t x = 0; x < image.width; ++x) {
            auto [sx, sy] = map(x + .5, y + .5);
            out.pixels[y * image.width + x] = sx == x + .5 && sy == y + .5
                                                  ? image.pixels[y * image.width + x]
                                                  : straight(sample(image, sx, sy, edge));
        }
    return out;
}
inline void boxAxis(Buffer &data, std::size_t w, std::size_t h, std::size_t r, bool horizontal) {
    if (!r)
        return;
    Buffer next(data.size());
    const std::size_t len = horizontal ? w : h, lines = horizontal ? h : w;
    auto at = [&](std::size_t line, std::size_t p) {
        return horizontal ? line * w + p : p * w + line;
    };
    const double norm = 1. / (2. * r + 1);
    for (std::size_t line = 0; line < lines; ++line) {
        Pixel sum{};
        for (int c = 0; c < 4; ++c) {
            sum[c] = data[at(line, 0)][c] * (r + 1.);
            for (std::size_t p = 1; p <= std::min(r, len - 1); ++p)
                sum[c] += data[at(line, p)][c];
            if (r >= len)
                sum[c] += data[at(line, len - 1)][c] * (r - len + 1.);
        }
        for (std::size_t p = 0; p < len; ++p) {
            for (int c = 0; c < 4; ++c)
                next[at(line, p)][c] = sum[c] * norm;
            auto add = p + r + 1 >= len ? len - 1 : p + r + 1;
            auto sub = p >= r ? p - r : 0;
            for (int c = 0; c < 4; ++c)
                sum[c] += data[at(line, add)][c] - data[at(line, sub)][c];
        }
    }
    data.swap(next);
}
inline Buffer boxed(ImageView image, std::size_t radius) {
    auto b = read(image);
    boxAxis(b, image.width, image.height, radius, true);
    boxAxis(b, image.width, image.height, radius, false);
    return b;
}
inline Buffer gauss(ImageView image, double sigma) {
    auto b = read(image);
    if (sigma == 0)
        return b;
    if (sigma > 4) {
        int low = std::max(1, int(std::floor(std::sqrt(4 * sigma * sigma + 1))));
        if (low % 2 == 0)
            --low;
        int m = std::clamp(int(std::round((12 * sigma * sigma - 3. * low * low - 12. * low - 9) /
                                          (-4. * low - 4))),
                           0, 3);
        for (bool hor : {true, false})
            for (int pass = 0; pass < 3; ++pass)
                boxAxis(b, image.width, image.height,
                        std::size_t((low + (pass < m ? 0 : 2) - 1) / 2), hor);
        return b;
    }
    const int r = int(std::ceil(3 * sigma));
    std::vector<double> k(2 * r + 1);
    double norm = 0;
    for (int i = -r; i <= r; ++i) {
        // The central tap is exactly one even when sigma squared underflows to zero.
        k[i + r] = i == 0 ? 1 : std::exp(-double(i * i) / (2 * sigma * sigma));
        norm += k[i + r];
    }
    for (auto &v : k)
        v /= norm;
    for (bool horizontal : {true, false}) {
        Buffer next(b.size());
        for (std::size_t y = 0; y < image.height; ++y)
            for (std::size_t x = 0; x < image.width; ++x)
                for (int j = -r; j <= r; ++j) {
                    auto xx = std::clamp<long long>(
                        static_cast<long long>(x) + (horizontal ? j : 0), 0, image.width - 1);
                    auto yy = std::clamp<long long>(
                        static_cast<long long>(y) + (horizontal ? 0 : j), 0, image.height - 1);
                    for (int c = 0; c < 4; ++c)
                        next[y * image.width + x][c] +=
                            b[std::size_t(yy) * image.width + std::size_t(xx)][c] * k[j + r];
                }
        b.swap(next);
    }
    return b;
}
inline double hash(long long x, long long y, std::uint32_t channel, std::uint32_t seed) {
    std::uint32_t v = std::uint32_t(x) * 0x8da6b343U ^ std::uint32_t(y) * 0xd8163841U ^
                      channel * 0xcb1ab31fU ^ seed * 0x9e3779b9U;
    v ^= v >> 16;
    v *= 0x7feb352dU;
    v ^= v >> 15;
    v *= 0x846ca68bU;
    v ^= v >> 16;
    return double(v >> 8) / 16777216.;
}
inline double noise(long long x, long long y, std::uint32_t c, std::uint32_t seed, bool gaussian) {
    if (!gaussian)
        return 2 * hash(x, y, c, seed) - 1;
    return std::sqrt(-2 * std::log(std::max(1e-7, hash(x, y, c * 2 + 101, seed)))) *
           std::cos(2 * pi * hash(x, y, c * 2 + 102, seed)) * .5;
}
// Validate borrowed input and translate allocation failures at the public boundary.
template <class F> auto guarded(ImageView image, F f) -> Result<Image> {
    if (auto valid = detail::validate(image); !valid)
        return std::unexpected(valid.error());
    return detail::allocated(f);
}
} // namespace iiGraphicAdjust::filter_detail
