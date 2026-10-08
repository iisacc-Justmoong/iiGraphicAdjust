#include <iiGraphicAdjust/Develop.h>

namespace iiGraphicAdjust {
Result<Image> develop(ImageView image, const DevelopOptions &o) {
    auto out = correctOptics(image, o.optics);
    if (!out)
        return out;
    out = whiteBalance(out->view(), o.whiteBalance);
    if (!out)
        return out;
    out = developDetail(out->view(), o.detail);
    if (!out)
        return out;
    out = developEffects(out->view(), o.effects);
    if (!out)
        return out;
    out = calibrate(out->view(), o.calibration);
    if (!out)
        return out;
    out = developLight(out->view(), o.light);
    if (!out)
        return out;
    out = developColor(out->view(), o.color);
    if (!out)
        return out;
    out = postCropVignette(out->view(), o.vignette);
    if (!out)
        return out;
    out = developCurve(out->view(), o.curve);
    if (!out)
        return out;
    return filmGrain(out->view(), o.grain);
}
} // namespace iiGraphicAdjust
