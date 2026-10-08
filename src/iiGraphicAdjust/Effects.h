#pragma once
#include <iiGraphicAdjust/Image.h>

namespace iiGraphicAdjust {

struct SharpenOptions final {
    double sigma = 1;     // Gaussian standard deviation in pixels, [0,32].
    double amount = 1;    // Unsharp-mask gain, [0,8].
    double threshold = 0; // Maximum-channel difference in linear RGB, [0,1].
};

enum class NoiseDistribution { Uniform, Gaussian };
struct NoiseOptions final {
    double amount = 0; // Uniform half-range or Gaussian standard deviation, [0,1].
    std::uint64_t seed = 1;
    NoiseDistribution distribution = NoiseDistribution::Uniform;
    bool monochromatic = true;
};

// Blur uses linear-light premultiplied RGBA and clamped edge samples.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> gaussianBlur(ImageView image, double sigma);
// Unsharp mask leaves alpha unchanged; noise modifies encoded RGB only.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> sharpen(ImageView image,
                                                        const SharpenOptions& options = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> addNoise(ImageView image,
                                                         const NoiseOptions& options = {});
} // namespace iiGraphicAdjust
