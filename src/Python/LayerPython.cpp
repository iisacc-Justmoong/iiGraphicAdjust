#include "AdvancedPython.h"

namespace iiGraphicAdjust::python {
namespace {
// Python-facing layer owns its image so temporary Python lists cannot leave
// dangling C++ views while the stack is evaluated.
struct OwnedLayer final { Image image; BlendOptions options; };
}
void bindLayers(py::module_& module) {
    using namespace objects;
    py::enum_<BlendMode>(module,"BlendMode")
        .value("NORMAL",BlendMode::Normal).value("MULTIPLY",BlendMode::Multiply)
        .value("SCREEN",BlendMode::Screen).value("OVERLAY",BlendMode::Overlay)
        .value("DARKEN",BlendMode::Darken).value("LIGHTEN",BlendMode::Lighten)
        .value("SOFT_LIGHT",BlendMode::SoftLight).value("DIFFERENCE",BlendMode::Difference);
    py::enum_<BlendSpace>(module,"BlendSpace")
        .value("SRGB",BlendSpace::Srgb).value("LINEAR_SRGB",BlendSpace::LinearSrgb);
    py::class_<BlendOptions>(module,"BlendOptions")
        .def(py::init<>()).def_readwrite("mode",&BlendOptions::mode)
        .def_readwrite("opacity",&BlendOptions::opacity).def_readwrite("space",&BlendOptions::space);
    py::class_<OwnedLayer>(module,"Layer")
        .def(py::init<>()).def_readwrite("image",&OwnedLayer::image).def_readwrite("options",&OwnedLayer::options);
    py::class_<LayerTranslation>(module,"LayerTranslation")
        .def_readonly("dx",&LayerTranslation::dx).def_readonly("dy",&LayerTranslation::dy)
        .def_readonly("error",&LayerTranslation::error);
    py::class_<LayerRegistrationSolver>(module,"LayerRegistrationSolver")
        .def(py::init<>()).def_readwrite("max_shift_x",&LayerRegistrationSolver::maxShiftX)
        .def_readwrite("max_shift_y",&LayerRegistrationSolver::maxShiftY)
        .def_readwrite("min_overlap",&LayerRegistrationSolver::minOverlap)
        .def("execute",[](const LayerRegistrationSolver& self,const Image& reference,const Image& moving) {
            return checked(self.execute(reference.view(),moving.view()));
        });
    py::class_<LayerStackBlendSolver>(module,"LayerStackBlendSolver")
        .def(py::init<>()).def("execute",[](const LayerStackBlendSolver& self,const std::vector<OwnedLayer>& layers) {
            std::vector<LayerView> views; views.reserve(layers.size());
            for (const auto& layer:layers) views.push_back({layer.image.view(),layer.options});
            return checked(self.execute(views));
        });
    py::class_<SkyReplacementCompositor>(module,"SkyReplacementCompositor")
        .def(py::init<>()).def("execute",[](const SkyReplacementCompositor& self,const Image& source,
                                             const Image& sky,const Mask& mask) {
            return checked(self.execute(source.view(),sky.view(),mask.view()));
        });
    py::class_<SubjectEdgeMatteRefiner>(module,"SubjectEdgeMatteRefiner")
        .def(py::init<>()).def_readwrite("amount",&SubjectEdgeMatteRefiner::amount)
        .def("execute",[](const SubjectEdgeMatteRefiner& self,const Image& image,const Mask& mask) {
            return checked(self.execute(image.view(),mask.view()));
        });
    py::class_<ColorDecontaminationPass>(module,"ColorDecontaminationPass")
        .def(py::init<>()).def_readwrite("amount",&ColorDecontaminationPass::amount)
        .def_readwrite("minimum_coverage",&ColorDecontaminationPass::minimumCoverage)
        .def("execute",[](const ColorDecontaminationPass& self,const Image& source,const Image& background,
                          const Mask& mask) { return checked(self.execute(source.view(),background.view(),mask.view())); });
}
} // namespace iiGraphicAdjust::python
