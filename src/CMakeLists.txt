include_directories(.)
include_directories(../include)

set(adiak_public_headers ../include/adiak.h ../include/adiak.hpp ../include/adiak_internal.hpp ../include/adiak_tool.h)

if (APPLE)
set(adiak_sys_sources adksys_posix.c adksys_unix.c adksys_osx.c)
elseif (UNIX)
set(adiak_sys_sources adksys_glibc.c adksys_posix.c adksys_procfs.c adksys_unix.c)
endif ()

if (MPI_FOUND)
set(adiak_mpi_source adksys_mpi.c)
add_definitions("-DUSE_MPI")
set(adiak_mpi_depends mpi)
else()
set(adiak_mpi_source)
set(adiak_mpi_depends)
endif()

blt_add_library( NAME adiak
                 HEADERS ${adiak_public_headers}
                 SOURCES adiak.c adiak_kokkos.cpp ${adiak_sys_sources} ${adiak_mpi_source}
                 DEPENDS_ON ${adiak_mpi_depends})

install(FILES
        ${adiak_public_headers}
        DESTINATION include)

install(TARGETS
  adiak
  EXPORT adiak-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT adiak-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/adiak)
