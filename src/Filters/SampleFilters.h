#pragma once

#include <iiGraphicAdjust/Export.h>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>

namespace iiGraphicAdjust {

enum class SampleFilterError {
    InvalidGeometry,
    InvalidStride,
    InsufficientStorage,
    MaskGeometryMismatch,
    OutputGeometryMismatch,
    UnsupportedSolarizeRowWidth,
};

// Width counts active samples, not bytes. Average accepts one planar channel;
// Solarize also accepts a contiguous interleaved row whose width is pixels*channels.
// Padding is caller-owned and is neither selected nor written.
template<class T> struct SamplePlaneView final {
    std::span<const T> samples;
    std::uint32_t width = 0, height = 0;
    std::size_t rowStrideSamples = 0;
};
template<class T> struct MutableSamplePlaneView final {
    std::span<T> samples;
    std::uint32_t width = 0, height = 0;
    std::size_t rowStrideSamples = 0;
};
using SampleSelectionMask = SamplePlaneView<std::uint8_t>;
template<class T> struct SampleAverageResult final {
    T value{};
    std::uint64_t selectedSamples = 0, tiles = 0;
};

// Evidence-bound Photoshop Average kernel: 256x256 tiles, mask >=128 selection,
// float32 addition within a tile and float64 addition between completed tiles.
// Integer rounding is half-up; empty selection returns integer zero / float NaN.
[[nodiscard]] IIGRAPHICADJUST_EXPORT
std::expected<SampleAverageResult<std::uint8_t>, SampleFilterError>
sampleAverage8(SamplePlaneView<std::uint8_t>, std::optional<SampleSelectionMask> = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT
std::expected<SampleAverageResult<std::uint16_t>, SampleFilterError>
sampleAverage16(SamplePlaneView<std::uint16_t>, std::optional<SampleSelectionMask> = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT
std::expected<SampleAverageResult<float>, SampleFilterError>
sampleAverageFloat32(SamplePlaneView<float>, std::optional<SampleSelectionMask> = {});

// Fill the entire active output rectangle with the selected channel's average.
// Selection blending and alpha handling remain the caller's responsibility.
[[nodiscard]] IIGRAPHICADJUST_EXPORT
std::expected<SampleAverageResult<std::uint8_t>, SampleFilterError>
fillSampleAverage8(SamplePlaneView<std::uint8_t>, MutableSamplePlaneView<std::uint8_t>,
                   std::optional<SampleSelectionMask> = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT
std::expected<SampleAverageResult<std::uint16_t>, SampleFilterError>
fillSampleAverage16(SamplePlaneView<std::uint16_t>, MutableSamplePlaneView<std::uint16_t>,
                    std::optional<SampleSelectionMask> = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT
std::expected<SampleAverageResult<float>, SampleFilterError>
fillSampleAverageFloat32(SamplePlaneView<float>, MutableSamplePlaneView<float>,
                         std::optional<SampleSelectionMask> = {});

// Evidence-bound Photoshop Solarize integer kernels, applied in place after
// validation. Byte scale: x>=128 ->255-x. Word scale: x>=16384 ->uint16(32768-x).
// The word kernel intentionally preserves wrapping above32768. No float API.
[[nodiscard]] IIGRAPHICADJUST_EXPORT std::expected<void, SampleFilterError>
solarizeSamples8(MutableSamplePlaneView<std::uint8_t>);
[[nodiscard]] IIGRAPHICADJUST_EXPORT std::expected<void, SampleFilterError>
solarizeSamples16(MutableSamplePlaneView<std::uint16_t>);

} // namespace iiGraphicAdjust
