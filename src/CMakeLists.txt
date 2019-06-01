include_directories(.)
include_directories(../include)

set(adiak_public_headers ../include/adiak.h ../include/adiak.hpp ../include/adiak_internal.hpp ../include/adiak_tool.h)

if (APPLE)
set(adiak_sys_sources adksys_posix.c adksys_unix.c adksys_osx.c)
elseif (UNIX)
set(adiak_sys_sources adksys_glibc.c adksys_posix.c adksys_procfs.c adksys_unix.c)
endif ()

blt_add_library( NAME adiak
                 HEADERS ${adiak_public_headers}
                 SOURCES adiak.c ${adiak_sys_sources} )

install(FILES
        ${adiak_public_headers}
        DESTINATION include)

install(TARGETS
        adiak
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
