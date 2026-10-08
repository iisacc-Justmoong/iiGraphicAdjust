#pragma once
#include <array>
#include <iiGraphicAdjust/DevelopColor.h>
#include <iiGraphicAdjust/Masks.h>

namespace iiGraphicAdjust {
struct MaskOverlayOptions final {
    std::array<double, 3> color{1, 0, 0}; // Normalized encoded sRGB.
    double opacity = 50;                  // [0,100].
};
enum class DiagnosticColorSpace { Srgb, DisplayP3, AdobeRgb, ProPhoto, Rec2020 };
struct GamutWarningOptions final {
    DiagnosticColorSpace proofSpace = DiagnosticColorSpace::Srgb;
    // Optional pre-gamut scene-linear Rec.2020 RGB, one triple per ImageView pixel, [-64,64].
    // Empty uses the decoded sRGB input, whose already-clipped colors cannot recover RAW gamut.
    std::span<const std::array<double, 3>> linearRec2020;
};
// The host supplies the photo before the selected Point Color sample's own adjustment.
// Adjustment sliders are validated but only the sample/range fields affect the preview.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
pointColorRangePreview(ImageView, const PointColorOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
maskColorOverlay(ImageView, MaskView, const MaskOverlayOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
maskColorOnBwOverlay(ImageView, MaskView, const MaskOverlayOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> maskImageOnBlack(ImageView, MaskView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> maskImageOnWhite(ImageView, MaskView);
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> maskWhiteOnBlack(ImageView, MaskView);
// Warning-only views retain the supplied display pixel where no warning applies.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
destinationGamutWarning(ImageView, const GamutWarningOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
displayGamutWarning(ImageView, const GamutWarningOptions & = {});
} // namespace iiGraphicAdjust
