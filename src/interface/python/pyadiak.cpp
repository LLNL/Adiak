#include "pyadiak.hpp"
#include "adiak.hpp"
#if defined(ENABLE_MPI)
    #include <mpi.h>
    #include <mpi4py/mpi4py.h>
#endif
#include "types.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

// Bindings for MPI Comm derived from this StackOverflow post:
// https://stackoverflow.com/a/62449190
struct mpi4py_comm {
  mpi4py_comm() = default;

  #if defined(ENABLE_MPI)
    mpi4py_comm(MPI_Comm comm) : m_comm(comm) {}
    MPI_Comm m_comm;
  #else
    // no-op placeholder so the type still compiles
    mpi4py_comm(int /*unused*/ = 0) {}
    std::nullptr_t m_comm = nullptr;
  #endif
};

namespace pybind11 {
namespace detail {

#if defined(ENABLE_MPI)
template <> struct type_caster<mpi4py_comm> {
  PYBIND11_TYPE_CASTER(mpi4py_comm, _("mpi4py_comm"));

  bool load(py::handle src, bool) {
    // If the comm is None, produce a mpi4py_comm object with m_comm set to
    // MPI_COMM_NULL
    if (src.is(py::none())) {
      value.m_comm = MPI_COMM_NULL;
      return !PyErr_Occurred();
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
    return !PyErr_Occurred();
  }

  static py::handle cast(mpi4py_comm src, py::return_value_policy, py::handle) {
    return PyMPIComm_New(src.m_comm);
  }
};
#endif

} // namespace detail
} // namespace pybind11

namespace adiak {
namespace python {

// Custom wrapper for adiak::init
// The only difference is that it uses the behavior of the pybind11
// type_caster above to decide whether to pass an MPI communicator
// to adiak::init
void init(mpi4py_comm comm) {
#if defined(ENABLE_MPI)
  if (comm.m_comm == MPI_COMM_NULL) {
    adiak::init(nullptr);
  } else {
    adiak::init(&comm.m_comm);
  }
#else
  adiak::init(comm.m_comm);
#endif
}

// Custom version of adiak::value for types that provide .to_adiak()
// Use detection instead of inheritance to improve portability across compilers.
template <class T, class U = decltype(std::declval<const T&>().to_adiak())>
bool value(std::string name, const T& value, int category = adiak_general,
           std::string subcategory = "") {
  (void)sizeof(U); // participates in overload resolution only if to_adiak() is valid
  return adiak::value(name, value.to_adiak(), category, subcategory);
}

template <class T, class U = decltype(std::declval<const T&>().to_adiak())>
bool value_vec(std::string name, const std::vector<T>& value,
               int category = adiak_general, std::string subcategory = "") {
  (void)sizeof(U);
  using adiak_type = decltype(std::declval<const T&>().to_adiak());
  std::vector<adiak_type> adiak_vec;
  std::transform(value.cbegin(), value.cend(), std::back_inserter(adiak_vec),
                 [](const T& v) { return v.to_adiak(); });
  return adiak::value(name, adiak_vec, category, subcategory);
}

// Custom version of adiak::value for sets of types that provide .to_adiak()
template <class T, class U = decltype(std::declval<const T&>().to_adiak())>
bool value_set(std::string name, const std::set<T>& value,
               int category = adiak_general, std::string subcategory = "") {
  (void)sizeof(U);
  using adiak_type = decltype(std::declval<const T&>().to_adiak());
  std::set<adiak_type> adiak_set;
  std::transform(value.cbegin(), value.cend(), std::inserter(adiak_set, adiak_set.end()),
                 [](const T& v) { return v.to_adiak(); });
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
  #if defined(ENABLE_MPI)
  if (import_mpi4py() < 0) {
    throw std::runtime_error(
        "Cannot load mpi4py within the Adiak Python bindings");
  }
  #endif

#if defined(ENABLE_MPI)
  // Robust overload that accepts mpi4py.MPI.Comm (or None) without needing a type_caster
  mod.def("init", [](py::object comm_obj) {
  if (comm_obj.is_none()) {
    adiak::init(nullptr);
    return;
  }
  // Validate it is an mpi4py.MPI.Comm and extract the MPI_Comm
  PyObject* obj = comm_obj.ptr();
  if (!PyObject_TypeCheck(obj, &PyMPIComm_Type)) {
    throw py::type_error("adiak.init expects mpi4py.MPI.Comm or None");
  }
  MPI_Comm comm = *PyMPIComm_Get(obj);
  adiak::init(&comm);
});
#else
  // Non-MPI build: accept anything and ignore (acts like None)
  mod.def("init", [](py::object) {
    adiak::init(nullptr);
  });
#endif

  mod.def("fini", GENERATE_API_CALL_NO_ARGS(fini));
  // Python has no concept of templates, so pybind11 requires
  // us to specify the types for all template functions. This requires us to
  // iterate over all acceptable value types for adiak::value.
  //
  // Because we need to use Python-compatible types everywhere,
  // we use special wrappers for types not natively supported by pybind11.
  // To bridge these types to Adiak, we use custom versions of "value".
  //
  // We map all these specializations to the Python "value" function.
  // Pybind11 will do the work of selecting the correct version of this
  // function for us.
  mod.def("value", [](const std::string& name, int val, int category, const std::string& subcategory) {
      return adiak::value(name, static_cast<int>(val), category, subcategory);
  });
  mod.def("value", [](const std::string& name, unsigned int val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, double val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::string& val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const adiak::python::Timepoint& val, int category, const std::string& subcategory) {
      return adiak::python::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const adiak::python::Date& val, int category, const std::string& subcategory) {
      return adiak::python::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const adiak::python::Version& val, int category, const std::string& subcategory) {
      return adiak::python::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const adiak::python::Path& val, int category, const std::string& subcategory) {
      return adiak::python::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const adiak::python::CatStr& val, int category, const std::string& subcategory) {
      return adiak::python::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const adiak::python::JsonStr& val, int category, const std::string& subcategory) {
      return adiak::python::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<int>& val, int category, const std::string& subcategory) {
      std::vector<int> converted(val.begin(), val.end());
      return adiak::value(name, converted, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<unsigned int>& val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<double>& val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<std::string>& val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<adiak::python::Timepoint>& val, int category, const std::string& subcategory) {
      return adiak::python::value_vec(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<adiak::python::Date>& val, int category, const std::string& subcategory) {
      return adiak::python::value_vec(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<adiak::python::Version>& val, int category, const std::string& subcategory) {
      return adiak::python::value_vec(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<adiak::python::Path>& val, int category, const std::string& subcategory) {
      return adiak::python::value_vec(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<adiak::python::CatStr>& val, int category, const std::string& subcategory) {
      return adiak::python::value_vec(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::vector<adiak::python::JsonStr>& val, int category, const std::string& subcategory) {
      return adiak::python::value_vec(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<int>& val, int category, const std::string& subcategory) {
      std::set<int> converted(val.begin(), val.end());
      return adiak::value(name, converted, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<unsigned int>& val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<double>& val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<std::string>& val, int category, const std::string& subcategory) {
      return adiak::value(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<adiak::python::Timepoint>& val, int category, const std::string& subcategory) {
      return adiak::python::value_set(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<adiak::python::Date>& val, int category, const std::string& subcategory) {
      return adiak::python::value_set(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<adiak::python::Version>& val, int category, const std::string& subcategory) {
      return adiak::python::value_set(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<adiak::python::Path>& val, int category, const std::string& subcategory) {
      return adiak::python::value_set(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<adiak::python::CatStr>& val, int category, const std::string& subcategory) {
      return adiak::python::value_set(name, val, category, subcategory);
  });
  mod.def("value", [](const std::string& name, const std::set<adiak::python::JsonStr>& val, int category, const std::string& subcategory) {
      return adiak::python::value_set(name, val, category, subcategory);
  });
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