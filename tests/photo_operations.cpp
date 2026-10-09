#include <iiGraphicAdjust/PhotoOperations.h>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <string_view>

using namespace iiGraphicAdjust;
namespace {
int failures = 0;
void check(bool ok, std::string_view message) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool near(double a, double b) { return std::abs(a - b) < 1e-10; }
Rgba label(unsigned n) {
    return {n / 20.F, (n + 1) / 20.F, (n + 2) / 20.F, (n % 3) / 2.F};
}
Image labeled(std::size_t width, std::size_t height) {
    Image result{width, height, {}};
    for (std::size_t n = 1; n <= width * height; ++n)
        result.pixels.push_back(label(static_cast<unsigned>(n)));
    return result;
}
void orientation() {
    const auto source = labeled(3, 2);
    const auto original = source.pixels;
    const std::array<std::array<unsigned, 6>, 8> expected{{
        {1, 2, 3, 4, 5, 6}, {3, 2, 1, 6, 5, 4},
        {6, 5, 4, 3, 2, 1}, {4, 5, 6, 1, 2, 3},
        {1, 4, 2, 5, 3, 6}, {4, 1, 5, 2, 6, 3},
        {6, 3, 5, 2, 4, 1}, {3, 6, 2, 5, 1, 4}
    }};
    for (unsigned code = 1; code <= 8; ++code) {
        auto result = applyOrientation(source.view(), static_cast<ExifOrientation>(code));
        check(result.has_value(), "All eight EXIF orientations succeed");
        if (!result) continue;
        check(result->width == (code < 5 ? 3 : 2) && result->height == (code < 5 ? 2 : 3),
              "Transpose and quarter turns swap rectangular dimensions");
        for (std::size_t n = 0; n < 6; ++n)
            check(result->pixels[n] == label(expected[code - 1][n]),
                  "Orientation matches independently specified labeled rows including hidden RGB");
        result->pixels[0] = {};
        check(source.pixels == original, "Even orientation identity returns detached storage");
    }
    const auto vertical = labeled(1, 6);
    const auto horizontal = applyOrientation(vertical.view(), ExifOrientation::RightTop);
    check(horizontal && horizontal->width == 6 && horizontal->height == 1 &&
          horizontal->pixels.front() == label(6) && horizontal->pixels.back() == label(1),
          "Single-column rotation has correct dimensions and endpoints");
    const auto single = labeled(1, 1);
    for (unsigned code = 1; code <= 8; ++code) {
        const auto result = applyOrientation(single.view(), static_cast<ExifOrientation>(code));
        check(result && result->pixels == single.pixels, "All orientations preserve a single pixel");
    }
}
void cropping() {
    const auto source = labeled(5, 3);
    const auto original = source.pixels;
    auto result = crop(source.view(), {1, 1, 3, 2});
    check(result && result->width == 3 && result->height == 2, "Rectangular crop dimensions are exact");
    constexpr std::array<unsigned, 6> expected{7, 8, 9, 12, 13, 14};
    if (result) {
        for (std::size_t n = 0; n < expected.size(); ++n)
            check(result->pixels[n] == label(expected[n]), "Crop copies exact integer-coordinate pixels");
        result->pixels[0] = {};
    }
    check(source.pixels == original, "Crop owns output storage and leaves input unchanged");
    const auto corner = crop(source.view(), {4, 2, 1, 1});
    check(corner && corner->pixels[0] == label(15), "Bottom-right boundary pixel is included");
    const auto full = crop(source.view(), {0, 0, 5, 3});
    check(full && full->pixels == original, "Full-image crop is lossless");
    constexpr auto max = std::numeric_limits<std::size_t>::max();
    for (auto rectangle : {PixelRect{0, 0, 0, 1}, PixelRect{0, 0, 1, 0},
                           PixelRect{5, 0, 1, 1}, PixelRect{0, 3, 1, 1},
                           PixelRect{4, 2, 2, 1}, PixelRect{1, 1, max, max},
                           PixelRect{max, max, 2, 2}}) {
        const auto bad = crop(source.view(), rectangle);
        check(!bad && bad.error().code == ErrorCode::InvalidParameter,
              "Empty, outside and overflowing crop rectangles fail without unsigned wraparound");
    }
}
void histogram() {
    const Image source{6, 1, {{0, 0, 0, 1}, {1, 1, 1, 1}, {.5F, .5F, .5F, .5F},
                            {1, 0, 0, .5F}, {0, 1, 0, 1}, {0, 0, 1, 0}}};
    const auto original = source.pixels;
    const auto result = imageHistogram(source.view(), {4, HistogramAlphaPolicy::CoverageWeighted});
    check(result && result->pixelCount == 6 && result->contributingPixels == 5 && near(result->totalWeight, 4),
          "Default coverage weighting excludes zero-alpha and counts contributing pixels");
    if (result) {
        check(result->red.bins == std::vector<double>{2, 0, .5, 1.5} &&
              result->green.bins == std::vector<double>{1.5, 0, .5, 2} &&
              result->blue.bins == std::vector<double>{2.5, 0, .5, 1},
              "Encoded RGB bins include exact black and white endpoints and alpha coverage");
        check(near(result->red.mean, .4375) && near(result->green.mean, .5625) && near(result->blue.mean, .3125),
              "RGB channel means use the same weights as the histogram");
        const double linearGray = std::pow((.5 + .055) / 1.055, 2.4);
        check(near(result->luminance.mean, (1 + .5 * linearGray + .5 * .2126 + .7152) / 4),
              "Luminance mean decodes sRGB and uses linear Rec709 coefficients");
        check(result->luminance.bins == std::vector<double>{2, 0, 1, 1},
              "Linear luminance bins differ deliberately from encoded RGB bins");
        for (const auto* channel : {&result->red, &result->green, &result->blue, &result->luminance}) {
            check(near(std::accumulate(channel->bins.begin(), channel->bins.end(), 0.), result->totalWeight),
                  "Every histogram conserves total contributing weight");
            check(channel->minimum == 0 && channel->maximum == 1, "Channel extrema include visible endpoints");
        }
    }
    const auto visible = imageHistogram(source.view(), {4, HistogramAlphaPolicy::VisiblePixels});
    check(visible && visible->contributingPixels == 5 && visible->totalWeight == 5 &&
          visible->red.bins == std::vector<double>{2, 0, 1, 2} && near(visible->red.mean, .5),
          "Visible-pixel policy gives equal weight to partial coverage pixels");
    const auto all = imageHistogram(source.view(), {4, HistogramAlphaPolicy::AllPixels});
    check(all && all->contributingPixels == 6 && all->totalWeight == 6 &&
          all->blue.bins == std::vector<double>{3, 0, 1, 2},
          "All-pixel policy explicitly includes hidden RGB");
    const Image hidden{2, 1, {{1, .3F, .7F, 0}, {.1F, .9F, .4F, 0}}};
    for (auto policy : {HistogramAlphaPolicy::CoverageWeighted, HistogramAlphaPolicy::VisiblePixels}) {
        const auto empty = imageHistogram(hidden.view(), {8, policy});
        check(empty && empty->totalWeight == 0 && empty->contributingPixels == 0 &&
              empty->red.mean == 0 && empty->red.minimum == 0 && empty->red.maximum == 0 &&
              std::accumulate(empty->red.bins.begin(), empty->red.bins.end(), 0.) == 0,
              "Fully transparent image produces defined empty statistics");
    }
    const Image gray{1, 1, {{.5F, .5F, .5F, 1}}};
    const auto grayHistogram = imageHistogram(gray.view());
    check(grayHistogram && grayHistogram->red.bins.size() == 256 &&
          grayHistogram->red.bins[128] == 1 && grayHistogram->luminance.bins[54] == 1 &&
          grayHistogram->red.minimum == .5 && near(grayHistogram->luminance.minimum, .21404114048223255),
          "Default 256 bins and grayscale extrema use documented spaces");
    check(source.pixels == original, "Statistics do not mutate source pixels");
}
void validation() {
    const auto source = labeled(1, 1);
    for (unsigned code : {0u, 9u, 255u}) {
        const auto bad = applyOrientation(source.view(), static_cast<ExifOrientation>(code));
        check(!bad && bad.error().code == ErrorCode::InvalidParameter, "Invalid EXIF orientation codes fail");
    }
    for (std::size_t bins : {std::size_t{0}, std::size_t{1}, std::size_t{65537},
                             std::numeric_limits<std::size_t>::max()}) {
        const auto bad = imageHistogram(source.view(), {bins});
        check(!bad && bad.error().code == ErrorCode::InvalidParameter, "Histogram bin allocation is bounded");
    }
    check(!imageHistogram(source.view(), {2, static_cast<HistogramAlphaPolicy>(99)}),
          "Invalid histogram alpha policy fails");
    auto checkInvalid = [](ImageView image, ErrorCode code) {
        const auto oriented = applyOrientation(image, ExifOrientation::TopLeft);
        const auto cropped = crop(image, {0, 0, 1, 1});
        const auto counted = imageHistogram(image);
        check(!oriented && oriented.error().code == code && !cropped && cropped.error().code == code &&
              !counted && counted.error().code == code, "Every operation rejects invalid images consistently");
    };
    checkInvalid({}, ErrorCode::InvalidDimensions);
    checkInvalid({2, 2, source.pixels}, ErrorCode::InvalidBuffer);
    checkInvalid({std::numeric_limits<std::size_t>::max(), 2, {}}, ErrorCode::InvalidDimensions);
    for (auto bad : {Rgba{std::numeric_limits<float>::quiet_NaN(), 0, 0, 1},
                     Rgba{0, std::numeric_limits<float>::infinity(), 0, 1},
                     Rgba{0, 0, 0, 1.1F}, Rgba{-.1F, 0, 0, 0}}) {
        const Image image{1, 1, {bad}};
        checkInvalid(image.view(), ErrorCode::InvalidPixel);
    }
}
} // namespace
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const std::string_view name = argv[1];
    if (name == "orientation") orientation();
    else if (name == "crop") cropping();
    else if (name == "histogram") histogram();
    else if (name == "validation") validation();
    else return 2;
    return failures ? 1 : 0;
}
