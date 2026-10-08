#pragma once
#include <iiGraphicAdjust/Image.h>
#include <array>

namespace iiGraphicAdjust {

// HSL hue sectors centered at 0, 60, 120, 180, 240 and 300 degrees.
enum class HueRange { Master, Reds, Yellows, Greens, Cyans, Blues, Magentas };
struct HueSaturationOptions final {
    double hue = 0;        // Hue rotation in degrees, [-180,180].
    double saturation = 0; // Signed interpolation toward 0/1 saturation, [-1,1].
    double lightness = 0;  // Signed interpolation toward black/white, [-1,1].
    HueRange range = HueRange::Master;
    double falloff = 30;   // Smooth transition outside the sector's 30-degree core, [0,60].
    bool colorize = false; // Sets hue (wrapped into [0,360)) and absolute saturation [0,1].
};
struct VibranceOptions final {
    double vibrance = 0;   // HSL delta v*s*(1-s), [-1,1]; gray remains gray.
    double saturation = 0; // Global signed saturation interpolation, [-1,1].
};
struct ToneBalance final {
    double cyanRed = 0, magentaGreen = 0, yellowBlue = 0; // Each [-1,1].
};
struct ColorBalanceOptions final {
    ToneBalance shadows, midtones, highlights;
    bool preserveLuminosity = true;
};
struct BlackWhiteOptions final {
    // Grayscale = minimum RGB + chroma * interpolated hue coefficient.
    std::array<double,6> weights{.4,.6,.4,.6,.2,.8}; // R,Y,G,C,B,M, each [-2,3].
    bool tint = false;
    double tintHue = 40;         // [0,360] degrees.
    double tintSaturation = .2;  // [0,1].
};
struct PhotoFilterOptions final {
    std::array<double,3> color{1,.5,0}; // Encoded sRGB filter transmittance, each [0,1].
    double density = .25;              // [0,1].
    bool preserveLuminosity = true;
};
struct ChannelMix final {
    std::array<double,3> coefficients{1,0,0}; // Each [-2,2].
    double constant = 0;                    // [-1,1].
};
struct ChannelMixerOptions final {
    ChannelMix red{{1,0,0}}, green{{0,1,0}}, blue{{0,0,1}};
    bool monochrome = false;
    ChannelMix gray{{.4,.4,.2}};
};
enum class ColorRange { Reds, Yellows, Greens, Cyans, Blues, Magentas, Whites, Neutrals, Blacks };
enum class SelectiveColorMethod { Relative, Absolute };
struct SelectiveColorAdjustment final {
    double cyan = 0, magenta = 0, yellow = 0, black = 0; // Each [-1,1].
};
struct SelectiveColorOptions final {
    std::array<SelectiveColorAdjustment,9> ranges{}; // Indexed by ColorRange.
    SelectiveColorMethod method = SelectiveColorMethod::Relative;
};
struct ReplaceColorOptions final {
    std::array<double,3> target{.5,.5,.5}; // Each [0,1].
    double fuzziness = .2; // Normalized encoded-RGB Euclidean distance, [0,1].
    double softness = 1;  // Fraction of the selection radius occupied by falloff, [0,1].
    double hue = 0, saturation = 0, lightness = 0; // Same offsets as HueSaturationOptions.
};
struct MatchColorOptions final {
    double luminance = 1;      // Linear-RGB luminance multiplier, [0,2].
    double colorIntensity = 1; // Linear-RGB chroma multiplier, [0,2].
    double fade = 0;           // Encoded-RGB interpolation back to the source, [0,1].
    bool neutralize = false;  // Targets equal reference-mean luminance in all three channels.
};

// Every operation produces detached storage and preserves alpha and hidden RGB at alpha zero.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> hueSaturation(ImageView, const HueSaturationOptions& = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> vibrance(ImageView, const VibranceOptions& = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> colorBalance(ImageView, const ColorBalanceOptions& = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> blackAndWhite(ImageView, const BlackWhiteOptions& = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> photoFilter(ImageView, const PhotoFilterOptions& = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> channelMixer(ImageView, const ChannelMixerOptions& = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> selectiveColor(ImageView, const SelectiveColorOptions& = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> desaturate(ImageView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> replaceColor(ImageView, const ReplaceColorOptions& = {});
// Reference dimensions may differ. Both images must contain positive-alpha pixels.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> matchColor(ImageView source, ImageView reference,
                                                          const MatchColorOptions& = {});
} // namespace iiGraphicAdjust
