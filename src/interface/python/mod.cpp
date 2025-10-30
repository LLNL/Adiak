#include "pyadiak.hpp"
#include "pyadiak_tool.hpp"

PYBIND11_MODULE(__pyadiak_impl, m) {
  // TODO add version

  auto types_module =
      m.def_submodule("types", "Submodule for type wrappers in Adiak");
  adiak::python::create_adiak_types_mod(types_module);

  auto annotation_module =
      m.def_submodule("annotations", "Submodule for annotation APIs");
  adiak::python::create_adiak_annotation_mod(annotation_module);

  auto tool_module = m.def_submodule("tools", "Submodule for tool APIs");
  adiak::python::create_adiak_tool_mod(tool_module);
}