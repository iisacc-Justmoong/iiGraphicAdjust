#pragma once
#include <iiGraphicAdjust/Operation.h>
#include <iiGraphicAdjust/SampleFilters.h>

namespace iiGraphicAdjust::objects {
// These operate on explicitly borrowed sample planes during a call. They retain
// the recovered integer/float contract and never convert through normalized RGBA.
struct SampleAverage8 final {
    inline static constexpr OperationInfo info{"sample.sampleAverage8","SampleAverage8","sample"};
    using sample_type=std::uint8_t;
    [[nodiscard]] auto analyze(SamplePlaneView<sample_type> input,
                              std::optional<SampleSelectionMask> mask={}) const {
        return sampleAverage8(input,mask);
    }
};
struct SampleAverage16 final {
    inline static constexpr OperationInfo info{"sample.sampleAverage16","SampleAverage16","sample"};
    using sample_type=std::uint16_t;
    [[nodiscard]] auto analyze(SamplePlaneView<sample_type> input,
                              std::optional<SampleSelectionMask> mask={}) const {
        return sampleAverage16(input,mask);
    }
};
struct SampleAverageFloat32 final {
    inline static constexpr OperationInfo info{"sample.sampleAverageFloat32","SampleAverageFloat32","sample"};
    using sample_type=float;
    [[nodiscard]] auto analyze(SamplePlaneView<sample_type> input,
                              std::optional<SampleSelectionMask> mask={}) const {
        return sampleAverageFloat32(input,mask);
    }
};
struct FillSampleAverage8 final {
    inline static constexpr OperationInfo info{"sample.fillSampleAverage8","FillSampleAverage8","sample"};
    using sample_type=std::uint8_t;
    [[nodiscard]] auto apply(SamplePlaneView<sample_type> input,MutableSamplePlaneView<sample_type> output,
                            std::optional<SampleSelectionMask> mask={}) const {
        return fillSampleAverage8(input,output,mask);
    }
};
struct FillSampleAverage16 final {
    inline static constexpr OperationInfo info{"sample.fillSampleAverage16","FillSampleAverage16","sample"};
    using sample_type=std::uint16_t;
    [[nodiscard]] auto apply(SamplePlaneView<sample_type> input,MutableSamplePlaneView<sample_type> output,
                            std::optional<SampleSelectionMask> mask={}) const {
        return fillSampleAverage16(input,output,mask);
    }
};
struct FillSampleAverageFloat32 final {
    inline static constexpr OperationInfo info{"sample.fillSampleAverageFloat32","FillSampleAverageFloat32","sample"};
    using sample_type=float;
    [[nodiscard]] auto apply(SamplePlaneView<sample_type> input,MutableSamplePlaneView<sample_type> output,
                            std::optional<SampleSelectionMask> mask={}) const {
        return fillSampleAverageFloat32(input,output,mask);
    }
};
struct SolarizeSamples8 final {
    inline static constexpr OperationInfo info{"sample.solarizeSamples8","SolarizeSamples8","sample"};
    using sample_type=std::uint8_t;
    [[nodiscard]] auto apply(MutableSamplePlaneView<sample_type> input) const { return solarizeSamples8(input); }
};
struct SolarizeSamples16 final {
    inline static constexpr OperationInfo info{"sample.solarizeSamples16","SolarizeSamples16","sample"};
    using sample_type=std::uint16_t;
    [[nodiscard]] auto apply(MutableSamplePlaneView<sample_type> input) const { return solarizeSamples16(input); }
};
} // namespace iiGraphicAdjust::objects
