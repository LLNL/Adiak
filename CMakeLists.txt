cmake_minimum_required(VERSION 3.12)
project (Adiak
         LANGUAGES CXX C
         VERSION 0.5.0)

set(BLT_CXX_STD "c++14" CACHE STRING "Version of C++ standard")

if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif()

# Find Python by location
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
  cmake_policy(SET CMP0094 NEW)
endif()

set(BLT_EXPORT_THIRDPARTY ON CACHE BOOL "")

if (DEFINED BLT_SOURCE_DIR)
    # Support having a shared BLT outside of the repository if given a BLT_SOURCE_DIR
    if (NOT EXISTS ${BLT_SOURCE_DIR}/SetupBLT.cmake)
        message(FATAL_ERROR "Given BLT_SOURCE_DIR does not contain SetupBLT.cmake")
    endif()
else()
    # Use internal BLT if no BLT_SOURCE_DIR is given
    set(BLT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/blt" CACHE PATH "")
    if (NOT EXISTS ${BLT_SOURCE_DIR}/SetupBLT.cmake)
        message(FATAL_ERROR
            "The BLT git submodule is not present. "
            "Either run the following two commands in your git repository: \n"
            "    git submodule init\n"
            "    git submodule update\n"
            "Or add -DBLT_SOURCE_DIR=/path/to/blt to your CMake command." )
    endif()
endif()

include(${BLT_SOURCE_DIR}/SetupBLT.cmake)

option(ENABLE_PYTHON_BINDINGS "Build/Install Python bindings for Adiak" FALSE)

if (ENABLE_PYTHON_BINDINGS)
  find_package(Python COMPONENTS Interpreter Development REQUIRED)
  find_package(pybind11 CONFIG REQUIRED)
  
  if (pybind11_FOUND)
    set(ADIAK_HAVE_PYBIND11 TRUE)
  else()
    message(WARNING "Python bindings requested, but pybind11 not found")
  endif()
endif()

add_subdirectory(src)
if (ENABLE_TESTS)
  add_subdirectory(tests)
endif()

if (ENABLE_DOCS)
  add_subdirectory(docs)
endif()

include(CMakePackageConfigHelpers)

# Configure adiak-config-version.cmake
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/adiak-config-version.cmake"
  VERSION ${LIBTOOL_INTERFACE}.${LIBTOOL_REVISION}.${LIBTOOL_AGE}
  COMPATIBILITY SameMajorVersion)

set(adiak_INSTALL_INCLUDE_DIR include/)

# Configure adiak-config.cmake
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/adiak-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/adiak"
  PATH_VARS adiak_INSTALL_INCLUDE_DIR)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/adiak-config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/adiak-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/adiak")
