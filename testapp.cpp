#include "adiak.hpp"
#include <vector>
#include <set>

#include <time.h>
#include <sys/time.h>


#if defined(USE_MPI)
extern "C" {
#include <mpi.h>
}
#endif

using namespace std;

int main(int argc, char *argv[])
{
   bool result;
   MPI_Comm world = MPI_COMM_WORLD;
   struct timeval start, end;

   gettimeofday(&start, NULL);

   MPI_Init(&argc, &argv);
   
   adiak::init(&world);

   vector<double> grid;
   grid.push_back(4.5);
   grid.push_back(1.18);
   grid.push_back(0.24);
   grid.push_back(8.92);

   set<string> names;
   names.insert("matt");
   names.insert("david");
   names.insert("greg");

   result = adiak::value("compiler", adiak::version("gcc@8.1.0"));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("gridvalues", grid);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("problemsize", 14000);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("allnames", names);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("githash", adiak::catstring("a0c93767478f23602c2eb317f641b091c52cf374"));
   if (!result) printf("return: %d\n\n", result);
   
   adiak_datatype_t customtype = {adiak_categorical, adiak_point, adiak_double, adiak_performance};
   result = adiak::value("unsortable_double", 0.123, customtype);
   if (!result) printf("return: %d\n\n", result);   
   
   result = adiak::value("birthday", adiak::date(286551000));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("nullpath", adiak::path("/dev/null"));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::user();
   if (!result) printf("return: %d\n\n", result);
   
   result = adiak::uid();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::launchdate();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::executable();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::executablepath();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::libraries();
   if (!result) printf("return: %d\n\n", result);
   
   result = adiak::cmdline();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::hostname();
   if (!result) printf("return: %d\n\n", result);

//   result = adiak::clustername();
//   if (!result) printf("return: %d\n\n", result);

   result = adiak::walltime();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::cputime();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::systime();
   if (!result) printf("return: %d\n\n", result);   

   result = adiak::jobsize();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::hostlist();
   if (!result) printf("return: %d\n\n", result);

   gettimeofday(&end, NULL);
   result = adiak::value("computetime", &start, &end);

   adiak::fini();
   MPI_Finalize();
   return 0;
}
