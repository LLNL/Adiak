.PHONY: default

CC = gcc
CXX = g++
CFLAGS = -fPIC -g -Wall
CXXFLAGS = $(CFLAGS) -std=c++11

SYSOBJ = sys_posix_common.o sys_linux.o
# SYSOBJ = sys_posix_common.o sys_posix.o

default: testapp testappcxx

adiak.o: adiak.c adiak.h adiak_internal.h adiak_tool.h
	$(CC) -o $@ $(CFLAGS) -c $<

sys_posix_common.o: sys_posix_common.c adiak.h adiak_internal.h
	$(CC) -o $@ $(CFLAGS) -c $<

sys_linux.o: sys_linux.c adiak.h adiak_internal.h
	$(CC) -o $@ $(CFLAGS) -c $<

testapp.o: testapp.c adiak.h
	$(CC) -o $@ $(CFLAGS) -c $<

testappcxx.o: testapp.cpp adiak.hpp adiak_internal.hpp adiak.h
	$(CXX) -o $@ $(CXXFLAGS) -c $<

testlib1.o: testlib.c adiak_tool.h adiak.h
	$(CC) -o $@ $(CFLAGS) -c $< -DTOOLNAME=TOOL1

testlib2.o: testlib.c adiak_tool.h adiak.h
	$(CC) -o $@ $(CFLAGS) -c $< -DTOOLNAME=TOOL2

testlib3.o: testlib.c adiak_tool.h adiak.h
	$(CC) -o $@ $(CFLAGS) -c $< -DTOOLNAME=TOOL3

libtest1.so: testlib1.o adiak.o adiak.o $(SYSOBJ)
	$(CC) -o $@ -shared $(LDFLAGS) $^

libtest2.so: testlib2.o adiak.o adiak.o $(SYSOBJ)
	$(CC) -o $@ -shared $(LDFLAGS) $^

libtest3.so: testlib3.o adiak.o adiak.o $(SYSOBJ)
	$(CC) -o $@ -shared $(LDFLAGS) $^

testapp: testapp.o libtest1.so libtest2.so libtest3.so
	$(CC) -o $@ $(LDFLAGS) -L. -Wl,-rpath,$(PWD) -ltest1 -ltest2 -ltest3 -ldl testapp.o adiak.o $(SYSOBJ)


testappcxx: testappcxx.o adiak.o libtest1.so libtest2.so libtest3.so
	$(CXX) -o $@ $(LDFLAGS) -L. -Wl,-rpath,$(PWD) -ltest1 -ltest2 -ltest3 -ldl testappcxx.o adiak.o $(SYSOBJ)

clean:
	rm -f testapp libtest?.so testlib?.o testapp.o adiak.o adiakcxx.o testapp.cxx testappcxx.o $(SYSOBJ) testappcxx 
