#include <iiGraphicAdjust/objects/OwnedResourceObjects.h>
#include "Internal.h"

namespace iiGraphicAdjust::objects {
Result<Image> LensBlur::apply(ImageView image) const {
    const auto &p=options;
    LensBlurOptions o;
    o.radius=p.radius; o.blades=p.blades; o.curvature=p.curvature; o.rotation=p.rotation;
    o.depth=p.depth; o.focalDistance=p.focalDistance; o.invertDepth=p.invertDepth;
    o.brightness=p.brightness; o.threshold=p.threshold; o.noise=p.noise;
    o.distribution=p.distribution; o.monochromatic=p.monochromatic; o.seed=p.seed;
    if (p.depthMap) o.depthMap=p.depthMap->view();
    return lensBlur(image,o);
}
namespace {
VanishingPointOptions borrowed(const VanishingPointParameters &p) {
    VanishingPointOptions o;
    o.planes=p.planes; o.tearOffs=p.tearOffs; o.focalLength=p.focalLength; o.clones=p.clones;
    o.pastes.reserve(p.pastes.size());
    for(const auto &paste:p.pastes)
        o.pastes.push_back({paste.image.view(),paste.plane,paste.at,paste.width,paste.opacity});
    return o;
}
template<class F> auto guarded(F &&function) -> decltype(function()) {
    try { return function(); }
    catch(const std::bad_alloc &) {
        return std::unexpected(Error{ErrorCode::AllocationFailure,"Could not allocate owned-resource call storage"});
    } catch(const std::length_error &) {
        return std::unexpected(Error{ErrorCode::AllocationFailure,"Owned-resource storage exceeds limits"});
    }
}
}
Result<Image> VanishingPoint::apply(ImageView image) const {
    return guarded([&]{return vanishingPoint(image,borrowed(options));});
}
Result<std::vector<VanishingPlane>> VanishingPoint::planes(ImageView image) const {
    return guarded([&]{return vanishingPointPlanes(image,borrowed(options));});
}
Result<Image> ApplyLocalAdjustments::apply(ImageView image) const {
    if(adjustments.size()>32)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"At most 32 local adjustments are supported"});
    std::array<iiGraphicAdjust::LocalAdjustment,32> views{};
    for(std::size_t i=0;i<adjustments.size();++i)
        views[i]={adjustments[i].mask.view(),adjustments[i].options};
    return applyLocalAdjustments(image,std::span{views.data(),adjustments.size()});
}
} // namespace iiGraphicAdjust::objects
