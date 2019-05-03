#include "adiak.h"
#include "adiak_internal.h"
#include "adiak_tool.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

typedef struct {
   adiak_nameval_cb_t nameval_cb;
   adiak_request_cb_t request_cb;
} adiak_tool_v1_t;

typedef struct adiak_tool_t {
   //Below fields are present in v1
   int version;
   struct adiak_tool_t *next;
   struct adiak_tool_t *prev;
   void *opaque_val;
   adiak_nameval_cb_t name_val_cb;
   adiak_request_cb_t request_cb;
   int report_on_all_ranks;
   adiak_category_t category;
} adiak_tool_t;

static adiak_t *global_adiak;
static adiak_tool_t **tool_list;

typedef enum {
   base_none = 0,
   base_long,
   base_int,
   base_string,
   base_double,
   base_ptr
} base_type;

typedef struct {
   const char *typename;
   char *format_chars;
   adiak_type_t dtype;
   adiak_numerical_t numerical;
   size_t elem_size;
   base_type base;
   int version;
} adiak_valentries_t;

static adiak_valentries_t adiak_valentries[] = {
   { "long",          "ld", adiak_long,      adiak_rational, sizeof(long),             base_long   },
   { "unsigned long", "lu", adiak_ulong,     adiak_rational, sizeof(unsigned long),    base_long   },
   { "int",           "d",  adiak_int,       adiak_rational, sizeof(int),              base_int    },
   { "unsigned int",  "U",  adiak_uint,      adiak_rational, sizeof(unsigned int),     base_int    },
   { "double",        "f",  adiak_double,    adiak_rational, sizeof(double),           base_double },
   { "date",          "D",  adiak_date,      adiak_interval, sizeof(unsigned long),    base_long   },
   { "timeval",       "t",  adiak_timeval,   adiak_interval, sizeof(struct timeval *), base_ptr    },
   { "version",       "v",  adiak_version,   adiak_ordinal,  sizeof(char *),           base_string },
   { "string",        "s",  adiak_string,    adiak_ordinal,  sizeof(char *),           base_string },
   { "catstring",     "r",  adiak_catstring, adiak_categorical, sizeof(char *),        base_string },
   { "path",          "p",  adiak_path,      adiak_categorical, sizeof(char *),        base_string },
   { NULL, NULL, 0, 0, 0, base_none }
};

static const adiak_datatype_t adiak_v1_types = {adiak_rational, adiak_set, adiak_path, adiak_performance};
static adiak_datatype_t adiak_types_by_version[1];

static void adiak_common_init();
static int format_match(const char *users, const char *reference);
static adiak_valentries_t *get_valentry_from_char(const char *c);
static int adiak_nameval(const char *name, const void *buffer, size_t buffer_size, size_t elem_size, adiak_datatype_t valtype);
static int adiak_request(adiak_request_t req, int request_version);

static void adiak_register(int adiak_version, adiak_category_t category,
                           adiak_nameval_cb_t nv, adiak_request_cb_t rq,
                           int report_on_all_ranks, void *opaque_val);

adiak_t* adiak_globals()
{
   return global_adiak;
}

int adiak_value(const char *name, adiak_category_t category, const char *typestr, ...)
{
   va_list argp;
   const char *c;
   int in_set = 0, in_range = 0, in_array = 0, parsed_type = 0;
   int i = 0, j = 0, set_size = 1, buffer_size = 0, elem_size = 0, array_size = 1;
   int result, err_ret = -1;
   adiak_datatype_t valtype = adiak_unset_datatype;
   adiak_valentries_t *valentry;

   char *buffer = NULL;
   char base_buffer[4096];
   
   va_start(argp, typestr);

   for (c = typestr; *c; c++) {
      if (*c == ' ' || *c == '\n' || *c == '\t' || *c == '}' || *c == ']')
         continue;      
      else if (*c == '{') {
         in_set = 1;
         valtype.grouping = adiak_set;
         set_size = va_arg(argp, int);
      }
      else if (*c == '[') {
         in_array = 1;
         valtype.grouping = adiak_set;
         array_size = va_arg(argp, int);
      }
      else if (*c == '-') {
         in_range = 1;
         valtype.grouping = adiak_range;
         set_size = 2;
      }
      else if (*c == '%') {
         c++;
         valentry = get_valentry_from_char(c);
         if (!valentry)
            goto error;
         c += (strlen(valentry->format_chars) - 1);

         elem_size = valentry->elem_size;
         buffer_size = elem_size * set_size * array_size;
         if (buffer_size < sizeof(base_buffer))
            buffer = base_buffer;
         else
            buffer = (char *) malloc(buffer_size);

         valtype.numerical = valentry->numerical;
         valtype.dtype = valentry->dtype;
         valtype.category = category;

         if (!in_array) {
            for (i = 0; i < set_size; i++) {
               switch (valentry->base) {
                  case base_long:
                     ((long*) buffer)[j++] = va_arg(argp, long);
                     break;
                  case base_int:
                     ((int*) buffer)[j++] = va_arg(argp, int);
                     break;
                  case base_double:
                     ((double*) buffer)[j++] = va_arg(argp, double);
                     break;
                  case base_string:
                     ((char**) buffer)[j++] = va_arg(argp, char*);
                     break;
                  case base_ptr:
                     ((void**) buffer)[j++] = va_arg(argp, void*);
                     break;
                  case base_none:
                     goto error;
               }
            }
         }
         else {
            switch (valentry->base) {
               case base_long: {
                  long *elemarray = va_arg(argp, long*);
                  for (i = 0; i < array_size; i++)
                     ((long*) buffer)[j++] = elemarray[i];
                  break;
               }
               case base_int: {
                  int *elemarray = va_arg(argp, int*);
                  for (i = 0; i < array_size; i++)
                     ((int*) buffer)[j++] = elemarray[i];
                  break;
               }
               case base_double: {
                  double *elemarray = va_arg(argp, double*);
                  for (i = 0; i < array_size; i++)
                     ((double*) buffer)[j++] = elemarray[i];
                  break;
               }
               case base_string: {
                  char **elemarray = va_arg(argp, char **);
                  for (i = 0; i < array_size; i++)
                     ((char **) buffer)[j++] = elemarray[i];
                  break;
               }
               case base_ptr: {
                  void **elemarray = va_arg(argp, void **);
                  for (i = 0; i < array_size; i++)
                     ((void **) buffer)[j++] = elemarray[i];
                  break;
               }
               case base_none:
                  goto error;
            }
         }
         if (!in_range && !in_set && !in_array)
            valtype.grouping = adiak_point; 
         parsed_type = 1;
      }
      else {
         goto error;
      }
   }

   if (!parsed_type)
      goto error;

   result = adiak_nameval(name, (void *) buffer, elem_size, set_size * array_size, valtype);
   if (result == -1)
      goto error;

   err_ret = 0;
  error:
   if (buffer && buffer != base_buffer) {
      free(buffer);
      buffer = NULL;
   }
   return err_ret;
}

int adiak_rawval(const char *name, const void *elems, size_t elem_size, size_t num_elems, adiak_datatype_t valtype)
{
   return adiak_nameval(name, elems, elem_size, num_elems, valtype);
}

adiak_numerical_t adiak_numerical_from_type(adiak_type_t t)
{
   adiak_valentries_t *i;
   for (i = adiak_valentries; i->typename != NULL; i++) {
      if (i->dtype == t)
         return i->numerical;
   }
   return adiak_numerical_unset;
}

void adiak_register_cb(int adiak_version, adiak_category_t category,
                       adiak_nameval_cb_t nv, int report_on_all_ranks, void *opaque_val)
{
   adiak_register(adiak_version, category, nv, NULL, report_on_all_ranks, opaque_val);
}

void adiak_register_preformance_request(int adiak_version, adiak_category_t category,
                                        adiak_request_cb_t req, int report_on_all_ranks, void *opaque_val)
{
   adiak_register(adiak_version, category, NULL, req, report_on_all_ranks, opaque_val);   
}


static int measure_adiak_walltime;
static int measure_adiak_systime;
static int measure_adiak_cputime;

int adiak_walltime()
{
   measure_adiak_walltime = 1;
   return 0;
}

int adiak_systime()
{
   measure_adiak_systime = 1;
   return 0;   
}

int adiak_cputime()
{
   measure_adiak_cputime = 1;
   return 0;   
}

int adiak_job_size()
{
#if defined(MPI_VERSION)
   int size;
   if (!global_adiak->use_mpi)
      return -1;
   MPI_Comm_size(global_adiak->adiak_communicator, &size);
   return adiak_value("jobsize", adiak_general, "%d", size);
#else
   return -1;
#endif
}

int adiak_mpitime()
{
#if defined(MPI_VERSION)
   if (!global_adiak->use_mpi)
      return -1;   
   return adiak_request(mpitime, 1);
#else
   return -1;
#endif
}

#if defined(MPI_VERSION)
void adiak_init(MPI_Comm *communicator)
{
   int rank;
   adiak_common_init();

   if (communicator) {
      global_adiak->adiak_communicator = *communicator;
      MPI_Comm_rank(global_adiak->adiak_communicator, &rank);
      global_adiak->reportable_rank = (rank == 0);
      global_adiak->use_mpi = 1;
   }
}
#else
void adiak_init(void *unused)
{
   adiak_common_init();
}
#endif

void adiak_fini()
{
   if (measure_adiak_cputime)
      adiak_measure_times(0, 1);
   if (measure_adiak_systime)
      adiak_measure_times(1, 0);
   if (measure_adiak_walltime)
      adiak_measure_walltime();
}

static void adiak_common_init()
{
   static int initialized = 0;
   if (initialized)
      return;
   initialized = 1;

   global_adiak = adiak_sys_init();
   assert(global_adiak);

   if (ADIAK_VERSION < global_adiak->minimum_version)
      global_adiak->minimum_version = ADIAK_VERSION;
   tool_list = global_adiak->tool_list;

   adiak_types_by_version[0] = adiak_v1_types;
}

static int format_match(const char *users, const char *reference)
{
   if (*users != *reference)
      return 0;
   if (*(reference+1) == '\0')
      return 1;
   return *(users+1) == *(reference+1) ? 1 : 0;
}

static adiak_valentries_t *get_valentry_from_char(const char *c)
{
   adiak_valentries_t *i;
   for (i = adiak_valentries; i->typename != NULL; i++) {
      if (format_match(c, i->format_chars))
         return i;
   }
   return NULL;
}

static int adiak_nameval(const char *name, const void *buffer, size_t elem_size, size_t num_elems, adiak_datatype_t valtype)
{
   adiak_tool_t *tool;
   int valtype_version = -1, i;

   for (i = 0; i < sizeof(adiak_types_by_version)/sizeof(adiak_datatype_t); i++) {
      if (adiak_types_by_version[i].numerical >= valtype.numerical &&
          adiak_types_by_version[i].grouping >= valtype.grouping &&
          adiak_types_by_version[i].dtype >= valtype.dtype &&
          adiak_types_by_version[i].category >= valtype.category) {
         valtype_version = i;
         break;
      }
   }
   if (valtype_version == -1)
      return -1;
   
   for (tool = *tool_list; tool != NULL; tool = tool->next) {
      if (tool->version < valtype_version)
         continue;
      if (!tool->report_on_all_ranks && !global_adiak->reportable_rank)
         continue;
      if (tool->category != adiak_category_all && tool->category != valtype.category)
         continue;
      if (tool->name_val_cb)
         tool->name_val_cb(name, (void *) buffer, elem_size, num_elems, valtype, tool->opaque_val);
   }
   return 0;
}

static int adiak_request(adiak_request_t req, int request_version)
{
   adiak_tool_t *tool;
   for (tool = *tool_list; tool != NULL; tool = tool->next) {
      if (tool->version < request_version)
         continue;
      if (!tool->report_on_all_ranks && !global_adiak->reportable_rank)
         continue;      
      if (tool->request_cb)
         tool->request_cb(req, tool->opaque_val);
   }
   return 0;
}

static void adiak_register(int adiak_version, adiak_category_t category,
                           adiak_nameval_cb_t nv, adiak_request_cb_t rq,
                           int report_on_all_ranks, void *opaque_val)
{
   adiak_tool_t *newtool;
   adiak_common_init();
   newtool = (adiak_tool_t *) malloc(sizeof(adiak_tool_t));
   memset(newtool, 0, sizeof(*newtool));
   newtool->version = adiak_version;
   newtool->opaque_val = opaque_val;
   newtool->report_on_all_ranks = report_on_all_ranks;
   newtool->name_val_cb = nv;
   newtool->request_cb = rq;
   newtool->category = category;
   newtool->next = *tool_list;
   newtool->prev = NULL;
   if (*tool_list) 
      (*tool_list)->prev = newtool;
   *tool_list = newtool;
   if (report_on_all_ranks && !global_adiak->report_on_all_ranks)
      global_adiak->report_on_all_ranks = 1;
}
