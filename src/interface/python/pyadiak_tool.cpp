#include "pyadiak_tool.hpp"
#include "adiak_tool.h"
#include "types.hpp"

#include <chrono>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace adiak {
namespace python {

class PyadiakDataType {
public:
  PyadiakDataType() = default;

  PyadiakDataType(adiak_datatype_t *c_type)
      : m_adiak_dtype_ptr(c_type), m_dtype(c_type->dtype),
        m_numerical(c_type->numerical), m_num_elems(c_type->num_elements),
        m_num_subtypes(c_type->num_subtypes), m_subtypes(),
        m_is_reference(c_type->is_reference == 0 ? false : true),
        m_num_ref_elements(c_type->num_ref_elements) {
    for (int i = 0; i < m_num_subtypes; i++) {
      m_subtypes.push_back(PyadiakDataType(c_type->subtype[i]));
    }
  }

  adiak_type_t get_dtype() const { return m_dtype; }

  adiak_numerical_t get_numerical() const { return m_numerical; }

  int get_num_elems() const {
    if (m_is_reference) {
      return m_num_ref_elements;
    } else {
      return m_num_elems;
    }
  }

  int get_num_subtypes_in_container() const { return m_num_subtypes; }

  bool is_reference() const { return m_is_reference; }

  std::vector<PyadiakDataType> get_subtypes() const { return m_subtypes; }

  adiak_datatype_t *get_adiak_dtype() { return m_adiak_dtype_ptr; }

private:
  adiak_datatype_t *m_adiak_dtype_ptr;
  adiak_type_t m_dtype;
  adiak_numerical_t m_numerical;
  int m_num_elems;
  int m_num_subtypes;
  std::vector<PyadiakDataType> m_subtypes;
  bool m_is_reference;
  int m_num_ref_elements;
};

py::object convert_adiak_value_to_python(adiak_value_t *val,
                                         adiak_datatype_t *type) {
  // TODO figure out if I need to do anything else to cast custom types to
  // py::object
  switch (type->dtype) {
  case adiak_long:
    return py::cast(val->v_long);
    break;
  case adiak_ulong:
    return py::cast((unsigned long)val->v_long);
    break;
  case adiak_int:
    return py::cast(val->v_int);
    break;
  case adiak_uint:
    return py::cast((unsigned int)val->v_int);
    break;
  case adiak_double:
    return py::cast(val->v_double);
  case adiak_date: {
    std::chrono::seconds chrono_seconds(val->v_long);
    return py::cast(adiak::python::Date(
        std::chrono::system_clock::time_point(chrono_seconds)));
    break;
  }
  case adiak_timeval: {
    struct timeval* tv = static_cast<struct timeval*>(val->v_ptr);
    uint64_t num_microseconds = tv->tv_sec * 1000000 + tv->tv_usec;
    std::chrono::microseconds chrono_usec(num_microseconds);
    return py::cast(adiak::python::Timepoint(
        std::chrono::system_clock::time_point(chrono_usec)));
    break;
  }
  case adiak_version:
    return py::cast(adiak::python::Version(std::string(static_cast<const char*>(val->v_ptr))));
    break;
  case adiak_string:
    return py::cast(std::string(static_cast<const char*>(val->v_ptr)));
    break;
  case adiak_catstring:
    return py::cast(adiak::python::CatStr(std::string(static_cast<const char*>(val->v_ptr))));
    break;
  case adiak_jsonstring:
    return py::cast(adiak::python::JsonStr(std::string(static_cast<const char*>(val->v_ptr))));
    break;
  case adiak_path:
    return py::cast(adiak::python::Path(std::string(static_cast<const char*>(val->v_ptr))));
    break;
  case adiak_range: {
    if (type->num_subtypes != 2) {
      throw std::runtime_error(
          "Mismatch in Adiak between 'range' type and number of subtypes");
    }
    std::tuple<py::object, py::object> range_tuple = std::make_tuple(
        convert_adiak_value_to_python(&(val->v_subval[0]), type->subtype[0]),
        convert_adiak_value_to_python(&(val->v_subval[1]), type->subtype[1]));
    return py::cast(range_tuple);
    break;
  }
  case adiak_set: {
    if (type->num_subtypes != 1) {
      throw std::runtime_error(
          "Mismatch in Adiak between 'set' type and number of subtypes");
    }
    adiak_datatype_t *subtype = type->subtype[0];
    int num_elems =
        type->is_reference != 0 ? type->num_ref_elements : type->num_elements;
    std::set<py::object> set_val;
    for (int i = 0; i < num_elems; i++) {
      set_val.insert(
          convert_adiak_value_to_python(&(val->v_subval[i]), subtype));
    }
    return py::cast(set_val);
    break;
  }
  case adiak_list: {
    if (type->num_subtypes != 1) {
      throw std::runtime_error(
          "Mismatch in Adiak between 'list' type and number of subtypes");
    }
    adiak_datatype_t *subtype = type->subtype[0];
    int num_elems =
        type->is_reference != 0 ? type->num_ref_elements : type->num_elements;
    std::vector<py::object> list_val;
    for (int i = 0; i < num_elems; i++) {
      list_val.push_back(
          convert_adiak_value_to_python(&(val->v_subval[i]), subtype));
    }
    return py::cast(list_val);
    break;
  }
  case adiak_tuple: {
    int num_subtypes = type->num_subtypes;
    int num_elems =
        type->is_reference != 0 ? type->num_ref_elements : type->num_elements;
    if (num_subtypes != num_elems) {
      throw std::runtime_error(
          "Mismatch in Adiak between 'tuple' type and number of subtypes");
    }
    std::vector<py::object> list_val;
    for (int i = 0; i < num_elems; i++) {
      list_val.push_back(
          convert_adiak_value_to_python(&(val->v_subval[i]), type->subtype[i]));
    }
    py::tuple tup_val = py::cast(list_val);
    return tup_val;
    break;
  }
  case adiak_longlong:
    return py::cast(val->v_longlong);
    break;
  case adiak_ulonglong:
    return py::cast(val->v_longlong);
    break;
  default:
    throw std::runtime_error("Invalid datatype obtained from Adiak");
  }
}

void pyadiak_tool_nameval_cb(const char *name, int category,
                             const char *subcategory, adiak_value_t *value,
                             adiak_datatype_t *t, void *py_cb) {
  py::handle nv_cb((PyObject *)py_cb);
  PyadiakDataType py_dtype(t);
  py::object py_val = convert_adiak_value_to_python(value, t);
  nv_cb(name, category, subcategory, py_val, py_dtype);
}

void pyadiak_register_cb(int adiak_version, int category, py::function nv_cb,
                         bool report_on_all_ranks) {
  PyObject *nv_cb_obj = nv_cb.ptr();
  Py_INCREF(nv_cb_obj);
  adiak_register_cb(adiak_version, category, pyadiak_tool_nameval_cb,
                    report_on_all_ranks ? 1 : 0, (void *)nv_cb_obj);
}

void pyadiak_list_namevals(int adiak_version, int category,
                           py::function nv_cb) {
  PyObject *nv_cb_obj = nv_cb.ptr();
  Py_INCREF(nv_cb_obj);
  adiak_list_namevals(adiak_version, category, pyadiak_tool_nameval_cb,
                      (void *)nv_cb_obj);
}

std::string pyadiak_type_to_string(PyadiakDataType &type, bool long_form) {
  adiak_datatype_t *adiak_dtype = type.get_adiak_dtype();
  if (adiak_dtype == nullptr) {
    throw std::runtime_error("Provided DataType is invalid");
  }
  char *alloced_str = adiak_type_to_string(adiak_dtype, long_form ? 1 : 0);
  if (alloced_str == nullptr) {
    throw std::runtime_error("Failed to get string for type");
  }
  std::string py_str(alloced_str);
  free(alloced_str);
  return py_str;
}

std::tuple<PyadiakDataType, py::object, int, const char *>
pyadiak_get_nameval(const char *name) {
  adiak_datatype_t *dtype = nullptr;
  adiak_value_t *val = nullptr;
  int category = 0;
  const char *subcat = nullptr;
  int rc = adiak_get_nameval(name, &dtype, &val, &category, &subcat);
  if (rc != 0) {
    throw std::runtime_error("Failed to get value for name");
  }
  py::object py_val = convert_adiak_value_to_python(val, dtype);
  PyadiakDataType py_dtype(dtype);
  return std::make_tuple(py_dtype, py_val, category, subcat);
}

void create_adiak_tool_mod(py::module_ &mod) {
  py::enum_<adiak_type_t>(mod, "AdiakTypeRaw")
      .value("AdiakTypeUnset", adiak_type_unset)
      .value("AdiakTypeLong", adiak_long)
      .value("AdiakTypeULong", adiak_ulong)
      .value("AdiakTypeInt", adiak_int)
      .value("AdiakTypeUInt", adiak_uint)
      .value("AdiakTypeDouble", adiak_double)
      .value("AdiakTypeDate", adiak_date)
      .value("AdiakTypeTimeval", adiak_timeval)
      .value("AdiakTypeVersion", adiak_version)
      .value("AdiakTypeString", adiak_string)
      .value("AdiakTypeCatString", adiak_catstring)
      .value("AdiakTypePath", adiak_path)
      .value("AdiakTypeRange", adiak_range)
      .value("AdiakTypeSet", adiak_set)
      .value("AdiakTypeList", adiak_list)
      .value("AdiakTypeTuple", adiak_tuple)
      .value("AdiakTypeLongLong", adiak_longlong)
      .value("AdiakTypeULongLong", adiak_ulonglong)
      .value("AdiakTypeJsonString", adiak_jsonstring)
      .export_values();
  py::enum_<adiak_numerical_t>(mod, "AdiakNumerical")
      .value("AdiakNumericalUnset", adiak_numerical_unset)
      .value("AdiakNumericalCategorical", adiak_categorical)
      .value("AdiakNumericalOrdinal", adiak_ordinal)
      .value("AdiakNumericalInterval", adiak_interval)
      .value("AdiakNumericalRational", adiak_rational)
      .export_values();
  py::class_<PyadiakDataType>(mod, "AdiakDataType")
      .def(py::init<>())
      .def("get_dtype", &PyadiakDataType::get_dtype)
      .def("get_numerical", &PyadiakDataType::get_numerical)
      .def("get_num_elems", &PyadiakDataType::get_num_elems)
      .def("get_num_subtypes_in_container",
           &PyadiakDataType::get_num_subtypes_in_container)
      .def("is_reference", &PyadiakDataType::is_reference)
      .def("get_subtypes", &PyadiakDataType::get_subtypes);
  mod.def("register_cb", &pyadiak_register_cb);
  mod.def("list_namevals", &pyadiak_list_namevals);
  mod.def("type_to_string", &pyadiak_type_to_string);
  mod.def("get_nameval", &pyadiak_get_nameval);
}

} // namespace python
} // namespace adiak
