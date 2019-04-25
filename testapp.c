#include "adiak.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
   int result;
   MPI_Comm world = MPI_COMM_WORLD;
   struct timeval start, end;

   gettimeofday(&start, NULL);

   MPI_Init(&argc, &argv);
   
   adiak_init(&world);
   double gridarray[] = { 4.5, 1.18, 0.24, 8.92 };

   result = adiak_value("compiler", adiak_general, "%s", "gcc@8.1.0");
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_value("gridvalues", adiak_general, "{%f}", 4, 5.4, 18.1, 24.0, 92.8);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_value("gridvalues2", adiak_general, "[%f]", sizeof(gridarray)/sizeof(*gridarray), gridarray);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_value("problemsize", adiak_general, "%lu", 14000);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_user();
   if (result != 0) printf("return: %d\n\n", result);
   
   result = adiak_uid();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_launchdate();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_executable();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_cmdline();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_hostname();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_clustername();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_runtime();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_iotime();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_mpi_ranks();
   if (result != 0) printf("return: %d\n\n", result);

  result = adiak_hostlist();
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_mpitime();
   if (result != 0) printf("return: %d\n\n", result);      

   gettimeofday(&end, NULL);
   result = adiak_value("computetime", adiak_performance, "-%t-", &start, &end);
   
   MPI_Finalize();
   return 0;
}
