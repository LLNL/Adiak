#include "pyadiak.hpp"
#include "adiak.hpp"
#include <mpi.h>
#include <mpi4py/mpi4py.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <set>
#include <type_traits>

namespace adiak {
namespace python {

// Bindings for MPI Comm derived from this StackOverflow post:
// https://stackoverflow.com/a/62449190
struct mpi4py_comm {
  mpi4py_comm() = default;

  mpi4py_comm(MPI_Comm comm) : m_comm(comm) {}

  MPI_Comm m_comm;
};

namespace pybind11 {
namespace detail {

template <> struct type_caster<mpi4py_comm> {
  PYBIND11_TYPE_CASTER(mpi4py_comm, _("mpi4py_comm"));

  bool load(py::handle src, bool) {
    // If the comm is None, produce a mpi4py_comm object with m_comm set to
    // MPI_COMM_NULL
    if (src == py::none()) {
      value.m_comm = MPI_COMM_NULL;
      return !PyErr_Occured();
    }
    // Get the underlying PyObject pointer to the mpi4py object
    PyObject *py_capi_comm = src.ptr();
    // Make sure the PyObject pointer's underlying type is mpi4py's
    // PyMPIComm_Type. If the type check passes, use PyMPIComm_Get (from
    // mpi4py's C API) to get the actual MPI_Comm. If the type check fails, tell
    // pybind11 that the conversion is invalid.
    if (PyObject_TypeCheck(py_capi_comm, &PyMPIComm_Type)) {
      value.m_comm = *PyMPIComm_Get(py_capi_comm);
    } else {
      return false;
    }
    // Tell pybind11 that the conversion to C++ was successfull so long as there
    // were no Python errors
    return !PyErr_Occured();
  }

  static py::handle cast(mpi4py_comm src, py::return_value_policy, py::handle) {
    return PyMPIComm_New(src.m_comm);
  }
};

} // namespace detail
} // namespace pybind11

// The 'is_templated_base_of' type trait is derived from the following
// StackOverflow post:
// https://stackoverflow.com/a/49163138
template <template <typename...> class Base, typename Derived,
          typename TCheck = void>
struct is_templated_base_of_tester;

template <template <typename...> class Base, typename Derived>
struct is_templated_base_of_tester<
    Base, Derived, std::enable_if_t<std::is_class<Derived>::value>> : Derived {
  template <typename... T> static constexpr std::true_type test(Base<T...> *);
  static constexpr std::false_type test(...);
  using is_base = decltype(test((is_templated_base_of_tester *)nullptr));
};

template <template <typename...> class Base, typename Derived>
struct is_templated_base_of_tester<
    Base, Derived, std::enable_if_t<!std::is_class<Derived>::value>> {
  using is_base = std::false_type;
};

template <template <typename...> class Base, typename Derived>
using is_templated_base_of =
    typename is_templated_base_of_tester<Base, Derived>::is_base;

template <typename T, class adiak_type> struct DataContainer {
  T m_v;

  DataContainer(T val) : m_v(val) {}

  virtual adiak_type to_adiak() const = 0;
};

// Python-compatible struct for storing timepoint info
// that will be converted into 'struct timeval' for Adiak
struct Timepoint : public DataContainer<std::chrono::system_clock::time_point,
                                        struct timeval *> {
  struct timeval m_time_for_adiak;

  Timepoint(std::chrono::system_clock::time_point time)
      : DataContainer<std::chrono::system_clock::time_point, struct timeval *>(
            time) {}

  struct timeval *to_adiak() const final {
    auto time_since_epoch =
        std::chrono::duration_cast<std::chrono::microseconds>(
            m_v.time_since_epoch());
    m_time_for_adiak.tv_sec = time_since_epoch.count() / 1000000;
    m_time_for_adiak.tv_usec = time_since_epoch.count() % 1000000;
    return &m_time_for_adiak;
  }
};

// Python-compatible struct for storing date or datetime info
// that will be converted into 'adiak::date' for Adiak
struct Date
    : public DataContainer<std::chrono::system_clock::time_point, adiak::date> {
  Date(std::chrono::system_clock::time_point time)
      : DataContainer<std::chrono::system_clock::time_point, adiak::date>(
            time) {}

  adiak::date to_adiak() const final {
    auto time_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(
        value.time_since_epoch());
    return adiak::date(time_since_epoch.count());
  }
};

// Python-compatible struct for storing version info
// that will be converted into 'adiak::version' for Adiak
struct Version : public DataContainer<std::string, adiak::version> {
  Version(const std::string &ver)
      : DataContainer<std::string, adiak::version>(ver) {}

  adiak::version to_adiak() const final { return adiak::version(m_v); }
};

// TODO replace use of string with std::filesystem if/when C++17 is supported
// Python-compatible struct for storing path info
// that will be converted into 'adiak::path' for Adiak
struct Path : public DataContainer<std::string, adiak::path> {
  Path(const std::string &p) : DataContainer<std::string, adiak::path>(p) {}

  adiak::path to_adiak() const final { return adiak::path(m_v); }
};

// Python-compatible struct for storing categorical string info
// that will be converted into 'adiak::catstring' for Adiak
struct CatStr : public DataContainer<std::string, adiak::catstring> {
  CatStr(const std::string &cs)
      : DataContainer<std::string, adiak::catstring>(cs) {}

  adiak::catstring to_adiak() const { return adiak::catstring(m_v); }
};

// Python-compatible struct for storing JSON string info
// that will be converted into 'adiak::jsonstring' for Adiak
struct JsonStr : public DataContainer<std::string, adiak::jsonstring> {
  JsonStr(const std::string &js)
      : DataContainer<std::string, adiak::jsonstring>(js) {}

  adiak::jsonstring to_adiak() const { return adiak::jsonstring(m_v); }
};

// Custom wrapper for adiak::init
// The only difference is that it uses the behavior of the pybind11
// type_caster above to decide whether to pass an MPI communicator
// to adiak::init
void init(mpi4py_comm comm) {
  if (comm.m_comm == MPI_COMM_NULL) {
    adiak::init(nullptr);
  } else {
    adiak::init(&comm.m_comm);
  }
}

// Custom version of adiak::value for subclasses of DataContainer
template <class T, typename = std::enable_if_t<
                       std::is_templated_base_of<DataContainer, T>::value>>
bool value(std::string name, T value, int category = adiak_general,
           std::string subcategory = "") {
  return adiak::value(name, value.to_adiak(), category, subcategory);
}

// Custom version of adiak::value for vectors of subclasses of DataContainer
template <class T, typename = std::enable_if_t<
                       std::is_templated_base_of<DataContainer, T>::value>>
bool value_vec(std::string name, std::vector<T> value,
               int category = adiak_general, std::string subcategory = "") {
  std::vector<std::result_of<decltype (&T::to_adiak)()>::type> adiak_vec;
  std::transform(value.cbegin(), value.cend(), adiak_vec.cbegin(),
                 [](T v) { return v.to_adiak(); });
  return adiak::value(name, adiak_vec, category, subcategory);
}

// Custom version of adiak::value for sets of subclasses of DataContainer
template <class T, typename = std::enable_if_t<
                       std::is_templated_base_of<DataContainer, T>::value>>
bool value_set(std::string name, std::set<T> value,
               int category = adiak_general, std::string subcategory = "") {
  std::set<std::result_of<decltype (&T::to_adiak)()>::type> adiak_set;
  std::transform(value.cbegin(), value.cend(), adiak_set.cbegin(),
                 [](T v) { return v.to_adiak(); });
  return adiak::value(name, adiak_set, category, subcategory);
}

// Macros to help generate the lambdas that pybind11 needs to bind the inline
// functions in adiak.hpp
//
// clang-format off
#define GENERATE_API_CALL_NO_ARGS(name)                                                \
  [](){ return adiak::name(); }
#define GENERATE_API_CALL_ONE_ARG(name, arg_type) [](arg_type val){ return adiak::name(val); }
// clang-format on

// Function to populate the Python module
void create_adiak_annotation_mod(py::module_ &mod) {
  // Try to import mpi4py so that we can early abort if
  // we can't properly create bindings
  if (import_mpi4py() < 0) {
    throw std::runtime_error(
        "Cannot load mpi4py within the Adiak Python bindings");
  }

  // Bind the custom data containers to Python.
  // Note that we don't wrap the DataContainer base class or the 'to_adiak'
  // methods because we don't need those exposed to Python.
  py::class_<adiak::python::Timepoint>(mod, "Timepoint")
      .def(py::init<std::chrono::system_clock::time_point>());
  py::class_<adiak::python::Date>(mod, "Date")
      .def(py::init<std::chrono::system_clock::time_point>());
  py::class_<adiak::python::Version>(mod, "Version")
      .def(py::init<const std::string &>());
  py::class_<adiak::python::Path>(mod, "Path")
      .def(py::init<const std::string &>());
  py::class_<adiak::python::CatStr>(mod, "CatStr")
      .def(py::init<const std::string &>());
  py::class_<adiak::python::JsonStr>(mod, "JsonStr")
      .def(py::init<const std::string &>());

  // Bind 'init' and 'fini'
  mod.def("init", &adiak::python::init);
  mod.def("fini", GENERATE_API_CALL_NO_ARGS(fini));
  // Python has no concept of templates, so pybind11 requires
  // us to specify the types for all template functions. The requires us to
  // iterate over all acceptable value types for adiak::value.
  //
  // Because we need to use Python-compatible types everywhere,
  // we use special wrappers for types not natively supported by pybind11.
  // To bridge these types to Adiak, we use custom versions of "value".
  //
  // We map all these specializations to the Python "value" function.
  // Pybind11 will do the work of selecting the correct version of this
  // function for us.
  mod.def("value", &adiak::value<int8_t>);
  mod.def("value", &adiak::value<uint64_t>);
  mod.def("value", &adiak::value<double>);
  mod.def("value", &adiak::value<std::string>);
  mod.def("value", &adiak::python::value<adiak::python::Timepoint>);
  mod.def("value", &adiak::python::value<adiak::python::Date>);
  mod.def("value", &adiak::python::value<adiak::python::Version>);
  mod.def("value", &adiak::python::value<adiak::python::Path>);
  mod.def("value", &adiak::python::value<adiak::python::CatStr>);
  mod.def("value", &adiak::python::value<adiak::python::JsonStr>);
  mod.def("value", &adiak::value<std::vector<int8_t>>);
  mod.def("value", &adiak::value<std::vector<uint64_t>>);
  mod.def("value", &adiak::value<std::vector<double>>);
  mod.def("value", &adiak::value<std::vector<std::string>>);
  mod.def("value", &adiak::python::value_vec<adiak::python::Timepoint>);
  mod.def("value", &adiak::python::value_vec<adiak::python::Date>);
  mod.def("value", &adiak::python::value_vec<adiak::python::Version>);
  mod.def("value", &adiak::python::value_vec<adiak::python::Path>);
  mod.def("value", &adiak::python::value_vec<adiak::python::CatStr>);
  mod.def("value", &adiak::python::value_vec<adiak::python::JsonStr>);
  mod.def("value", &adiak::value<std::set<int8_t>>);
  mod.def("value", &adiak::value<std::set<uint64_t>>);
  mod.def("value", &adiak::value<std::set<double>>);
  mod.def("value", &adiak::value<std::set<std::string>>);
  mod.def("value", &adiak::python::value_set<adiak::python::Timepoint>);
  mod.def("value", &adiak::python::value_set<adiak::python::Date>);
  mod.def("value", &adiak::python::value_set<adiak::python::Version>);
  mod.def("value", &adiak::python::value_set<adiak::python::Path>);
  mod.def("value", &adiak::python::value_set<adiak::python::CatStr>);
  mod.def("value", &adiak::python::value_set<adiak::python::JsonStr>);

  // Bind all the other Adiak functions to custom lambda functions
  // created by the GENERATE_API_CALL_* macros. These labmdas are
  // used over the plain versions of these functions to avoid any issues
  // with those functions being marked 'inline'.
  mod.def("adiakversion", GENERATE_API_CALL_NO_ARGS(adiakversion));
  mod.def("user", GENERATE_API_CALL_NO_ARGS(user));
  mod.def("uid", GENERATE_API_CALL_NO_ARGS(uid));
  mod.def("launchdate", GENERATE_API_CALL_NO_ARGS(launchdate));
  mod.def("launchday", GENERATE_API_CALL_NO_ARGS(launchday));
  mod.def("executable", GENERATE_API_CALL_NO_ARGS(executable));
  mod.def("executablepath", GENERATE_API_CALL_NO_ARGS(executablepath));
  mod.def("workdir", GENERATE_API_CALL_NO_ARGS(workdir));
  mod.def("libraries", GENERATE_API_CALL_NO_ARGS(libraries));
  mod.def("cmdline", GENERATE_API_CALL_NO_ARGS(cmdline));
  mod.def("hostname", GENERATE_API_CALL_NO_ARGS(hostname));
  mod.def("clustername", GENERATE_API_CALL_NO_ARGS(clustername));
  mod.def("walltime", GENERATE_API_CALL_NO_ARGS(walltime));
  mod.def("systime", GENERATE_API_CALL_NO_ARGS(systime));
  mod.def("cputime", GENERATE_API_CALL_NO_ARGS(cputime));
  mod.def("jobsize", GENERATE_API_CALL_NO_ARGS(jobsize));
  mod.def("hostlist", GENERATE_API_CALL_NO_ARGS(hostlist));
  mod.def("numhosts", GENERATE_API_CALL_NO_ARGS(numhosts));
  mod.def("mpi_version", GENERATE_API_CALL_NO_ARGS(mpi_version));
  mod.def("mpi_library", GENERATE_API_CALL_NO_ARGS(mpi_library));
  mod.def("mpi_library_version",
          GENERATE_API_CALL_NO_ARGS(mpi_library_version));
  mod.def("flush", GENERATE_API_CALL_ONE_ARG(flush, std::string));
  mod.def("clean", GENERATE_API_CALL_NO_ARGS(clean));
  mod.def("collect_all", GENERATE_API_CALL_NO_ARGS(collect_all));
}

} // namespace python
} // namespace adiak