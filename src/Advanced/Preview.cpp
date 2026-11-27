#include <iiGraphicAdjust/Preview.h>
#include "Internal.h"

namespace iiGraphicAdjust::objects {
namespace {
// Free functions bridge settings to existing kernels without storing or calling
// another execution object. Borrowed views never outlive this stack frame.
Result<Image> render(ImageView image,const PuppetWarpOptions& options) { return puppetWarp(image,options); }
Result<Image> render(ImageView image,const PerspectiveWarpOptions& options) { return perspectiveWarp(image,options); }
Result<Image> render(ImageView image,const AdaptiveWideAngleOptions& options) { return adaptiveWideAngle(image,options); }
Result<Image> render(ImageView image,const LensCorrectionOptions& options) { return lensCorrection(image,options); }
Result<Image> render(ImageView image,const LiquifyOptions& options) { return liquify(image,options); }
Result<Image> render(ImageView image,const std::vector<GalleryEffect>& effects) { return filterGallery(image,effects); }
Result<Image> render(ImageView image,const VanishingPointParameters& options) {
    VanishingPointOptions borrowed;
    borrowed.planes=options.planes; borrowed.tearOffs=options.tearOffs;
    borrowed.focalLength=options.focalLength; borrowed.clones=options.clones;
    borrowed.pastes.reserve(options.pastes.size());
    for (const auto& paste:options.pastes)
        borrowed.pastes.push_back({paste.image.view(),paste.plane,paste.at,paste.width,paste.opacity});
    return vanishingPoint(image,borrowed);
}
bool same(ImageView a,ImageView b) {
    return a.width==b.width && a.height==b.height &&
        std::equal(a.pixels.begin(),a.pixels.end(),b.pixels.begin(),b.pixels.end());
}
} // namespace

template<class Settings>
Result<PreviewFrame<Settings>> PreviewState<Settings>::execute(
    ImageView image,PreviewAction action,std::optional<Settings> replacement) {
    if (!maxHistory_ || maxHistory_>4096 ||
        (action!=PreviewAction::Refresh && action!=PreviewAction::Replace &&
         action!=PreviewAction::Undo && action!=PreviewAction::Redo) ||
        (action==PreviewAction::Replace)!=replacement.has_value())
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Invalid preview request or history capacity"});
    auto guarded=detail::allocated([&]() -> Result<PreviewFrame<Settings>> {
        auto next=history_;
        auto cursor=cursor_;
        if (action==PreviewAction::Replace) {
            next.erase(next.begin()+static_cast<std::ptrdiff_t>(cursor+1),next.end());
            next.push_back(std::move(*replacement));
            if (next.size()>maxHistory_) next.erase(next.begin());
            cursor=next.size()-1;
        } else if (action==PreviewAction::Undo) {
            if (!cursor) return std::unexpected(Error{ErrorCode::InvalidParameter,"No preview edit to undo"});
            --cursor;
        } else if (action==PreviewAction::Redo) {
            if (cursor+1>=next.size())
                return std::unexpected(Error{ErrorCode::InvalidParameter,"No preview edit to redo"});
            ++cursor;
        }
        const bool changed=action!=PreviewAction::Refresh;
        if (changed && revision_==std::numeric_limits<std::uint64_t>::max())
            return std::unexpected(Error{ErrorCode::InvalidParameter,"Preview revision exhausted"});
        auto imageResult=render(image,next[cursor]);
        if (!imageResult) return std::unexpected(imageResult.error());
        const auto nextRevision=revision_+std::uint64_t(changed);
        PreviewFrame<Settings> result{std::move(*imageResult),next[cursor],nextRevision,
                                      cursor>0,cursor+1<next.size()};
        history_.swap(next); cursor_=cursor; revision_=nextRevision;
        return result;
    });
    if (!guarded) return std::unexpected(guarded.error());
    return std::move(*guarded);
}

Result<PreviewCacheResult> FilterPreviewCache::execute(const PreviewCacheRequest& request) {
    if (!maxEntries_ || maxEntries_>4096 ||
        (request.action!=PreviewCacheAction::Lookup && request.action!=PreviewCacheAction::Store &&
         request.action!=PreviewCacheAction::Invalidate && request.action!=PreviewCacheAction::Clear))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Invalid preview cache request or capacity"});
    if (request.action==PreviewCacheAction::Lookup || request.action==PreviewCacheAction::Store) {
        if (auto valid=detail::validate(request.source);!valid) return std::unexpected(valid.error());
        if (request.action==PreviewCacheAction::Store) {
            if (auto valid=detail::validate(request.result);!valid) return std::unexpected(valid.error());
            if (request.source.width!=request.result.width || request.source.height!=request.result.height)
                return std::unexpected(Error{ErrorCode::SizeMismatch,"Cached preview dimensions must match its source"});
        }
    }
    auto guarded=detail::allocated([&]() -> PreviewCacheResult {
        if (request.action==PreviewCacheAction::Clear) entries_.clear();
        else if (request.action==PreviewCacheAction::Invalidate) {
            std::erase_if(entries_,[&](const Entry& entry) {
                return entry.key.sourceId==request.key.sourceId && entry.key.pipelineId==request.key.pipelineId;
            });
        } else {
            // Allocate all potentially failing copies before changing the LRU.
            std::optional<Entry> incoming;
            if (request.action==PreviewCacheAction::Store) {
                incoming=Entry{request.key,detail::copy(request.source),detail::copy(request.result)};
                entries_.reserve(std::min(maxEntries_,entries_.size()+1));
            }
            auto found=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& entry) {
                return entry.key==request.key && same(entry.source.view(),request.source);
            });
            PreviewCacheResult result;
            if (request.action==PreviewCacheAction::Lookup && found!=entries_.end())
                result={found->result,true};
            std::erase_if(entries_,[&](const Entry& entry) {
                return (entry.key.sourceId==request.key.sourceId &&
                        (entry.key.sourceRevision!=request.key.sourceRevision || !same(entry.source.view(),request.source))) ||
                       (entry.key.pipelineId==request.key.pipelineId &&
                        entry.key.pipelineRevision!=request.key.pipelineRevision);
            });
            found=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& entry) { return entry.key==request.key; });
            if (result.hit) {
                Entry recent=std::move(*found); entries_.erase(found); entries_.push_back(std::move(recent));
                return result;
            }
            if (request.action==PreviewCacheAction::Store) {
                if (found!=entries_.end()) entries_.erase(found);
                if (entries_.size()>=maxEntries_) entries_.erase(entries_.begin());
                entries_.push_back(std::move(*incoming));
            }
        }
        return {};
    });
    if (!guarded) return std::unexpected(guarded.error());
    return std::move(*guarded);
}

Result<Image> Float32PreviewToneMapper::execute(HdrImageView image) const { return hdrToning(image,options); }

template class IIGRAPHICADJUST_EXPORT PreviewState<PuppetWarpOptions>;
template class IIGRAPHICADJUST_EXPORT PreviewState<PerspectiveWarpOptions>;
template class IIGRAPHICADJUST_EXPORT PreviewState<AdaptiveWideAngleOptions>;
template class IIGRAPHICADJUST_EXPORT PreviewState<LensCorrectionOptions>;
template class IIGRAPHICADJUST_EXPORT PreviewState<LiquifyOptions>;
template class IIGRAPHICADJUST_EXPORT PreviewState<VanishingPointParameters>;
template class IIGRAPHICADJUST_EXPORT PreviewState<std::vector<GalleryEffect>>;
} // namespace iiGraphicAdjust::objects
