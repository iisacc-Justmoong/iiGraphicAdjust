#pragma once
#include <array>
#include <iiGraphicAdjust/Image.h>
namespace iiGraphicAdjust {
enum class FilterEdge { Wrap, Repeat, Transparent };
enum class SpherizeMode { Normal, HorizontalOnly, VerticalOnly };
enum class WaveType { Sine, Triangle, Square };
enum class RippleSize { Small, Medium, Large };
enum class PolarMode { RectangularToPolar, PolarToRectangular };
enum class ZigZagStyle { AroundCenter, OutFromCenter, PondRipples };
struct SpherizeOptions final {
    double amount = 100;
    SpherizeMode mode = SpherizeMode::Normal;
};
struct WaveOptions final {
    unsigned generators = 5;
    double wavelengthMin = 10, wavelengthMax = 120, amplitudeMin = 5, amplitudeMax = 35;
    WaveType type = WaveType::Sine;
    FilterEdge edge = FilterEdge::Wrap;
    std::uint32_t seed = 0;
};
struct RippleOptions final {
    double amount = 100;
    RippleSize size = RippleSize::Medium;
};
struct DisplaceOptions final {
    double horizontal = 10, vertical = 10;
    bool stretch = true;
    FilterEdge edge = FilterEdge::Repeat;
};
struct ShearPoint final {
    double position = 0, offset = 0;
};
struct ZigZagOptions final {
    double amount = 10, ridges = 5;
    ZigZagStyle style = ZigZagStyle::PondRipples;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
offset(ImageView, std::int32_t horizontal, std::int32_t vertical, FilterEdge = FilterEdge::Wrap);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> twirl(ImageView, double angle = 50);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> pinch(ImageView, double amount = 50);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> spherize(ImageView,
                                                            const SpherizeOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> wave(ImageView, const WaveOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> ripple(ImageView, const RippleOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
    polarCoordinates(ImageView, PolarMode = PolarMode::RectangularToPolar);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> displace(ImageView, ImageView map,
                                                            const DisplaceOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> shear(ImageView, std::span<const ShearPoint>,
                                                         FilterEdge = FilterEdge::Wrap);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> zigZag(ImageView, const ZigZagOptions & = {});
} // namespace iiGraphicAdjust
