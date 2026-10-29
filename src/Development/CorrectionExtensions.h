#pragma once
#include <iiGraphicAdjust/Mapping.h>
#include <iiGraphicAdjust/LocalCorrections.h>

namespace iiGraphicAdjust {
struct ColorToAlphaOptions final {
    std::array<double,3> color{1,1,1}; // Encoded sRGB key.
    double transparencyThreshold=0, opacityThreshold=1; // Independently [0,1].
};
// Encoded-space minimum-opacity color unmixing, not an AI background selection.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
colorToAlpha(ImageView, const ColorToAlphaOptions & = {});

struct FlatFieldOptions final {
    double amount=1; // [0,1]. Correction gain interpolated in linear light.
    double minimum=1e-4; // [1e-8,1], floor for the linear reference response.
    bool perChannel=true;
};
// Caller supplies a decoded, equally sized flat exposure. Visible source pixels
// require visible reference coverage. This is rendered RGB calibration, not RAW decoding.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
flatFieldCorrection(ImageView, ImageView reference, const FlatFieldOptions & = {});

struct SolidFillOptions final {
    Rgba color{0,0,0,1};
    bool preserveAlpha=true; // Multiply fill alpha by input alpha.
};
enum class GradientFillStyle { Linear, Radial, Angle, Reflected, Diamond };
struct GradientFillOptions final {
    std::vector<GradientStop> stops{{0,{0,0,0}},{1,{1,1,1}}};
    GradientFillStyle style=GradientFillStyle::Linear;
    double centerX=.5, centerY=.5, angle=0, scale=1; // Centers [-2,3], angle [-360,360], scale [.001,100].
    bool reverse=false, preserveAlpha=true;
    CorrectionSpace space=CorrectionSpace::LinearSrgb;
};
struct PatternFillOptions final {
    double scale=1, angle=0, offsetX=0, offsetY=0; // Scale [.01,100], degrees [-360,360], pixels [-1e9,1e9].
    bool preserveAlpha=true;
};
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> solidFill(ImageView, const SolidFillOptions & = {});
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> gradientFill(ImageView, const GradientFillOptions & = {});
// Encoded, premultiplied bilinear wrapping of an owned/caller-supplied pattern.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image>
patternFill(ImageView, ImageView pattern, const PatternFillOptions & = {});
} // namespace iiGraphicAdjust
