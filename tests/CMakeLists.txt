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