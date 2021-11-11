include_directories(../include)

if (MPI_FOUND)
add_definitions("-DUSE_MPI")
endif()

blt_add_library(NAME testlib 
                SOURCES testlib.c
                DEPENDS_ON adiak)

blt_add_executable(NAME adiakctest
                   SOURCES testapp.c
                   DEPENDS_ON adiak testlib)

blt_add_executable(NAME adiakcxxtest
                   SOURCES testapp.cpp
                   DEPENDS_ON adiak testlib)
