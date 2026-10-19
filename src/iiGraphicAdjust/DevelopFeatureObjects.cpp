#include <iiGraphicAdjust/objects/DevelopFeatureObjects.h>
#include "Internal.h"

namespace iiGraphicAdjust::objects {
Result<Image> PointColor::apply(ImageView image) const {
    if(samples.size()>8)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Point Color supports at most eight samples"});
    try {
        DevelopColorOptions options; options.points=samples;
        return developColor(image,options);
    } catch(const std::bad_alloc &) {
        return std::unexpected(Error{ErrorCode::AllocationFailure,"Could not allocate Point Color samples"});
    }
}
} // namespace iiGraphicAdjust::objects
