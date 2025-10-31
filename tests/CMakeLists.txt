include_directories(.)
include_directories(../include)

blt_add_library( NAME testlib
                 SOURCES testlib.c
                 DEFINES "TOOLNAME=TOOL3"
                 DEPENDS_ON adiak)

blt_add_executable( NAME testapp
                    SOURCES testapp.c
                    DEPENDS_ON testlib adiak )
blt_add_executable( NAME testapp-cxx
                    SOURCES testapp.cpp
                    DEPENDS_ON testlib adiak )
blt_add_executable( NAME testapp-zerocopy
                    SOURCES test_zerocopy.c
                    DEPENDS_ON testlib adiak )

blt_add_executable(NAME test_adiak
    SOURCES test_application-api.cpp 
    DEPENDS_ON adiak gtest)
blt_add_test(NAME test_adiak
    COMMAND test_adiak)

# Python testing
if (ENABLE_PYTHON_BINDINGS)
  find_package(Python COMPONENTS Interpreter REQUIRED)

  set(_py_root ${CMAKE_BINARY_DIR}/src/interface/python)
  set(_py_pkg  ${_py_root}/pyadiak)
  set(_libdir  ${CMAKE_BINARY_DIR}/lib)

  # Serial (works with ENABLE_MPI=OFF)
  blt_add_test(
    NAME test_python_serial
    COMMAND
      ${CMAKE_COMMAND} -E env
        PYTHONPATH=${_py_root}:$ENV{PYTHONPATH}
        LD_LIBRARY_PATH=${_libdir}:$ENV{LD_LIBRARY_PATH}
        ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testapp.py
    DEPENDS_ON __pyadiak_impl
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  if(ENABLE_MPI)
    blt_add_test(
      NAME test_python_mpi
      COMMAND
        ${CMAKE_COMMAND} -E env
          ADIAK_TEST_MPI=1
          PYTHONPATH=${_py_root}:$ENV{PYTHONPATH}
          LD_LIBRARY_PATH=${_libdir}:$ENV{LD_LIBRARY_PATH}
          ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testapp.py
      NUM_MPI_TASKS 2
      DEPENDS_ON __pyadiak_impl
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  endif()
endif()