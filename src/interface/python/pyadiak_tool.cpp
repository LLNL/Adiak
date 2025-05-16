#include "pyadiak_tool.hpp"
#include "adiak_tool.h"

namespace adiak {
namespace python {

void pyadiak_tool_nameval_cb(const char *name, int category,
                             const char *subcategory, adiak_value_t *value,
                             adiak_datatype_t *t, void *py_cb) {
  py::handle nv_cb((PyObject *)py_cb);
  // TODO add any necessary conversion from adiak_value_t and adiak_datatype_t
  // to correct Python-compatible types
  nv_cb(name, category, subcategory, value, t);
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

std::string pyadiak_type_to_string(adiak_datatype_t *t, bool long_form) {
  // TODO implement once I figure out how to handle dtypes
  return std::string("");
}

std::tuple<adiak_datatype_t *, adiak_value_t *, int, const char *>
pyadiak_get_nameval(const char *name) {
  // TODO implement once I figure out how to handle dtypes and values
}

int pyadiak_num_subvals(adiak_datatype_t *t) {
  // TODO implement once I figure out how to handle dtypes
  return 0;
}

} // namespace python
} // namespace adiak
