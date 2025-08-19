#include <pybind11/pybind11.h>
#include <pybind11/stl.h>   // for map, vector, string
#include "SampleTool.h"

namespace py = pybind11;

PYBIND11_MODULE(pySampleTool, m) {
    m.doc() = "Python bindings for SampleTool";

    py::class_<SampleTool>(m, "SampleTool")
        .def(py::init<>())  // constructor

        // expose members
        .def_readwrite("BkgDict", &SampleTool::BkgDict)
        .def_readwrite("SigDict", &SampleTool::SigDict)
        .def_readwrite("MasterDict", &SampleTool::MasterDict)
        .def_readwrite("SignalKeys", &SampleTool::SignalKeys)

        // expose methods
        .def("LoadBkgs", &SampleTool::LoadBkgs)
        .def("LoadSigs", &SampleTool::LoadSigs)
        .def("PrintDict", &SampleTool::PrintDict)
        .def("PrintKeys", &SampleTool::PrintKeys);
}

