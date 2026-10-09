#include <iiGraphicAdjust/PhotoOperations.h>
#include "Internal.h"

namespace iiGraphicAdjust {
Result<Image> applyOrientation(ImageView source, ExifOrientation orientation) {
    if (auto valid = detail::validate(source); !valid)
        return std::unexpected(valid.error());
    const auto code = static_cast<std::uint8_t>(orientation);
    if (code < 1 || code > 8)
        return std::unexpected(Error{ErrorCode::InvalidParameter, "EXIF orientation must be in [1,8]"});
    return detail::allocated([&] {
        Image result{code < 5 ? source.width : source.height,
                     code < 5 ? source.height : source.width,
                     std::vector<Rgba>(source.pixels.size())};
        for (std::size_t y = 0; y < source.height; ++y) {
            for (std::size_t x = 0; x < source.width; ++x) {
                std::size_t dx = x, dy = y;
                switch (orientation) {
                case ExifOrientation::TopLeft: break;
                case ExifOrientation::TopRight: dx = source.width - 1 - x; break;
                case ExifOrientation::BottomRight:
                    dx = source.width - 1 - x; dy = source.height - 1 - y; break;
                case ExifOrientation::BottomLeft: dy = source.height - 1 - y; break;
                case ExifOrientation::LeftTop: dx = y; dy = x; break;
                case ExifOrientation::RightTop: dx = source.height - 1 - y; dy = x; break;
                case ExifOrientation::RightBottom:
                    dx = source.height - 1 - y; dy = source.width - 1 - x; break;
                case ExifOrientation::LeftBottom: dx = y; dy = source.width - 1 - x; break;
                }
                result.pixels[dy * result.width + dx] = source.pixels[y * source.width + x];
            }
        }
        return result;
    });
}

Result<Image> crop(ImageView source, PixelRect rectangle) {
    if (auto valid = detail::validate(source); !valid)
        return std::unexpected(valid.error());
    // Subtraction after origin checks avoids overflowing x + width / y + height.
    if (!rectangle.width || !rectangle.height || rectangle.x >= source.width || rectangle.y >= source.height ||
        rectangle.width > source.width - rectangle.x || rectangle.height > source.height - rectangle.y)
        return std::unexpected(Error{ErrorCode::InvalidParameter, "Crop rectangle must be nonempty and inside the source"});
    return detail::allocated([&] {
        Image result{rectangle.width, rectangle.height,
                     std::vector<Rgba>(rectangle.width * rectangle.height)};
        for (std::size_t y = 0; y < rectangle.height; ++y)
            std::copy_n(source.pixels.begin() + (rectangle.y + y) * source.width + rectangle.x,
                        rectangle.width, result.pixels.begin() + y * rectangle.width);
        return result;
    });
}

Result<ImageHistogram> imageHistogram(ImageView source, const HistogramOptions& options) {
    if (auto valid = detail::validate(source); !valid)
        return std::unexpected(valid.error());
    if (options.bins < 2 || options.bins > 65536 ||
        (options.alphaPolicy != HistogramAlphaPolicy::CoverageWeighted &&
         options.alphaPolicy != HistogramAlphaPolicy::VisiblePixels &&
         options.alphaPolicy != HistogramAlphaPolicy::AllPixels))
        return std::unexpected(Error{ErrorCode::InvalidParameter, "Histogram bins or alpha policy are invalid"});
    return detail::allocated([&] {
        ImageHistogram result;
        result.pixelCount = source.pixels.size();
        for (auto* channel : {&result.red, &result.green, &result.blue, &result.luminance})
            channel->bins.resize(options.bins);
        auto add = [&](ChannelHistogram& channel, double value, double weight) {
            const auto bin = value >= 1 ? options.bins - 1 : static_cast<std::size_t>(value * options.bins);
            channel.bins[bin] += weight;
            if (result.contributingPixels == 0) {
                channel.minimum = value;
                channel.maximum = value;
            } else {
                channel.minimum = std::min(channel.minimum, value);
                channel.maximum = std::max(channel.maximum, value);
            }
            channel.mean += weight * value;
        };
        for (auto pixel : source.pixels) {
            const double weight = options.alphaPolicy == HistogramAlphaPolicy::CoverageWeighted ? pixel.a :
                                  options.alphaPolicy == HistogramAlphaPolicy::AllPixels ? 1. :
                                  pixel.a > 0 ? 1. : 0.;
            if (weight == 0) continue;
            const double luminance = detail::clamp(.2126 * detail::linear(pixel.r) +
                                                  .7152 * detail::linear(pixel.g) +
                                                  .0722 * detail::linear(pixel.b));
            add(result.red, pixel.r, weight);
            add(result.green, pixel.g, weight);
            add(result.blue, pixel.b, weight);
            add(result.luminance, luminance, weight);
            ++result.contributingPixels;
            result.totalWeight += weight;
        }
        if (result.totalWeight > 0)
            for (auto* channel : {&result.red, &result.green, &result.blue, &result.luminance})
                channel->mean /= result.totalWeight;
        return result;
    });
}
} // namespace iiGraphicAdjust
