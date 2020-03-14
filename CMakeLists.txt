cmake_minimum_required(VERSION 3.8)
project (Adiak
         LANGUAGES CXX C
         VERSION 0.2)

set(BLT_CXX_STD "c++11" CACHE STRING "Version of C++ standard")

if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif()


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

add_subdirectory(src)
add_subdirectory(tests)

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
