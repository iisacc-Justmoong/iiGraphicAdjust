#include <iiGraphicAdjust/SampleFilters.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class T> iiGraphicAdjust::SamplePlaneView<T> plane(
    const std::vector<T>& values, std::uint32_t w, std::uint32_t h, std::size_t stride) {
    return {values, w, h, stride};
}
template<class T> iiGraphicAdjust::MutableSamplePlaneView<T> writable(
    std::vector<T>& values, std::uint32_t w, std::uint32_t h, std::size_t stride) {
    return {values, w, h, stride};
}
void average8() {
    using namespace iiGraphicAdjust;
    const std::vector<std::uint8_t> values{0, 1, 2, 255};
    auto result = sampleAverage8(plane(values, 4, 1, 4));
    check(result && result->value == 65 && result->selectedSamples == 4 && result->tiles == 1,
          "byte average uses half-up rounding");
    constexpr std::uint32_t w = 258, h = 257;
    constexpr std::size_t stride = 263;
    std::vector<std::uint8_t> pixels(stride * h, 255);
    std::uint64_t sum = 0;
    for (std::uint32_t y = 0; y < h; ++y) for (std::uint32_t x = 0; x < w; ++x) {
        pixels[y * stride + x] = std::uint8_t((x * 17 + y * 13) % 251);
        sum += pixels[y * stride + x];
    }
    const auto before = pixels;
    result = sampleAverage8(plane(pixels, w, h, stride));
    const auto count = std::uint64_t(w) * h;
    check(result && result->value == std::uint8_t(double(sum) / double(count) + .5)
                 && result->selectedSamples == count && result->tiles == 4,
          "byte average excludes padding at both tile boundaries");
    check(pixels == before, "average does not modify input");
}
void average16() {
    using namespace iiGraphicAdjust;
    std::vector<std::uint16_t> maximum(256 * 256, 65535);
    auto result = sampleAverage16(plane(maximum, 256, 256, 256));
    check(result && result->value == 65535 && result->selectedSamples == 65536,
          "maximum word tile sum remains unsigned32");
    const std::vector<std::uint16_t> half{32768, 32769, 65535, 65535};
    result = sampleAverage16(plane(half, 2, 1, 4));
    check(result && result->value == 32769, "word average rounds half up and excludes padding");
}
void floating() {
    using namespace iiGraphicAdjust;
    const std::vector<float> cancellation{1e8F, 1.F, -1e8F, 1.F};
    auto result = sampleAverageFloat32(plane(cancellation, 4, 1, 4));
    check(result && std::bit_cast<std::uint32_t>(result->value) ==
                        std::bit_cast<std::uint32_t>(.25F),
          "float path rounds each addition, not a double precision global mean");
    std::vector<float> tiles(257, 0.F);
    tiles[0] = 1e8F; tiles[1] = 1.F; tiles[2] = -1e8F; tiles[256] = 1.F;
    result = sampleAverageFloat32(plane(tiles, 257, 1, 257));
    check(result && result->tiles == 2 && result->value == float(1. / 257.),
          "float accumulator is reset at256sample tile boundary");
    const std::vector<float> hdr{-2.F, 4.F};
    result = sampleAverageFloat32(plane(hdr, 2, 1, 2));
    check(result && result->value == 1.F, "float HDR input is unclamped");
    const std::vector<float> infinity{std::numeric_limits<float>::infinity(),
                                     -std::numeric_limits<float>::infinity()};
    result = sampleAverageFloat32(plane(infinity, 2, 1, 2));
    check(result && std::isnan(result->value), "opposite infinities preserve NaN classification");
    const std::vector<float> negativeZero{-0.F};
    result = sampleAverageFloat32(plane(negativeZero, 1, 1, 1));
    check(result && !std::signbit(result->value), "initial positive zero follows native accumulation");
}
void selection() {
    using namespace iiGraphicAdjust;
    const std::vector<std::uint8_t> values{1, 2, 3, 20, 40, 60};
    std::vector<std::uint8_t> mask{0, 1, 127, 128, 254, 255};
    auto result = sampleAverage8(plane(values, 6, 1, 6), SampleSelectionMask{mask, 6, 1, 6});
    check(result && result->selectedSamples == 3 && result->value == 40,
          "mask high bit selects equally, not alpha weighted");
    std::fill(mask.begin(), mask.end(), 0);
    result = sampleAverage8(plane(values, 6, 1, 6), SampleSelectionMask{mask, 6, 1, 6});
    check(result && result->value == 0 && result->selectedSamples == 0,
          "empty integer selection returns zero");
    const std::vector<float> floats(6, 1.F);
    auto f = sampleAverageFloat32(plane(floats, 6, 1, 6), SampleSelectionMask{mask, 6, 1, 6});
    check(f && std::isnan(f->value) && f->selectedSamples == 0,
          "empty floating selection returns NaN");
}
void fill() {
    using namespace iiGraphicAdjust;
    const std::vector<std::uint8_t> values{0, 100, 255, 255, 200, 100};
    std::vector<std::uint8_t> output(8, 17);
    const std::vector<std::uint8_t> mask{0, 128, 0, 0, 128, 0};
    auto result = fillSampleAverage8(plane(values, 2, 2, 4), writable(output, 2, 2, 4),
                                    SampleSelectionMask{mask, 2, 2, 4});
    check(result && result->value == 150 && result->selectedSamples == 2,
          "fill computes selected average");
    check(output == std::vector<std::uint8_t>{150, 150, 17, 17, 150, 150, 17, 17},
          "fill writes all active output, preserves padding");
    std::vector<std::uint16_t> words{10, 20, 30, 40};
    auto wordsResult = fillSampleAverage16(plane(words, 2, 2, 2), writable(words, 2, 2, 2));
    check(wordsResult && words == std::vector<std::uint16_t>(4, 25),
          "exact input/output alias averages before writing");
    const std::vector<float> input{-1.F, 3.F};
    std::vector<float> floats(3, 9.F);
    auto floatResult = fillSampleAverageFloat32(plane(input, 2, 1, 2), writable(floats, 2, 1, 3));
    check(floatResult && floats == std::vector<float>{1.F, 1.F, 9.F},
          "float fill retains unused storage");
}
void solarize() {
    using namespace iiGraphicAdjust;
    std::vector<std::uint8_t> bytes(258, 31);
    for (unsigned x = 0; x < 256; ++x) bytes[x] = std::uint8_t(x);
    check(bool(solarizeSamples8(writable(bytes, 256, 1, 258))), "byte solarize succeeds");
    for (unsigned x = 0; x < 256; ++x)
        check(bytes[x] == (x < 128 ? x : 255 - x), "all256byte inputs match exact kernel");
    check(bytes[256] == 31 && bytes[257] == 31, "solarize byte padding untouched");
    constexpr std::size_t stride = 259;
    std::vector<std::uint16_t> words(stride * 256, 1234);
    for (unsigned x = 0; x < 65536; ++x) words[(x / 256) * stride + x % 256] = std::uint16_t(x);
    check(bool(solarizeSamples16(writable(words, 256, 256, stride))), "word solarize succeeds");
    for (unsigned x = 0; x < 65536; ++x) {
        const auto expected = std::uint16_t(x < 16384 ? x : 32768U - x);
        check(words[(x / 256) * stride + x % 256] == expected,
              "all65536word inputs preserve native scale and wrapping");
    }
    for (unsigned y = 0; y < 256; ++y) for (unsigned x = 256; x < stride; ++x)
        check(words[y * stride + x] == 1234, "solarize word padding untouched");
}
void validation() {
    using namespace iiGraphicAdjust;
    std::vector<std::uint8_t> values(10, 200);
    const auto before = values;
    auto r = sampleAverage8(plane(values, 0, 1, 1));
    check(!r && r.error() == SampleFilterError::InvalidGeometry, "zero geometry rejected");
    r = sampleAverage8(plane(values, 4, 1, 3));
    check(!r && r.error() == SampleFilterError::InvalidStride, "short stride rejected");
    r = sampleAverage8(plane(values, 4, 2, 7));
    check(!r && r.error() == SampleFilterError::InsufficientStorage, "last row extent checked");
    r = sampleAverage8(plane(values, 1, 2, std::numeric_limits<std::size_t>::max()));
    check(!r && r.error() == SampleFilterError::InsufficientStorage, "stride overflow rejected");
    r = sampleAverage8(plane(values, std::numeric_limits<std::uint32_t>::max(), 1,
                            std::numeric_limits<std::uint32_t>::max()));
    check(!r && r.error() == SampleFilterError::InvalidGeometry, "signed host extent bounded");
    const SampleSelectionMask mask{values, 1, 1, 1};
    r = sampleAverage8(plane(values, 2, 1, 2), mask);
    check(!r && r.error() == SampleFilterError::MaskGeometryMismatch, "mask geometry checked");
    r = fillSampleAverage8(plane(values, 2, 1, 2), writable(values, 1, 1, 1));
    check(!r && r.error() == SampleFilterError::OutputGeometryMismatch && values == before,
          "invalid output rejected before mutation");
    auto s = solarizeSamples8(writable(values, 4, 2, 7));
    check(!s && values == before, "invalid solarize storage unchanged");
    std::vector<std::uint8_t> wide(32768, 200);
    s = solarizeSamples8(writable(wide, 32768, 1, 32768));
    check(!s && s.error() == SampleFilterError::UnsupportedSolarizeRowWidth
                && std::all_of(wide.begin(), wide.end(), [](auto x) { return x == 200; }),
          "unsupported native row width rejected before writing");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("one test case is required");
        const std::string_view test = argv[1];
        if (test == "average8") average8();
        else if (test == "average16") average16();
        else if (test == "float") floating();
        else if (test == "selection") selection();
        else if (test == "fill") fill();
        else if (test == "solarize") solarize();
        else if (test == "validation") validation();
        else throw std::runtime_error("unknown test case");
        std::cout << "Sample filter contract passed: " << test << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
