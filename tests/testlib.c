#if defined(_MSC_VER)
#pragma warning(disable : 4100)
#pragma warning(disable : 4996)
#endif

#include "adiak_tool.h"

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define XSTR(S) #S
#define STR(S) XSTR(S)

#if !defined(TOOLNAME)
#define TOOLNAME tool
#endif

static void print_value(adiak_value_t *val, adiak_datatype_t *t)
{
   if (!t)
      printf("ERROR");
   switch (t->dtype) {
      case adiak_type_unset:
         printf("UNSET");
         break;
      case adiak_long:
         printf("%ld", val->v_long);
         break;
      case adiak_ulong:
         printf("%lu", (unsigned long) val->v_long);
         break;
      case adiak_int:
         printf("%d", val->v_int);
         break;
      case adiak_uint:
         printf("%u", (unsigned int) val->v_int);
         break;         
      case adiak_double:
         printf("%f", val->v_double);
         break;
      case adiak_date: {
         char datestr[512];
         signed long seconds_since_epoch = (signed long) val->v_long;
#if defined(_MSC_VER)
         __time64_t s = (__time64_t) seconds_since_epoch;       
         struct tm *loc, loc_tmp;
         _localtime64_s(&loc_tmp, &s);
         loc = &loc_tmp;
#else
         struct tm *loc = localtime(&seconds_since_epoch);
#endif
         strftime(datestr, sizeof(datestr), "%a, %d %b %Y %T %z", loc);
         printf("%s", datestr);
         break;
      }
      case adiak_timeval: {
         struct timeval *tval = (struct timeval *) val->v_ptr;
         double duration = tval->tv_sec + (tval->tv_usec / 1000000.0);
         printf("%fs (timeval)", duration);
         break;
      }
      case adiak_version: {
         char *s = (char *) val->v_ptr;
         printf("%s (version)", s);
         break;
      }
      case adiak_string: {
         char *s = (char *) val->v_ptr;
         printf("%s (string)", s);
         break;
      }      
      case adiak_catstring: {
         char *s = (char *) val->v_ptr;
         printf("%s (catstring)", s);
         break;
      }
      case adiak_path: {
         char *s = (char *) val->v_ptr;
         printf("%s (path)", s);
         break;
      }
      case adiak_range: {
         adiak_value_t *subvals = (adiak_value_t *) val->v_ptr;
         print_value(subvals+0, t->subtype[0]);
         printf(" - ");
         print_value(subvals+1, t->subtype[0]);
         break;
      }
      case adiak_set: {
         adiak_value_t *subvals = (adiak_value_t *) val->v_ptr;
         int i;
         printf("[");
         for (i = 0; i < t->num_elements; i++) {
            print_value(subvals + i, t->subtype[0]);
            if (i+1 != t->num_elements)
               printf(", ");
         }
         printf("]");
         break;
      }
      case adiak_list: {
         adiak_value_t *subvals = (adiak_value_t *) val->v_ptr;
         int i;
         printf("{");
         for (i = 0; i < t->num_elements; i++) {
            print_value(subvals + i, t->subtype[0]);
            if (i+1 != t->num_elements)
               printf(", ");
         }
         printf("}");
         break;
      }
      case adiak_tuple: {
         adiak_value_t *subvals = (adiak_value_t *) val->v_ptr;
         int i;
         printf("<");
         for (i = 0; i < t->num_elements; i++) {
            print_value(subvals + i, t->subtype[i]);
            if (i+1 != t->num_elements)
               printf(", ");
         }
         printf(">");
         break;
      }
   }
}

static void print_nameval(const char *name, int category, const char *subcategory, adiak_value_t *value, adiak_datatype_t *t, void *opaque_value)
{
   printf("%s - %s: ", STR(TOOLNAME), name);
   print_value(value, t);
   printf("\n");
   (void) category;
   (void) subcategory;
   (void) opaque_value;
}

static void print_on_flush(const char *name, int category, const char *subcategory, adiak_value_t *value, adiak_datatype_t *t, void *opaque_value)
{
   if (strcmp(name, "fini") != 0)
      return;
   adiak_list_namevals(1, adiak_category_all, print_nameval, NULL);
   (void) category;
   (void) subcategory;
   (void) value;
   (void) t;
   (void) opaque_value;
}

#if !defined(_MSC_VER)
void onload() __attribute__((constructor));
#endif
void onload()
{
   if (strcmp(STR(TOOLNAME), "TOOL3") == 0)
      adiak_register_cb(1, adiak_control, print_on_flush, 0, NULL);
   else
      adiak_register_cb(1, adiak_category_all, print_nameval, 0, NULL);
}
      

