#include "adiak.h"
#include "testlib.c"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#if defined(USE_MPI)
#include <mpi.h>
#endif

static const int array[9] = { -1, 2, -3, 4, 5, 4, 3, 2, 1 };

static const char* string_a = "One";
static const char* string_b = "Two";

static float floaties[4] = { 1.5, 2.5, 3.5, 4.5 };

static unsigned char itsybitsyints[4] = { 2, 3, 4, 5 };

/* Structs/tuples are super iffy!
 * There is no reliable way to determine their actual memory layout in adiak.
 * Using two 64-bit elements here which has a decent chance of working.
 */
struct int_string_tuple {
    long long i; const char* str;
};

static const struct int_string_tuple hello_data[3] = {
    { 1, "Hello" }, { 2, "Adiak" }, { 9876543210, "!" }
};

void dowork()
{
   int result;

    /* zero-copy list of ints */
   result = adiak_namevalue("array_zerocopy", adiak_general, NULL, "&{%d}", array, 9);
   if (result != 0) printf("return: %d\n\n", result);

    /* zero-copy list of u8s */
   result = adiak_namevalue("itsybitsyints_zerocopy", adiak_general, NULL, "&{%u8}", itsybitsyints, 4);
   if (result != 0) printf("return: %d\n\n", result);

    /* zero-copy list of floats */
   result = adiak_namevalue("floaties_zerocopy", adiak_general, NULL, "&{%f32}", floaties, 4);
   if (result != 0) printf("return: %d\n\n", result);

   /* a single zero-copy string */
   result = adiak_namevalue("one_zerocopy_string", adiak_general, NULL, "&%s", string_a);
   if (result != 0) printf("return: %d\n\n", result);

   /* set of zero-copy strings (shallow-copies the list but not the strings)*/
   const char* strings[2] = { string_a, string_b };
   result = adiak_namevalue("strings_zerocopy", adiak_general, NULL, "[&%s]", strings, 2);
   if (result != 0) printf("return: %d\n\n", result);

   /* full deep copy list of tuples */
   result = adiak_namevalue("hello_deepcopy", adiak_general, NULL, "{(%lld,%s)}", hello_data, 3, 2);
   if (result != 0) printf("return: %d\n\n", result);

   /* zero-copy list of tuples */
   result = adiak_namevalue("hello_zerocopy_0", adiak_general, NULL, "&{(%lld,%s)}", hello_data, 3, 2);
   if (result != 0) printf("return: %d\n\n", result);

   /* shallow-copy list of zero-copy tuples
    * Note this still implies a list of structs, *not* a list of pointers to structs
    */
   result = adiak_namevalue("hello_zerocopy_1", adiak_general, NULL, "{&(%lld,%s)}", hello_data, 3, 2);
   if (result != 0) printf("return: %d\n\n", result);

   /* shallow-copy list of tuples with zero-copy strings */
   result = adiak_namevalue("hello_zerocopy_2", adiak_general, NULL, "{(%lld,&%s)}", hello_data, 3, 2);
   if (result != 0) printf("return: %d\n\n", result);

   result = adiak_launchdate();
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

#if defined(USE_MPI)
   MPI_Init(&argc, &argv);
   adiak_init(&world);
#else
   adiak_init(NULL);
#endif

   dowork();

   int result = adiak_flush("stdout");
   if (result != 0) printf("return: %d\n\n", result);

   adiak_fini();
   adiak_clean();

#if defined(USE_MPI)
   MPI_Finalize();
#endif
   return 0;
}
