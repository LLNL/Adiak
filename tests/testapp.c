#include "adiak.h"
#include "testlib.c"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#if defined(USE_MPI)
#include <mpi.h>
#endif

void dowork(struct timeval start)
{
   int result;
   struct timeval end;

   double gridarray[4] = { 4.5, 1.18, 0.24, 8.92 };
   float floaties[4] = { 1.5, 2.5, 3.5, 4.5 };
   unsigned char itsybitsyints[4] = { 2, 3, 4, 5 };

   result = adiak_namevalue("compiler", adiak_general, NULL, "%s", "gcc@8.1.0");
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_namevalue("gridvalues2", adiak_general, NULL, "[%f]", gridarray, 4);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_namevalue("problemsize", adiak_general, NULL, "%lu", 14000);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_namevalue("countdown", adiak_general, NULL, "%lld", 9876543210);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_namevalue("floaties", adiak_general, NULL, "[%f32]", floaties, 4);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_namevalue("itsybitsyints", adiak_general, NULL, "[%u8]", itsybitsyints, 4);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_walltime();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_cputime();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_systime();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_collect_all();
   if (result != 0) printf("return %d\n\n", result);

   gettimeofday(&end, NULL);
   struct timeval *timerange[2] = { &start, &end };
   result = adiak_namevalue("endtime", adiak_general, NULL, "%t", timerange[1]);
   result = adiak_namevalue("computetime", adiak_performance, NULL, "<%t>", timerange);

   result = adiak_flush("stdout");
   if (result != 0) printf("return: %d\n\n", result);
}

int main(
#if defined(USE_MPI)
         int argc, char *argv[]
#else
         int UNUSED(argc), char **UNUSED(argv)
#endif
         )
{
#if defined(USE_MPI)
   MPI_Comm world = MPI_COMM_WORLD;
#endif
   struct timeval start;
   gettimeofday(&start, NULL);

#if defined(USE_MPI)
   MPI_Init(&argc, &argv);
   adiak_init(&world);
#else
   adiak_init(NULL);
#endif

   dowork(start);
   dowork(start);

   adiak_fini();
   adiak_clean();

#if defined(USE_MPI)
   MPI_Finalize();
#endif
   return 0;
}
