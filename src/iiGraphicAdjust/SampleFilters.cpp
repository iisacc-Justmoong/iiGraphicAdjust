// Adapted from the evidence-bound C++23 reconstruction in Adobe/Photoshop.
// Canonical function Evidence IDs and verified sample contracts: docs/SampleFilters.md.
// This translation unit must compile without fast-math or FP contraction.
#include "SampleFilters.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace iiGraphicAdjust {
namespace {

// Bounds checks are the reconstruction's public API contract. They replace
// implicit Photoshop-host preconditions; the original binary has no spans.
template<class View>
std::expected<void, SampleFilterError> validate(View image) {
    if (image.width == 0 || image.height == 0 ||
        image.width > std::numeric_limits<std::int32_t>::max() ||
        image.height > std::numeric_limits<std::int32_t>::max()) {
        return std::unexpected(SampleFilterError::InvalidGeometry);
    }
    if (image.rowStrideSamples < image.width) {
        return std::unexpected(SampleFilterError::InvalidStride);
    }
    const auto remainingRows = std::size_t{image.height - 1};
    if (remainingRows != 0 && image.rowStrideSamples >
        (std::numeric_limits<std::size_t>::max() - image.width) / remainingRows) {
        return std::unexpected(SampleFilterError::InsufficientStorage);
    }
    const auto required = remainingRows * image.rowStrideSamples + image.width;
    if (required > image.samples.size()) {
        return std::unexpected(SampleFilterError::InsufficientStorage);
    }
    return {};
}

template<class T>
std::expected<SampleAverageResult<T>, SampleFilterError> average(
    SamplePlaneView<T> input, std::optional<SampleSelectionMask> mask) {
    if (auto valid = validate(input); !valid) return std::unexpected(valid.error());
    if (mask) {
        if (mask->width != input.width || mask->height != input.height) {
            return std::unexpected(SampleFilterError::MaskGeometryMismatch);
        }
        if (auto valid = validate(*mask); !valid) return std::unexpected(valid.error());
    }

    // _DoStart chooses tiles in top/left order and resets the channel sum at
    // each tile. 0x39c0..0x39e4 converts the tile float sum/count to float64.
    double total = 0.0;
    double countAsDouble = 0.0;
    std::uint64_t count = 0;
    std::uint64_t tiles = 0;
    for (std::uint32_t top = 0; top < input.height; ) {
        const auto bottom = top + std::min<std::uint32_t>(256, input.height - top);
        for (std::uint32_t left = 0; left < input.width; ) {
            const auto right = left + std::min<std::uint32_t>(256, input.width - left);
            std::conditional_t<std::is_same_v<T, float>, float, std::uint32_t> sum{};
            std::uint32_t tileCount = 0;
            for (auto row = top; row < bottom; ++row) {
                const auto rowOffset = std::size_t{row} * input.rowStrideSamples;
                const auto maskOffset = mask ? std::size_t{row} * mask->rowStrideSamples : 0;
                for (auto column = left; column < right; ++column) {
                    // ldrsb + tbz sign bit at 0x3788/0x3870/0x3920.
                    if (!mask || mask->samples[maskOffset + column] >= 128) {
                        sum += input.samples[rowOffset + column];
                        ++tileCount;
                    }
                }
            }
            total += static_cast<double>(sum);
            countAsDouble += static_cast<double>(tileCount);
            count += tileCount;
            ++tiles;
            left = right;
        }
        top = bottom;
    }

    T value{};
    if constexpr (std::is_same_v<T, float>) {
        // fdiv d1 followed by fcvt s1,d1. Zero selections remain NaN.
        value = static_cast<float>(total / countAsDouble);
    } else if (count != 0) {
        // fadd 0.5 + fcvtzu gives half-up rounding for valid unsigned pixels.
        value = static_cast<T>(total / countAsDouble + 0.5);
    }
    // ARM64 FCVTZU maps the zero-selection NaN to zero for integer outputs.
    return SampleAverageResult<T>{value, count, tiles};
}

template<class T>
std::expected<SampleAverageResult<T>, SampleFilterError> fill(
    SamplePlaneView<T> input, MutableSamplePlaneView<T> output, std::optional<SampleSelectionMask> mask) {
    if (input.width != output.width || input.height != output.height) {
        return std::unexpected(SampleFilterError::OutputGeometryMismatch);
    }
    if (auto valid = validate(output); !valid) return std::unexpected(valid.error());
    auto result = average(input, mask);
    if (!result) return result;
    for (std::uint32_t row = 0; row < output.height; ++row) {
        const auto offset = std::size_t{row} * output.rowStrideSamples;
        std::fill_n(output.samples.begin() + offset, output.width, result->value);
    }
    return result;
}

template<class T>
std::expected<void, SampleFilterError> solarize(MutableSamplePlaneView<T> image) {
    if (auto valid = validate(image); !valid) return valid;
    // FUN_00002800 truncates width*channels with SXTH. Only its nonnegative
    // supported row domain is exposed; invalid host layouts are rejected.
    if (image.width > 32767) {
        return std::unexpected(SampleFilterError::UnsupportedSolarizeRowWidth);
    }
    for (std::uint32_t row = 0; row < image.height; ++row) {
        const auto offset = std::size_t{row} * image.rowStrideSamples;
        for (std::uint32_t column = 0; column < image.width; ++column) {
            auto& sample = image.samples[offset + column];
            if constexpr (sizeof(T) == 1) {
                if (sample >= 128) sample = static_cast<T>(255u - sample);
            } else {
                // csel lo at 0x289c; STRH preserves modulo-16-bit subtraction,
                // including raw values outside Photoshop's 0..32768 scale.
                if (sample >= 16384) sample = static_cast<T>(32768u - sample);
            }
        }
    }
    return {};
}

} // namespace

std::expected<SampleAverageResult<std::uint8_t>, SampleFilterError> sampleAverage8(
    SamplePlaneView<std::uint8_t> input, std::optional<SampleSelectionMask> mask) { return average(input, mask); }
std::expected<SampleAverageResult<std::uint16_t>, SampleFilterError> sampleAverage16(
    SamplePlaneView<std::uint16_t> input, std::optional<SampleSelectionMask> mask) { return average(input, mask); }
std::expected<SampleAverageResult<float>, SampleFilterError> sampleAverageFloat32(
    SamplePlaneView<float> input, std::optional<SampleSelectionMask> mask) { return average(input, mask); }
std::expected<SampleAverageResult<std::uint8_t>, SampleFilterError> fillSampleAverage8(
    SamplePlaneView<std::uint8_t> input, MutableSamplePlaneView<std::uint8_t> output,
    std::optional<SampleSelectionMask> mask) { return fill(input, output, mask); }
std::expected<SampleAverageResult<std::uint16_t>, SampleFilterError> fillSampleAverage16(
    SamplePlaneView<std::uint16_t> input, MutableSamplePlaneView<std::uint16_t> output,
    std::optional<SampleSelectionMask> mask) { return fill(input, output, mask); }
std::expected<SampleAverageResult<float>, SampleFilterError> fillSampleAverageFloat32(
    SamplePlaneView<float> input, MutableSamplePlaneView<float> output,
    std::optional<SampleSelectionMask> mask) { return fill(input, output, mask); }
std::expected<void, SampleFilterError> solarizeSamples8(MutableSamplePlaneView<std::uint8_t> image) { return solarize(image); }
std::expected<void, SampleFilterError> solarizeSamples16(MutableSamplePlaneView<std::uint16_t> image) { return solarize(image); }

} // namespace iiGraphicAdjust
