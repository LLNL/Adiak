#include "adiak_tool.h"

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>

#define XSTR(S) #S
#define STR(S) XSTR(S)

#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_ ## x
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
      case adiak_longlong:
         printf("%lld", val->v_longlong);
         break;
      case adiak_ulonglong:
         printf("%llu", (unsigned long long) val->v_longlong);
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
         struct tm *loc = localtime(&seconds_since_epoch);
         strftime(datestr, sizeof(datestr), "%a, %d %b %Y %T %z", loc);
         printf("%s", datestr);
         break;
      }
      case adiak_timeval: {
         struct timeval *tval = (struct timeval *) val->v_ptr;
         double duration = tval->tv_sec + (tval->tv_usec / 1000000.0);
         printf("%fs:timeval", duration);
         break;
      }
      case adiak_version: {
         char *s = (char *) val->v_ptr;
         printf("\"%s\":version", s);
         break;
      }
      case adiak_string: {
         char *s = (char *) val->v_ptr;
         printf("\"%s\":string", s);
         break;
      }
      case adiak_catstring: {
         char *s = (char *) val->v_ptr;
         printf("\"%s\":catstring", s);
         break;
      }
      case adiak_jsonstring: {
         char *s = (char *) val->v_ptr;
         printf("\"%s\":jsonstring", s);
         break;
      }
      case adiak_path: {
         char *s = (char *) val->v_ptr;
         printf("\"%s\":path", s);
         break;
      }
      case adiak_range: {
         adiak_value_t subvals[2];
         adiak_datatype_t* subtypes[2];

         adiak_get_subval(t, val, 0, subtypes+0, subvals+0);
         adiak_get_subval(t, val, 1, subtypes+1, subvals+1);

         print_value(subvals+0, *(subtypes+0));
         printf(" - ");
         print_value(subvals+1, *(subtypes+1));
         break;
      }
      case adiak_set: {
         printf("[");
         int num_elements = adiak_num_subvals(t);
         for (int i = 0; i < num_elements; i++) {
            adiak_value_t subval;
            adiak_datatype_t* subtype;
            adiak_get_subval(t, val, i, &subtype, &subval);
            print_value(&subval, subtype);
            if (i+1 != num_elements)
               printf(", ");
         }
         printf("]");
         break;
      }
      case adiak_list: {
         printf("{");
         int num_elements = adiak_num_subvals(t);
         for (int i = 0; i < num_elements; i++) {
            adiak_value_t subval;
            adiak_datatype_t* subtype;
            adiak_get_subval(t, val, i, &subtype, &subval);
            print_value(&subval, subtype);
            if (i+1 != num_elements)
               printf(", ");
         }
         printf("}");
         break;
      }
      case adiak_tuple: {
         printf("(");
         int num_elements = adiak_num_subvals(t);
         for (int i = 0; i < num_elements; i++) {
            adiak_value_t subval;
            adiak_datatype_t* subtype;
            adiak_get_subval(t, val, i, &subtype, &subval);
            print_value(&subval, subtype);
            if (i+1 != num_elements)
               printf(", ");
         }
         printf(")");
         break;
      }
   }
}

static void print_nameval(const char *name, adiak_value_t *value, adiak_datatype_t *t, adiak_record_info_t *info, void *UNUSED(opaque_value))
{
   double timestamp = ((double) info->timestamp.tv_sec) + (info->timestamp.tv_nsec * 1e-9);
   printf("%s - %f - %s: ", STR(TOOLNAME), timestamp, name);
   print_value(value, t);
   printf("\n");
}

static void print_on_flush(const char *name, int UNUSED(category), const char *UNUSED(subcategory), adiak_value_t *UNUSED(value), adiak_datatype_t *UNUSED(t), void *UNUSED(opaque_value))
{
   if (strcmp(name, "flush") != 0)
      return;
   adiak_list_namevals_with_info(1, adiak_category_all, print_nameval, NULL);
}

void print_all_adiak_vars()
{
   adiak_list_namevals_with_info(1, adiak_category_all, print_nameval, NULL);
}

static void onload() __attribute__((constructor));
static void onload()
{
   if (strcmp(STR(TOOLNAME), "TOOL3") == 0)
      adiak_register_cb(1, adiak_control, print_on_flush, 0, NULL);
   else
      adiak_register_cb_with_info(1, adiak_category_all, print_nameval, 0, NULL);
}
