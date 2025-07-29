#include "types.hpp"
#include "adiak.hpp"

namespace adiak {
namespace python {

Timepoint::Timepoint(std::chrono::system_clock::time_point time)
    : DataContainer<std::chrono::system_clock::time_point, struct timeval *>(
          time) {}

struct timeval *Timepoint::to_adiak() const {
  auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(
      m_v.time_since_epoch());
  m_time_for_adiak.tv_sec = time_since_epoch.count() / 1000000;
  m_time_for_adiak.tv_usec = time_since_epoch.count() % 1000000;
  return &m_time_for_adiak;
}

Date::Date(std::chrono::system_clock::time_point time)
    : DataContainer<std::chrono::system_clock::time_point, adiak::date>(time) {}

adiak::date Date::to_adiak() const {
  auto time_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(
      m_v.time_since_epoch());
  return adiak::date(time_since_epoch.count());
}

Version::Version(const std::string &ver)
    : DataContainer<std::string, adiak::version>(ver) {}

adiak::version Version::to_adiak() const { return adiak::version(m_v); }

Path::Path(const std::string &p) : DataContainer<std::string, adiak::path>(p) {}

adiak::path Path::to_adiak() const { return adiak::path(m_v); }

CatStr::CatStr(const std::string &cs)
    : DataContainer<std::string, adiak::catstring>(cs) {}

adiak::catstring CatStr::to_adiak() const { return adiak::catstring(m_v); }

JsonStr::JsonStr(const std::string &js)
    : DataContainer<std::string, adiak::jsonstring>(js) {}

adiak::jsonstring JsonStr::to_adiak() const { return adiak::jsonstring(m_v); }

void create_adiak_types_mod(py::module_ &mod) {
  mod.attr("ADIAK_CATEGORY_UNSET") = py::int_(adiak_category_unset);
  mod.attr("ADIAK_CATEGORY_ALL") = py::int_(adiak_category_all);
  mod.attr("ADIAK_CATEGORY_GENERAL") = py::int_(adiak_general);
  mod.attr("ADIAK_CATEGORY_PERFORMANCE") = py::int_(adiak_performance);
  mod.attr("ADIAK_CATEGORY_CONTROL") = py::int_(adiak_control);
  // Bind the custom data containers to Python.
  // Note that we don't wrap the DataContainer base class or the 'to_adiak'
  // methods because we don't need those exposed to Python.
  py::class_<adiak::python::Timepoint>(mod, "Timepoint")
      .def(py::init<std::chrono::system_clock::time_point>())
      .def("to_python", &adiak::python::Timepoint::to_python);
  py::class_<adiak::python::Date>(mod, "Date")
      .def(py::init<std::chrono::system_clock::time_point>())
      .def("to_python", &adiak::python::Date::to_python);
  py::class_<adiak::python::Version>(mod, "Version")
      .def(py::init<const std::string &>())
      .def("to_python", &adiak::python::Version::to_python);
  py::class_<adiak::python::Path>(mod, "Path")
      .def(py::init<const std::string &>())
      .def("to_python", &adiak::python::Path::to_python);
  py::class_<adiak::python::CatStr>(mod, "CatStr")
      .def(py::init<const std::string &>())
      .def("to_python", &adiak::python::CatStr::to_python);
  py::class_<adiak::python::JsonStr>(mod, "JsonStr")
      .def(py::init<const std::string &>())
      .def("to_python", &adiak::python::JsonStr::to_python);
}

} // namespace python
} // namespace adiak