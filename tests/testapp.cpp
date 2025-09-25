#include "adiak.hpp"
#include <vector>
#include <set>
#include <cstdio>
#include <time.h>
#include <sys/time.h>


#if defined(USE_MPI)
#include <mpi.h>
#endif

using namespace std;

void dowork(struct timeval
#if defined(USE_MPI)
            start
#endif
  )
{
   bool result;

   vector<double> grid;
   grid.push_back(4.5);
   grid.push_back(1.18);
   grid.push_back(0.24);
   grid.push_back(8.92);

   set<string> names;
   names.insert("matt");
   names.insert("david");
   names.insert("greg");

   vector<tuple<string, double, double> > points;

   points.push_back(make_tuple("first", 1.0, 1.0));
   points.push_back(make_tuple("second", 2.0, 4.0));
   points.push_back(make_tuple("third", 3.0, 9.0));

   vector<string> ap_a;
   ap_a.push_back("first");
   ap_a.push_back("second");
   ap_a.push_back("third");
   vector<double> ap_b;
   ap_b.push_back(1.0);
   ap_b.push_back(2.0);
   ap_b.push_back(3.0);
   vector<double> ap_c;
   ap_c.push_back(1.0);
   ap_c.push_back(4.0);
   ap_c.push_back(9.0);
   tuple<vector<string>, vector<double>, vector<double> > antipoints = make_tuple(ap_a, ap_b, ap_c);
   const tuple<vector<string>, vector<double>, vector<double> > &antipoints_r = antipoints;

   result = adiak::value("points", points);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("antipoints", antipoints_r);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("antipoints_r", antipoints_r);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("str", std::string("s"));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("compiler", adiak::version("gcc@8.1.0"));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("mydouble", (double) 3.14);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("gridvalues", grid);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("problemsize", 14000);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("countdown", static_cast<long long>(9876543210));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("allnames", names);
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("githash", adiak::catstring("a0c93767478f23602c2eb317f641b091c52cf374"));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("birthday", adiak::date(286551000));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::value("nullpath", adiak::path("/dev/null"));
   if (!result) printf("return: %d\n\n", result);

   result = adiak::collect_all();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::walltime();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::cputime();
   if (!result) printf("return: %d\n\n", result);

   result = adiak::systime();
   if (!result) printf("return: %d\n\n", result);
/* struct timeval end;
   gettimeofday(&end, NULL);
   result = adiak::value("computetime", &start, &end);
   if (!result) printf("return: %d\n\n", result);*/

   array<float, 3> floatar;
   floatar[0] = 0.01f;
   floatar[1] = 0.02f;
   floatar[2] = 0.03f;
   result = adiak::value("floats", floatar);
   if (!result) printf("return: %d\n\n", result);
}

/* Somehow the testlib constructor doesn't work in the C++ app
   so we just call the print function directly.
 */
extern "C"
{
void print_all_adiak_vars();
}

int main(
#if defined(USE_MPI)
  int argc, char *argv[]
#else
  int, char **
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
   adiak::init(&world);
#else
   adiak::init(NULL);
#endif

   dowork(start);
   print_all_adiak_vars();

   adiak::fini();
   adiak::clean();
#if defined(USE_MPI)
   MPI_Finalize();
#endif
   return 0;
}
