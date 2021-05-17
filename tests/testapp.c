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

   result = adiak_namevalue("compiler", adiak_general, NULL, "%s", "gcc@8.1.0");
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_namevalue("gridvalues2", adiak_general, NULL, "[%f]", gridarray, 4);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_namevalue("problemsize", adiak_general, NULL, "%lu", 14000);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_namevalue("countdown", adiak_general, NULL, "%lld", 9876543210);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_user();
   if (result != 0) printf("return: %d\n\n", result);
   
   result = adiak_uid();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_launchdate();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_executable();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_libraries();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_cmdline();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_hostname();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_clustername();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_walltime();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_cputime();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_systime();
   if (result != 0) printf("return: %d\n\n", result);   

#if defined(USE_MPI)
   result = adiak_job_size();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_hostlist();
   if (result != 0) printf("return: %d\n\n", result);
#endif
   
   gettimeofday(&end, NULL);
   struct timeval *timerange[2];
   timerange[0] = &start;
   timerange[1] = &end;
   result = adiak_namevalue("computetime", adiak_performance, NULL, "<%t>", &timerange);

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

   adiak_clean();

   adiak_fini();

#if defined(USE_MPI)
   MPI_Finalize();
#endif
   return 0;
}
