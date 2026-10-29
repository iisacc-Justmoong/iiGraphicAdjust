#pragma once
#include <iiGraphicAdjust/DevelopColor.h>
#include <iiGraphicAdjust/DevelopEffects.h>
#include <iiGraphicAdjust/DevelopTone.h>
#include <iiGraphicAdjust/Optics.h>

namespace iiGraphicAdjust {
struct DevelopOptions final {
    OpticsOptions optics;
    WhiteBalanceOptions whiteBalance;
    DevelopDetailOptions detail;
    DevelopEffectsOptions effects;
    CalibrationOptions calibration;
    DevelopLightOptions light;
    DevelopColorOptions color;
    PostCropVignetteOptions vignette;
    DevelopCurveOptions curve;
    FilmGrainOptions grain;
};
// Explicit SDR stage composition in field order. Each stage gamut-maps to sRGB.
// A host may call the individual functions in a different order.
[[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> develop(ImageView image,
                                                           const DevelopOptions &options = {});
} // namespace iiGraphicAdjust
