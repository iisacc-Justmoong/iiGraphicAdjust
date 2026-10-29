#include <iiGraphicAdjust/Adjustments.h>
#include "Internal.h"

namespace iiGraphicAdjust {
Result<Image> adjust(ImageView image,const ColorAdjustment& s) {
    const auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(s.exposure,-16,16) || !detail::inRange(s.brightness,-1,1) ||
        !detail::inRange(s.contrast,-1,1) || !detail::inRange(s.saturation,-1,1) ||
        !detail::inRange(s.gamma,.01,100) || !detail::inRange(s.temperature,-1,1) ||
        !detail::inRange(s.tint,-1,1) || !detail::inRange(s.opacity,0,1))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Color adjustment values are outside documented ranges"});
    return detail::allocated([&] {
        auto result=detail::copy(image);
        const bool useLinear=s.exposure!=0 || s.temperature!=0 || s.tint!=0;
        const double exposure=std::exp2(s.exposure);
        for (auto& p : result.pixels) {
            if (p.a!=0) {
                double r=p.r,g=p.g,b=p.b;
                if (useLinear) {
                    r=detail::encoded(detail::linear(r)*exposure*(1+.25*s.temperature));
                    g=detail::encoded(detail::linear(g)*exposure*(1+.25*s.tint));
                    b=detail::encoded(detail::linear(b)*exposure*(1-.25*s.temperature));
                }
                if (s.brightness!=0 || s.contrast!=0) {
                    r=detail::clamp((r+s.brightness-.5)*(1+s.contrast)+.5);
                    g=detail::clamp((g+s.brightness-.5)*(1+s.contrast)+.5);
                    b=detail::clamp((b+s.brightness-.5)*(1+s.contrast)+.5);
                }
                if (s.saturation!=0) {
                    const double luma=.2126*r+.7152*g+.0722*b;
                    r=detail::clamp(luma+(r-luma)*(1+s.saturation));
                    g=detail::clamp(luma+(g-luma)*(1+s.saturation));
                    b=detail::clamp(luma+(b-luma)*(1+s.saturation));
                }
                if (s.gamma!=1) {
                    r=std::pow(r,1/s.gamma); g=std::pow(g,1/s.gamma); b=std::pow(b,1/s.gamma);
                }
                p.r=static_cast<float>(r); p.g=static_cast<float>(g); p.b=static_cast<float>(b);
            }
            p.a=static_cast<float>(p.a*s.opacity);
        }
        return result;
    });
}
} // namespace iiGraphicAdjust
