.PHONY: default

CC = mpicc
CFLAGS = -fPIC -g -Wall -DUSE_MPI

default: testapp

adiak.o: adiak.c adiak.h adiak_tool.h
	$(CC) -o $@ $(CFLAGS) -c $<

testapp.o: testapp.c adiak.h
	$(CC) -o $@ $(CFLAGS) -c $<

testlib1.o: testlib.c adiak_tool.h adiak.h
	$(CC) -o $@ $(CFLAGS) -c $< -DTOOLNAME=TOOL1

testlib2.o: testlib.c adiak_tool.h adiak.h
	$(CC) -o $@ $(CFLAGS) -c $< -DTOOLNAME=TOOL2

testlib3.o: testlib.c adiak_tool.h adiak.h
	$(CC) -o $@ $(CFLAGS) -c $< -DTOOLNAME=TOOL3

libtest1.so: testlib1.o adiak.o
	$(CC) -o $@ -shared $(LDFLAGS) $^

libtest2.so: testlib2.o adiak.o
	$(CC) -o $@ -shared $(LDFLAGS) $^

libtest3.so: testlib3.o adiak.o
	$(CC) -o $@ -shared $(LDFLAGS) $^

testapp: testapp.o adiak.o libtest1.so libtest2.so libtest3.so
	$(CC) -o $@ $(LDFLAGS) -L. -Wl,-rpath,$(PWD) -ltest1 -ltest2 -ltest3 -ldl $^

clean:
	rm -f testapp libtest?.so testlib?.o testapp.o adiak.o
