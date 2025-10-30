set(adiak_public_headers
  ../include/adiak.h
  ../include/adiak.hpp
  ../include/adiak_internal.hpp
  ../include/adiak_tool.h)

set(adiak_sources
  adiak.c)

if (APPLE)
  list(APPEND adiak_sources
    adksys_posix.c
    adksys_unix.c
    adksys_osx.c)
elseif (UNIX)
  list(APPEND adiak_sources
    adksys_glibc.c
    adksys_posix.c
    adksys_procfs.c
    adksys_unix.c)
  list(APPEND adiak_dependencies
    ${CMAKE_DL_LIBS})
endif ()

if (MPI_FOUND)
  list(APPEND adiak_sources
    adksys_mpi.c)
  add_definitions("-DUSE_MPI")
  list(APPEND adiak_dependencies
    mpi)
  list(APPEND adiak_export_targets
    mpi)
endif()

blt_add_library( NAME adiak
                 HEADERS ${adiak_public_headers}
                 SOURCES ${adiak_sources}
                 DEPENDS_ON ${adiak_dependencies})

target_include_directories(adiak
  PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")

install(FILES
        ${adiak_public_headers}
        DESTINATION include)

install(TARGETS
  adiak ${adiak_export_targets}
  EXPORT adiak-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT adiak-targets NAMESPACE adiak:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/adiak)

if (ENABLE_PYTHON_BINDINGS)
  add_subdirectory(interface/python)
  if (MPI_FOUND)
    find_package(MPI REQUIRED CXX)
  endif()
endif()
