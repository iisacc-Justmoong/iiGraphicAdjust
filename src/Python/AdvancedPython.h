#pragma once

#include <iiGraphicAdjust/ContentAware.h>
#include <iiGraphicAdjust/LayerCompositing.h>
#include <iiGraphicAdjust/Preview.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace iiGraphicAdjust::python {
namespace py = pybind11;

// Convert explicit native failures to Python exceptions; successful values own
// their storage and do not retain Python buffers or references.
template<class T> T checked(Result<T> result) {
    if (!result) throw py::value_error(std::string(result.error().message));
    return std::move(*result);
}

// Each registration function only populates the module at import time. Python
// objects own their values; calls remain synchronous and require the GIL.
void bindContent(py::module_&);
void bindLayers(py::module_&);
void bindGeometry(py::module_&);
void bindGalleryA(py::module_&);
void bindGalleryB(py::module_&);
void bindGalleryC(py::module_&);
void bindPreviews(py::module_&);
} // namespace iiGraphicAdjust::python
