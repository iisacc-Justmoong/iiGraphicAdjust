#include "AdvancedPython.h"

namespace iiGraphicAdjust::python {
// Module initialization installs value types first so later bindings can
// safely convert nested options, images, and mask snapshots.
PYBIND11_MODULE(_iigraphicadjust, module) {
    module.doc() = "Native in-memory iiGraphicAdjust advanced editing API";
    py::class_<Rgba>(module,"Rgba")
        .def(py::init<>())
        .def(py::init<float,float,float,float>(),py::arg("r"),py::arg("g"),py::arg("b"),py::arg("a")=1)
        .def_readwrite("r",&Rgba::r).def_readwrite("g",&Rgba::g)
        .def_readwrite("b",&Rgba::b).def_readwrite("a",&Rgba::a);
    py::class_<Image>(module,"Image")
        .def(py::init<>())
        .def(py::init<std::size_t,std::size_t,std::vector<Rgba>>(),py::arg("width"),py::arg("height"),py::arg("pixels"))
        .def_readwrite("width",&Image::width).def_readwrite("height",&Image::height)
        .def_readwrite("pixels",&Image::pixels);
    py::class_<Mask>(module,"Mask")
        .def(py::init<>())
        .def(py::init<std::size_t,std::size_t,std::vector<float>>(),py::arg("width"),py::arg("height"),py::arg("coverage"))
        .def_readwrite("width",&Mask::width).def_readwrite("height",&Mask::height)
        .def_readwrite("coverage",&Mask::coverage);
    py::class_<HdrRgba>(module,"HdrRgba")
        .def(py::init<>())
        .def(py::init<float,float,float,float>(),py::arg("r"),py::arg("g"),py::arg("b"),py::arg("a")=1)
        .def_readwrite("r",&HdrRgba::r).def_readwrite("g",&HdrRgba::g)
        .def_readwrite("b",&HdrRgba::b).def_readwrite("a",&HdrRgba::a);
    bindContent(module);
    bindLayers(module);
    bindGeometry(module);
    bindGalleryA(module);
    bindGalleryB(module);
    bindGalleryC(module);
    bindPreviews(module);
}
} // namespace iiGraphicAdjust::python
