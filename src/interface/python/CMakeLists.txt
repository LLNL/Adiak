set(PYADIAK_BINDING_SOURCES
    mod.cpp
    pyadiak_tool.cpp
    pyadiak.cpp
    types.cpp
)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

set(PYADIAK_SYSCONFIG_SCHEME "posix_user" CACHE STRING "Scheme used for searching for pyadiak's install path. Valid options can be determined with 'sysconfig.get_scheme_names()'")

execute_process(
  COMMAND
    ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cmake/get_python_install_paths.py purelib ${PYADIAK_SYSCONFIG_SCHEME}
  OUTPUT_VARIABLE
    PYADIAK_SITELIB)
execute_process(
  COMMAND
    ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cmake/get_python_install_paths.py platlib ${PYADIAK_SYSCONFIG_SCHEME}
  OUTPUT_VARIABLE
    PYADIAK_SITEARCH)

set(PYADIAK_SITELIB "${PYADIAK_SITELIB}/pyadiak")
set(PYADIAK_SITEARCH "${PYADIAK_SITEARCH}/pyadiak")

pybind11_add_module(__pyadiak_impl ${PYADIAK_BINDING_SOURCES})
target_link_libraries(__pyadiak_impl PUBLIC adiak)
target_compile_features(__pyadiak_impl PUBLIC cxx_std_11)
target_include_directories(__pyadiak_impl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
execute_process(
  COMMAND python3 -c "import mpi4py, os; print(os.path.join(os.path.dirname(mpi4py.__file__), 'include'))"
  OUTPUT_VARIABLE MPI4PY_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
include_directories(${MPI4PY_INCLUDE_DIR})

add_custom_target(
    pyadiak_py_source_copy ALL # Always build pycaliper_test
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/pyadiak
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/pyadiak ${CMAKE_CURRENT_BINARY_DIR}/pyadiak
    COMMENT "Copying pyadiak Python source to ${CMAKE_CURRENT_BINARY_DIR}/pyadiak"
)
add_dependencies(__pyadiak_impl pyadiak_py_source_copy)

install(
    DIRECTORY
        pyadiak/
    DESTINATION
        ${PYADIAK_SITELIB}
)

install(
    TARGETS
        __pyadiak_impl
    ARCHIVE DESTINATION
        ${PYADIAK_SITEARCH}
    LIBRARY DESTINATION
        ${PYADIAK_SITEARCH}
)

# Put the compiled extension inside the pyadiak package in the build tree
set(_py_pkg_dir "${CMAKE_BINARY_DIR}/src/interface/python/pyadiak")

set_target_properties(__pyadiak_impl PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY  "${_py_pkg_dir}"   # where the .so (MODULE/LIBRARY) goes
  RUNTIME_OUTPUT_DIRECTORY  "${_py_pkg_dir}"
  ARCHIVE_OUTPUT_DIRECTORY  "${_py_pkg_dir}"
)
