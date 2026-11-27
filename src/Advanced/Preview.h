#pragma once

#include <iiGraphicAdjust/GalleryFilters.h>
#include <iiGraphicAdjust/Hdr.h>
#include <iiGraphicAdjust/LensFilter.h>
#include <iiGraphicAdjust/MeshFilters.h>
#include <iiGraphicAdjust/objects/OwnedResourceObjects.h>
#include <optional>

namespace iiGraphicAdjust {

// Replace starts an undoable edit; Refresh only renders. Undo/Redo move the
// history cursor and fail without modifying state if no step exists.
enum class PreviewAction { Refresh, Replace, Undo, Redo };

// Owns the rendered image and complete settings snapshot. Revision changes on
// every successful edit or history movement, including revisiting old settings.
template<class Settings> struct PreviewFrame final {
    Image image;
    Settings settings;
    std::uint64_t revision = 0;
    bool canUndo = false, canRedo = false;
};

// Cache keys are caller-assigned identities and monotonic content revisions.
// Pipeline identity must be unique for independently created histories.
struct PreviewCacheKey final {
    std::uint64_t sourceId = 0, sourceRevision = 0, pipelineId = 0, pipelineRevision = 0;
    bool operator==(const PreviewCacheKey&) const = default;
};
enum class PreviewCacheAction { Lookup, Store, Invalidate, Clear };
struct PreviewCacheRequest final {
    PreviewCacheAction action = PreviewCacheAction::Lookup;
    PreviewCacheKey key;
    ImageView source; // Required for Lookup/Store; inspected only during the call.
    ImageView result; // Required for Store; copied into cache-owned storage.
};
struct PreviewCacheResult final { std::optional<Image> image; bool hit = false; };

namespace objects {

// Owns an editable sequence of settings snapshots. Every execute call renders
// synchronously from borrowed input and returns a detached frame. Failed edits
// and failed renders leave history and revision unchanged. The caller must
// serialize access and schedule refreshes; no thread, timer or callback is held.
// maxHistory counts the current snapshot and must be in [1,4096].
template<class Settings> class PreviewState final {
    std::vector<Settings> history_{Settings{}};
    std::size_t cursor_ = 0, maxHistory_ = 32;
    std::uint64_t revision_ = 0;
public:
    explicit PreviewState(std::size_t maxHistory = 32) : maxHistory_(maxHistory) {}
    [[nodiscard]] Result<PreviewFrame<Settings>> execute(
        ImageView image, PreviewAction action, std::optional<Settings> replacement = {});
};

using PuppetWarpPreviewState = PreviewState<PuppetWarpOptions>;
using PerspectiveWarpPreviewState = PreviewState<PerspectiveWarpOptions>;
using AdaptiveWideAnglePreviewState = PreviewState<AdaptiveWideAngleOptions>;
using LensCorrectionPreviewState = PreviewState<LensCorrectionOptions>;
using LiquifyPreviewState = PreviewState<LiquifyOptions>;
using VanishingPointPreviewState = PreviewState<VanishingPointParameters>;
using FilterGalleryPipeline = PreviewState<std::vector<GalleryEffect>>;

// Caller-owned, bounded LRU. A lookup invalidates entries whose source pixels,
// source revision, or pipeline revision changed. Identities must not be reused
// for different live sources/pipelines. Invalidate clears the specified pair;
// Clear removes all entries. Calls must be serialized externally.
class FilterPreviewCache final {
    struct Entry final { PreviewCacheKey key; Image source, result; };
    std::vector<Entry> entries_;
    std::size_t maxEntries_;
public:
    explicit FilterPreviewCache(std::size_t maxEntries = 8) : maxEntries_(maxEntries) {}
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<PreviewCacheResult>
    execute(const PreviewCacheRequest&);
};

// Maps caller-owned linear float32 HDR values to a detached normalized sRGB
// preview using hdrToning's explicit validation, clipping and alpha semantics.
struct Float32PreviewToneMapper final {
    HdrToningOptions options{HdrToningMethod::HighlightCompression,0,1};
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> execute(HdrImageView) const;
};

extern template class IIGRAPHICADJUST_EXPORT PreviewState<PuppetWarpOptions>;
extern template class IIGRAPHICADJUST_EXPORT PreviewState<PerspectiveWarpOptions>;
extern template class IIGRAPHICADJUST_EXPORT PreviewState<AdaptiveWideAngleOptions>;
extern template class IIGRAPHICADJUST_EXPORT PreviewState<LensCorrectionOptions>;
extern template class IIGRAPHICADJUST_EXPORT PreviewState<LiquifyOptions>;
extern template class IIGRAPHICADJUST_EXPORT PreviewState<VanishingPointParameters>;
extern template class IIGRAPHICADJUST_EXPORT PreviewState<std::vector<GalleryEffect>>;

} // namespace objects
} // namespace iiGraphicAdjust
