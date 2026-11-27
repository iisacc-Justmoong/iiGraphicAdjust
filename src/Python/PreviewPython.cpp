#include "AdvancedPython.h"

namespace iiGraphicAdjust::python {
namespace {
// Converts a completed snapshot to Python-owned values. The setting value is
// copied just like the native result; no mutable C++ history is exposed.
template<class Settings> py::dict frameDictionary(PreviewFrame<Settings> frame) {
    py::dict out;
    out["image"]=py::cast(std::move(frame.image));
    out["settings"]=py::cast(std::move(frame.settings));
    out["revision"]=frame.revision;
    out["can_undo"]=frame.canUndo;
    out["can_redo"]=frame.canRedo;
    return out;
}
template<class Settings> void bindPreview(py::module_& module,const char *name) {
    using State=objects::PreviewState<Settings>;
    py::class_<State>(module,name)
        .def(py::init<std::size_t>(),py::arg("max_history")=32)
        .def("execute",[](State& state,const Image& image,PreviewAction action,
                          std::optional<Settings> settings) {
            return frameDictionary(checked(state.execute(image.view(),action,std::move(settings))));
        },py::arg("image"),py::arg("action")=PreviewAction::Refresh,py::arg("settings")=std::nullopt);
}
} // namespace

void bindPreviews(py::module_& module) {
    using namespace objects;
    py::enum_<PreviewAction>(module,"PreviewAction")
        .value("REFRESH",PreviewAction::Refresh).value("REPLACE",PreviewAction::Replace)
        .value("UNDO",PreviewAction::Undo).value("REDO",PreviewAction::Redo);
    bindPreview<PuppetWarpOptions>(module,"PuppetWarpPreviewState");
    bindPreview<PerspectiveWarpOptions>(module,"PerspectiveWarpPreviewState");
    bindPreview<AdaptiveWideAngleOptions>(module,"AdaptiveWideAnglePreviewState");
    bindPreview<LensCorrectionOptions>(module,"LensCorrectionPreviewState");
    bindPreview<LiquifyOptions>(module,"LiquifyPreviewState");
    bindPreview<VanishingPointParameters>(module,"VanishingPointPreviewState");
    bindPreview<std::vector<GalleryEffect>>(module,"FilterGalleryPipeline");
    py::class_<PreviewCacheKey>(module,"PreviewCacheKey").def(py::init<>())
        .def_readwrite("source_id",&PreviewCacheKey::sourceId)
        .def_readwrite("source_revision",&PreviewCacheKey::sourceRevision)
        .def_readwrite("pipeline_id",&PreviewCacheKey::pipelineId)
        .def_readwrite("pipeline_revision",&PreviewCacheKey::pipelineRevision);
    py::enum_<PreviewCacheAction>(module,"PreviewCacheAction")
        .value("LOOKUP",PreviewCacheAction::Lookup).value("STORE",PreviewCacheAction::Store)
        .value("INVALIDATE",PreviewCacheAction::Invalidate).value("CLEAR",PreviewCacheAction::Clear);
    py::class_<FilterPreviewCache>(module,"FilterPreviewCache")
        .def(py::init<std::size_t>(),py::arg("max_entries")=8)
        .def("execute",[](FilterPreviewCache& cache,PreviewCacheAction action,const PreviewCacheKey& key,
                          const std::optional<Image>& source,const std::optional<Image>& image) {
            PreviewCacheRequest request{action,key,source?source->view():ImageView{},image?image->view():ImageView{}};
            auto result=checked(cache.execute(request));
            py::dict out; out["hit"]=result.hit;
            out["image"]=result.image?py::cast(std::move(*result.image)):py::none();
            return out;
        },py::arg("action"),py::arg("key"),py::arg("source")=std::nullopt,py::arg("image")=std::nullopt);
    py::enum_<HdrToningMethod>(module,"HdrToningMethod")
        .value("EXPOSURE_GAMMA",HdrToningMethod::ExposureGamma)
        .value("HIGHLIGHT_COMPRESSION",HdrToningMethod::HighlightCompression);
    py::class_<HdrToningOptions>(module,"HdrToningOptions").def(py::init<>())
        .def_readwrite("method",&HdrToningOptions::method).def_readwrite("exposure",&HdrToningOptions::exposure)
        .def_readwrite("gamma",&HdrToningOptions::gamma);
    py::class_<Float32PreviewToneMapper>(module,"Float32PreviewToneMapper")
        .def(py::init<>()).def_readwrite("options",&Float32PreviewToneMapper::options)
        .def("execute",[](const Float32PreviewToneMapper& self,std::size_t width,std::size_t height,
                          const std::vector<HdrRgba>& pixels) {
            return checked(self.execute({width,height,pixels}));
        });
}
} // namespace iiGraphicAdjust::python
