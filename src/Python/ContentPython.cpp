#include "AdvancedPython.h"

namespace iiGraphicAdjust::python {
// Python values are copied by pybind11. Every operation borrows their native
// data only until the call returns and propagates native validation failures.
void bindContent(py::module_& module) {
    using namespace objects;
    py::class_<SeamConstraints>(module,"SeamConstraints")
        .def(py::init<>()).def_readwrite("width",&SeamConstraints::width)
        .def_readwrite("height",&SeamConstraints::height).def_readwrite("weights",&SeamConstraints::weights);
    py::class_<ContentAwareInpaintSolver>(module,"ContentAwareInpaintSolver")
        .def(py::init<>()).def_readwrite("patch_radius",&ContentAwareInpaintSolver::patchRadius)
        .def_readwrite("max_candidates",&ContentAwareInpaintSolver::maxCandidates)
        .def("execute",[](const ContentAwareInpaintSolver& self,const Image& image,const Mask& mask) {
            return checked(self.execute(image.view(),mask.view()));
        });
    py::class_<SeamConstraintMask>(module,"SeamConstraintMask")
        .def(py::init<>()).def("execute",[](const SeamConstraintMask& self,const Mask& protect,const Mask& remove) {
            return checked(self.execute(protect.view(),remove.view()));
        });
    py::class_<ContentAwareRescaleSolver>(module,"ContentAwareRescaleSolver")
        .def(py::init<>()).def_readwrite("max_pixels",&ContentAwareRescaleSolver::maxPixels)
        .def_readwrite("max_seams",&ContentAwareRescaleSolver::maxSeams)
        .def("execute",[](const ContentAwareRescaleSolver& self,const Image& image,std::size_t width,
                          std::size_t height,const std::optional<SeamConstraints>& constraints) {
            return checked(self.execute(image.view(),width,height,constraints?constraints->view():SeamConstraintView{}));
        },py::arg("image"),py::arg("width"),py::arg("height"),py::arg("constraints")=std::nullopt);
}
} // namespace iiGraphicAdjust::python
