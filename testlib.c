#include "adiak_tool.h"

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

static void cb(const char *name, void *elems, size_t elem_size, size_t num_elems, adiak_datatype_t valtype, void *opaque);

#define XSTR(S) #S
#define STR(S) XSTR(S)

static void onload() __attribute__((constructor));
static void onload()
{
   adiak_register_cb(1, adiak_category_all, cb, 0, NULL);
}

static void cb(const char *name, void *elems, size_t elem_size, size_t num_elems, adiak_datatype_t valtype, void *opaque)
{
   int i;
   printf("%s/%s: ", STR(TOOLNAME), name);

   if (valtype.grouping == adiak_set)
      printf("[");
#define PRINT_ELEM(TOKEN, TYPE)  printf(TOKEN, ((TYPE *) elems)[i])
   for (i = 0; i < num_elems; i++) {
      if (valtype.dtype == adiak_long)
         PRINT_ELEM("%ld", long);
      else if (valtype.dtype == adiak_ulong)
         PRINT_ELEM("%lu", unsigned long);
      else if (valtype.dtype == adiak_int)
         PRINT_ELEM("%d", int);
      else if (valtype.dtype == adiak_uint)
         PRINT_ELEM("%u", unsigned int);
      else if (valtype.dtype == adiak_double)
         PRINT_ELEM("%f", double);
      else if (valtype.dtype == adiak_date) {
         char datestr[512];
         signed long seconds_since_epoch = ((signed long *) elems)[i];
         struct tm *loc = localtime(&seconds_since_epoch);
         strftime(datestr, sizeof(datestr), "%a, %d %b %Y %T %z", loc);
         printf("%s", datestr);
      }
      else if (valtype.dtype == adiak_timeval) {
         struct timeval *val = ((struct timeval **) elems)[i];
         double duration = val->tv_sec + (val->tv_usec / 1000000.0);
         printf("%fs", duration);
      }
      else if (valtype.dtype == adiak_version)
         PRINT_ELEM("%s", char *);
      else if (valtype.dtype == adiak_string)
         PRINT_ELEM("%s", char *);
      else if (valtype.dtype == adiak_catstring)
         PRINT_ELEM("%s", char *);
      else if (valtype.dtype == adiak_path)
         PRINT_ELEM("%s", char *);

      if (valtype.grouping == adiak_range && i == 0)
         printf(" - ");
      else if (i != num_elems-1)
         printf(", ");
   }
   if (valtype.grouping == adiak_set)
      printf("]");
   printf("\n");
}
