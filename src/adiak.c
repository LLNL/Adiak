// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "adiak.h"
#include "adiak_tool.h"
#include "adksys.h"

typedef struct adiak_tool_t {
   //Below fields are present in v1
   int version;
   struct adiak_tool_t *next;
   struct adiak_tool_t *prev;
   void *opaque_val;
   adiak_nameval_cb_t name_val_cb;
   int report_on_all_ranks;
   int category;
} adiak_tool_t;

typedef struct {
   int minimum_version;
   int version;
   int report_on_all_ranks;
   int reportable_rank;
   adiak_tool_t **tool_list;
   int use_mpi;
} adiak_t;

typedef struct record_list_t {
   const char *name;
   int category;
   const char *subcategory;
   adiak_value_t *value;
   adiak_datatype_t *dtype;
   struct record_list_t *list_next;
   struct record_list_t *hash_next;
} record_list_t;

static adiak_t *adiak_config;
static volatile adiak_tool_t **tool_list;

static adiak_tool_t *local_tool_list;
adiak_t adiak_public = { ADIAK_VERSION, ADIAK_VERSION, 0, 1, &local_tool_list, 0 };


static int measure_adiak_walltime;
static int measure_adiak_systime;
static int measure_adiak_cputime;

static adiak_datatype_t base_long = { adiak_long, adiak_rational, 0, 0, NULL };
static adiak_datatype_t base_ulong = { adiak_ulong, adiak_rational, 0, 0, NULL };
static adiak_datatype_t base_int = { adiak_int, adiak_rational, 0, 0, NULL };
static adiak_datatype_t base_uint = { adiak_uint, adiak_rational, 0, 0, NULL };
static adiak_datatype_t base_double = { adiak_double, adiak_rational, 0, 0, NULL };
static adiak_datatype_t base_date = { adiak_date, adiak_interval, 0, 0, NULL };
static adiak_datatype_t base_timeval = { adiak_timeval, adiak_interval, 0, 0, NULL };
static adiak_datatype_t base_version = { adiak_version, adiak_ordinal, 0, 0, NULL };
static adiak_datatype_t base_string = { adiak_string, adiak_ordinal, 0, 0, NULL };
static adiak_datatype_t base_catstring = { adiak_catstring, adiak_categorical, 0, 0, NULL };
static adiak_datatype_t base_path = { adiak_path, adiak_categorical, 0, 0, NULL };

static void adiak_register(int adiak_version, int category,
                           adiak_nameval_cb_t nv,
                           int report_on_all_ranks, void *opaque_val);

static int find_end_brace(const char *typestr, char endchar, int typestr_start, int typestr_end);
static adiak_datatype_t *parse_typestr(const char *typestr, va_list *ap);
static adiak_datatype_t *parse_typestr_helper(const char *typestr, int typestr_start, int typestr_end,
                                              va_list *ap, int *new_typestr_start);
static void free_adiak_type(adiak_datatype_t *t);
static void free_adiak_value(adiak_datatype_t *t, adiak_value_t *v);
static void free_adiak_value_worker(adiak_datatype_t *t, adiak_value_t *v);

static adiak_type_t toplevel_type(const char *typestr);
static int copy_value(adiak_value_t *target, adiak_datatype_t *datatype, void *ptr);

static void record_nameval(const char *name, int category, const char *subcategory,
                           adiak_value_t *value, adiak_datatype_t *dtype);

static int measure_walltime();
static int measure_systime();
static int measure_cputime();

#define RECORD_HASH_SIZE 128
static record_list_t *record_list;
static record_list_t *record_list_end;
static record_list_t *record_hash[RECORD_HASH_SIZE];

#define MAX_PATH_LEN 4096

adiak_datatype_t *adiak_new_datatype(const char *typestr, ...)
{
   va_list ap;
   adiak_datatype_t *t;
   va_start(ap, typestr);
   t = parse_typestr(typestr, &ap);
   va_end(ap);
   return t;
}

int adiak_raw_namevalue(const char *name, int category, const char *subcategory,
                        adiak_value_t *value, adiak_datatype_t *type)
{
   adiak_tool_t *tool;

   if (category != adiak_control)
      record_nameval(name, category, subcategory, value, type);
   if (!tool_list)
      return 0;
   
   for (tool = (adiak_tool_t *) *tool_list; tool != NULL; tool = tool->next) {
      if (!tool->report_on_all_ranks && !adiak_config->reportable_rank)
         continue;
      if (tool->category != adiak_category_all && tool->category != category)
         continue;
      if (tool->name_val_cb)
         tool->name_val_cb(name, category, subcategory, value, type, tool->opaque_val);
   }
   return 0;   
}

int adiak_namevalue(const char *name, int category, const char *subcategory, const char *typestr, ...)
{
   va_list ap;
   adiak_datatype_t *t;
   adiak_type_t toptype;
   adiak_value_t *value = NULL;
   void *container_ptr = NULL;

   toptype = toplevel_type(typestr);
   if (toptype == adiak_type_unset)
      return -1;
   value = (adiak_value_t *) malloc(sizeof(adiak_value_t));

   va_start(ap, typestr);

   switch (toptype) {
      case adiak_type_unset:
         return -1;
      case adiak_long:
      case adiak_ulong:
      case adiak_date:
         value->v_long = va_arg(ap, long);
         break;
      case adiak_int:
      case adiak_uint:
         value->v_int = va_arg(ap, int);
         break;
      case adiak_double:
         value->v_double = va_arg(ap, double);
         break;
      case adiak_timeval: {
         struct timeval *v = (struct timeval *) malloc(sizeof(struct timeval));
         *v = *va_arg(ap, struct timeval *);
         value->v_ptr = v;
         break;
      }
      case adiak_version:
      case adiak_string:
      case adiak_catstring:
      case adiak_path:
         value->v_ptr = strdup(va_arg(ap, void*));
         break;
      case adiak_range:
      case adiak_set:
      case adiak_list:
      case adiak_tuple:
         container_ptr = va_arg(ap, void*);
         break;
   }
   
   t = parse_typestr(typestr, &ap);
   va_end(ap);
   if (!t) {
      free(value);
      return -1;
   }

   if (container_ptr) {
      copy_value(value, t, container_ptr);
   }
   
   return adiak_raw_namevalue(name, category, subcategory, value, t);
}

adiak_numerical_t adiak_numerical_from_type(adiak_type_t dtype)
{
   switch (dtype) {
      case adiak_type_unset:
         return adiak_numerical_unset;
      case adiak_long:
      case adiak_ulong:
      case adiak_int:
      case adiak_uint:
      case adiak_double:
         return adiak_rational;
      case adiak_date:
      case adiak_timeval:
         return adiak_interval;
      case adiak_version:
      case adiak_string:
         return adiak_ordinal;
      case adiak_catstring:
      case adiak_path:
      case adiak_range:
      case adiak_set:
      case adiak_list:
      case adiak_tuple:
         return adiak_categorical;
   }
   return adiak_numerical_unset;
}

void adiak_register_cb(int adiak_version, int category,
                       adiak_nameval_cb_t nv, int report_on_all_ranks, void *opaque_val)
{
   adiak_register(adiak_version, category, nv, report_on_all_ranks, opaque_val);
}

void adiak_list_namevals(int adiak_version, int category, adiak_nameval_cb_t nv, void *opaque_val)
{
   record_list_t *i;
   for (i = record_list; i != NULL; i = i->list_next) {
      if (category != adiak_category_all && i->category != category)
         continue;
      nv(i->name, i->category, i->subcategory, i->value, i->dtype, opaque_val);
   }
   (void) adiak_version;
}

int adiak_get_nameval(const char *name, adiak_datatype_t **t, adiak_value_t **value,  int *cat, const char **subcat)
{
   record_list_t *i;
   for (i = record_list; i != NULL; i = i->list_next) {
      if (strcmp(i->name, name) == 0) {
         if (t)
            *t = i->dtype;
         if (value)
            *value = i->value;
         if (cat)
            *cat = i->category;
         if (subcat)
            *subcat = i->subcategory;
         return 0;
      }
   }
   return -1;
}

static void adiak_common_init()
{
   static int initialized = 0;
   if (initialized)
      return;
   initialized = 1;

   adiak_config = (adiak_t *) adksys_get_public_adiak_symbol();
   if (!adiak_config)
      adiak_config = &adiak_public;
   if (ADIAK_VERSION < adiak_config->minimum_version)
      adiak_config->minimum_version = ADIAK_VERSION;
   tool_list = (volatile adiak_tool_t **) adiak_config->tool_list;
}

void adiak_init(void *mpi_communicator_p)
{
   static int initialized = 0;
   if (initialized)
      return;
   initialized = 1;

   adiak_common_init();

#if (USE_MPI)
   if (mpi_communicator_p && adksys_mpi_initialized()) {
      adksys_mpi_init(mpi_communicator_p);
      adiak_config->reportable_rank = adksys_reportable_rank();
      adiak_config->use_mpi = 1;
   }
   else 
#endif
   do {
      adiak_config->reportable_rank = 1;
      adiak_config->use_mpi = 0;
      (void) mpi_communicator_p;
   } while (0);

}


void adiak_fini()
{
   adiak_value_t val;
   if (measure_adiak_cputime)
      measure_cputime();
   if (measure_adiak_systime)
      measure_systime();
   if (measure_adiak_walltime)
      measure_walltime();

   val.v_int = 0;
   adiak_raw_namevalue("fini", adiak_control, NULL, &val, &base_int);   
}

static void adiak_register(int adiak_version, int category,
                           adiak_nameval_cb_t nv,
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
   newtool->category = category;
   newtool->next = (adiak_tool_t *) *tool_list;
   newtool->prev = NULL;
   if (*tool_list) 
      (*tool_list)->prev = newtool;
   *tool_list = newtool;
   if (report_on_all_ranks && !adiak_config->report_on_all_ranks)
      adiak_config->report_on_all_ranks = 1;
}

static int find_end_brace(const char *typestr, char endchar, int typestr_start, int typestr_end) {
   int depth = 0;
   int cur = typestr_start;

   if (!typestr)
      return -1;

   while (cur < typestr_end) {
      if (typestr[cur] == '[' || typestr[cur] == '{' || typestr[cur] == '(' || typestr[cur] == '<')
         depth++;
      if (typestr[cur] == ']' || typestr[cur] == '}' || typestr[cur] == ')' || typestr[cur] == '>')
         depth--;
      if (depth == 0 && typestr[cur] == endchar)
         return cur;
      cur++;
   }
   return -1;      
}

static adiak_datatype_t *parse_typestr(const char *typestr, va_list *ap)
{
   int end = 0;
   int len;
   
   len = strlen(typestr);
   return parse_typestr_helper(typestr, 0, len, ap, &end);
}

static adiak_type_t toplevel_type(const char *typestr) {
   const char *cur = typestr;
   if (!cur)
      return adiak_type_unset;
   while (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == ',')
      cur++;
   if (*cur == '%') {
      cur++;      
      switch (*cur) {
         case 'l':
            cur++;
            if (*cur == 'd') return adiak_long;
            if (*cur == 'u') return adiak_ulong;
            return adiak_type_unset;
         case 'd': return adiak_int;
         case 'u': return adiak_uint;
         case 'f': return adiak_double;
         case 'D': return adiak_date;
         case 't': return adiak_timeval;
         case 'v': return adiak_version;
         case 's': return adiak_string;
         case 'r': return adiak_catstring;
         case 'p': return adiak_path;
         default:
            return adiak_type_unset;
      }
   }
   switch (*cur) {
      case '<': return adiak_range;
      case '[': return adiak_set;
      case '{': return adiak_list;
      case '(': return adiak_tuple;
      default: return adiak_type_unset;
   }
}

adiak_datatype_t *adiak_get_basetype(adiak_type_t t)
{
   switch (t) {
      case adiak_type_unset:
         return NULL;
      case adiak_long:
         return &base_long;
      case adiak_ulong:
         return &base_ulong;
      case adiak_int:
         return &base_int;
      case adiak_uint:
         return &base_uint;
      case adiak_double:
         return &base_double;
      case adiak_date:
         return &base_date;
      case adiak_timeval:
         return &base_timeval;
      case adiak_version:
         return &base_version;
      case adiak_string:
         return &base_string;
      case adiak_catstring:
         return &base_catstring;
      case adiak_path:
         return &base_path;
      case adiak_range:
      case adiak_set:
      case adiak_list:
      case adiak_tuple:
      default:
         return NULL;
   }
}

static void free_adiak_type(adiak_datatype_t *t)
{
   int i;
   if (t == NULL)
      return;
   if (adiak_get_basetype(t->dtype) == t)
      return;
   for (i = 0; i < t->num_subtypes; i++) {
      free_adiak_type(t->subtype[i]);
   }
   if (t->num_subtypes)
      free(t->subtype);
   free(t);
}

static void free_adiak_value_worker(adiak_datatype_t *t, adiak_value_t *v) {
   int i;
   adiak_value_t *values;
   
   switch (t->dtype) {
      case adiak_type_unset:
      case adiak_long:
      case adiak_ulong:
      case adiak_int:
      case adiak_uint:
      case adiak_double:
      case adiak_date:
         break;
      case adiak_timeval:
      case adiak_version:
      case adiak_string:
      case adiak_catstring:
      case adiak_path:
         free(v->v_ptr);
         break;
      case adiak_range:
      case adiak_set:
      case adiak_list:
         values = (adiak_value_t *) v->v_ptr;
         for (i = 0; i < t->num_elements; i++) {
            free_adiak_value_worker(t->subtype[0], values+i);
         }
         free(values);
         break;
      case adiak_tuple:
         values = (adiak_value_t *) v->v_ptr;
         for (i = 0; i < t->num_elements; i++) {
            free_adiak_value_worker(t->subtype[i], values+i);
         }
         free(values);
         break;
   }
}

static void free_adiak_value(adiak_datatype_t *t, adiak_value_t *v) {
   free_adiak_value_worker(t, v);
   free(v);
}

static int copy_value(adiak_value_t *target, adiak_datatype_t *datatype, void *ptr) {
   int bytes_read = 0, result, type_index = 0, i;
   adiak_value_t *newvalues;
   switch (datatype->dtype) {
      case adiak_type_unset:
         return -1;
      case adiak_long:
      case adiak_ulong:
      case adiak_date:
         target->v_long = *((long *) ptr);
         return sizeof(long);
      case adiak_int:
      case adiak_uint:
         target->v_int = *((int *) ptr);
         return sizeof(int);
      case adiak_double:
         target->v_double= *((double *) ptr);
         return sizeof(double);
      case adiak_timeval: {
         struct timeval *v = (struct timeval *) malloc(sizeof(struct timeval));
         *v = *(struct timeval *) ptr;
         target->v_ptr = v;
         return sizeof(struct timeval *);
      }
      case adiak_version:
      case adiak_string:
      case adiak_catstring:
      case adiak_path:
         target->v_ptr = strdup(*((void **) ptr));
         return sizeof(char *);
      case adiak_range:
      case adiak_set:
      case adiak_list:
      case adiak_tuple:
         newvalues = (adiak_value_t *) malloc(sizeof(adiak_value_t) * datatype->num_elements);
         for (i = 0; i < datatype->num_elements; i++) {
            unsigned char *array_base = (unsigned char *) ptr;
            result = copy_value(newvalues+i, datatype->subtype[type_index], array_base + bytes_read);
            if (result == -1)
               return -1;
            bytes_read += result;
            if (datatype->dtype == adiak_tuple)
               type_index++;
         }
         target->v_subval = newvalues;
         return bytes_read;
   }
   return -1;
}

static adiak_datatype_t *parse_typestr_helper(const char *typestr, int typestr_start, int typestr_end,
                                       va_list *ap, int *new_typestr_start)

{
   adiak_datatype_t *t = NULL;
   int cur = typestr_start;
   int end_brace, i, is_long = 0;
   
   if (!typestr)
      goto error;
   if (typestr_start == typestr_end)
      goto error;

   while (typestr[cur] == ' ' || typestr[cur] == '\t' || typestr[cur] == '\n' || typestr[cur] == ',')
      cur++;

   if (strchr("[{<(", typestr[cur])) {
      t = (adiak_datatype_t *) malloc(sizeof(adiak_datatype_t));
      memset(t, 0, sizeof(*t));
   }
   if (typestr[cur] == '{' || typestr[cur] == '[') {
      end_brace = find_end_brace(typestr, typestr[cur] == '{' ? '}' : ']',
                                 cur, typestr_end);
      if (end_brace == -1)
         goto error;
      t->num_elements = va_arg(*ap, int);
      t->dtype = typestr[cur] == '{' ? adiak_list : adiak_set;
      t->numerical = adiak_categorical;
      t->num_subtypes = 1;
      t->subtype = (adiak_datatype_t **) malloc(sizeof(adiak_datatype_t *));
      t->subtype[0] = parse_typestr_helper(typestr, cur+1, end_brace, ap, &cur);
      if (t->subtype[0] == NULL)
         goto error;
      *new_typestr_start = end_brace+1;
      goto done;
   }
   else if (typestr[cur] == '<') {
      end_brace = find_end_brace(typestr, '>', cur, typestr_end);
      if (end_brace == -1)
         goto error;
      t->dtype = adiak_range;
      t->num_elements = 2;
      t->numerical = adiak_categorical;
      t->num_subtypes = 1;
      t->subtype = (adiak_datatype_t **) malloc(sizeof(adiak_datatype_t *));
      t->subtype[0] = parse_typestr_helper(typestr, cur+1, end_brace, ap, &cur);
      if (t->subtype[0] == NULL)
         goto error;
      *new_typestr_start = end_brace+1;      
      goto done;
   }
   else if (typestr[cur] == '(') {
      end_brace = find_end_brace(typestr, ')', cur, typestr_end);
      if (end_brace == -1)
         goto error;
      t->dtype = adiak_tuple;
      t->numerical = adiak_categorical;
      t->num_subtypes = t->num_elements = va_arg(*ap, int);
      t->subtype = (adiak_datatype_t **) malloc(sizeof(adiak_datatype_t *) * t->num_subtypes);
      memset(t->subtype, 0, sizeof(adiak_datatype_t *) * t->num_subtypes);
      cur++;
      for (i = 0; i < t->num_subtypes; i++) {
         t->subtype[i] = parse_typestr_helper(typestr, cur, end_brace, ap, &cur);
         if (!t->subtype[i])
            goto error;
      }
   }
   else if (typestr[cur] == '%') {
      cur++;
      if (typestr[cur] == 'l') {
         is_long = 1;
         cur++;
      }
      switch (typestr[cur]) {
         case 'd':
            t = is_long ? &base_long : &base_int;
            break;
         case 'u':
            t = is_long ? &base_ulong : &base_uint;
            break;
         case 'f':
            t = &base_double;
            break;
         case 'D':
            t = &base_date;
            break;
         case 't':
            t = &base_timeval;
            break;
         case 'v':
            t = &base_version;
            break;
         case 's':
            t = &base_string;
            break;
         case 'r':
            t = &base_catstring;
            break;
         case 'p':
            t = &base_path;
            break;
         default:
            goto error;
      }
      cur++;
      *new_typestr_start = cur;
      goto done;
   }

  done:
   return t;
  error:
   if (t)
      free_adiak_type(t);
   return NULL;
}

static unsigned long strhash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;   
}

static void record_nameval(const char *name, int category, const char *subcategory,
                           adiak_value_t *value, adiak_datatype_t *dtype)
{
   record_list_t *addrecord = NULL, *i;
   unsigned long hashval;
   int newrecord = 0;

   hashval = strhash(name) % RECORD_HASH_SIZE;
   for (i = record_hash[hashval]; i != NULL; i = i->hash_next) {
      if (strcmp(i->name, name) == 0) {
         addrecord = i;
         break;
      }
   }

   if (!addrecord) {
      addrecord = (record_list_t *) malloc(sizeof(record_list_t));
      memset(addrecord, 0, sizeof(*addrecord));
      newrecord = 1;
   }
   else {
      free_adiak_value(addrecord->dtype, addrecord->value);
      free_adiak_type(addrecord->dtype);
   }
   
   addrecord->category = category;
   addrecord->subcategory = subcategory ? strdup(subcategory) : NULL;
   addrecord->value = value;
   addrecord->dtype = dtype;

   if (!newrecord)
      return;
   
   addrecord->name = (const char *) strdup(name);
   addrecord->list_next = NULL;
   if (!record_list) {
      record_list = record_list_end = addrecord;
   }
   else {
      record_list_end->list_next = addrecord;
      record_list_end = addrecord;
   }

   addrecord->hash_next = record_hash[hashval];
   record_hash[hashval] = addrecord;
}

int adiak_flush(const char *location)
{
   adiak_value_t val;
   val.v_ptr = (void *) location;
   return adiak_raw_namevalue("flush", adiak_control, NULL, &val, &base_path);
}

int adiak_clean()
{
   adiak_value_t val;
   record_list_t *i, *next;
   int result;
   
   val.v_int = 0;
   result = adiak_raw_namevalue("clean", adiak_control, NULL, &val, &base_int);
   for (i = record_list; i != NULL; i = next) {
      free_adiak_value(i->dtype, i->value);
      free_adiak_type(i->dtype);
      free((void *) i->name);
      if (i->subcategory)
         free((void *) i->subcategory);
      next = i->list_next;
      free(i);
   }
   memset(record_hash, 0, sizeof(record_hash));
   record_list = record_list_end = NULL;
   return result;
}

int adiak_launchdate()
{
   int result;
   struct timeval stime;
   result = adksys_starttime(&stime);
   if (result == -1)
      return -1;
   if (stime.tv_sec == 0 && stime.tv_usec == 0)
      return -1;
   adiak_namevalue("launchdate", adiak_general, "runinfo", "%D", stime.tv_sec);
   return 0;
}

int adiak_launchday()
{
   int result;
   struct timeval stime;
   result = adksys_starttime(&stime);
   if (result == -1)
      return -1;
   if (stime.tv_sec == 0 && stime.tv_usec == 0)
      return -1;
#define SECONDS_IN_DAY (24*60*60)
   stime.tv_sec -= (stime.tv_sec % SECONDS_IN_DAY);
   adiak_namevalue("launchday", adiak_general, "runinfo", "%D", stime.tv_sec);
   return 0;
}

int adiak_executable()
{
   char path[MAX_PATH_LEN+1];
   char *filepart;
   int result;

   result = adksys_get_executable(path, sizeof(path));
   if (result == -1)
      return -1;
   
   filepart = strrchr(path, '/');
   if (!filepart)
      filepart = path;
   else
      filepart++;

   adiak_namevalue("executable", adiak_general, "binary", "%s", filepart);
   return 0;
}

int adiak_executablepath()
{
   char path[MAX_PATH_LEN+1];
   int result;

   result = adksys_get_executable(path, sizeof(path));
   if (result == -1)
      return -1;
   
   adiak_namevalue("executablepath", adiak_general, "binary", "%p", path);
   return 0;
}

int adiak_cmdline()
{
   int result;
   char *arglist = NULL;
   int  arglist_size, i, j = 0;
   char **myargv = NULL;
   int myargc = 0;
   int retval = -1;
   
   result = adksys_get_cmdline_buffer(&arglist, &arglist_size);
   if (result == -1)
      goto error;

   for (i = 0; i < arglist_size; i++) {
      if (arglist[i] == '\0')
         myargc++;
   }

   myargv = (char **) malloc(sizeof(char *) * (myargc+1));
   myargv[j++] = arglist;
   for (i = 0; i < arglist_size; i++) {
      if (arglist[i] == '\0')
         myargv[j++] = arglist + i + 1;
   }

   result = adiak_namevalue("cmdline", adiak_general, "runinfo", "[%s]", myargv, myargc);
   if (result == -1)
      goto error;

   retval = 0;
  error:
   if (arglist)
      free(arglist);
   if (myargv)
      free(myargv);
   return retval;
}

static int measure_walltime()
{
   struct timeval stime;
   struct timeval etime, diff;
   int result;

   result = adksys_starttime(&stime);
   if (result == -1)
      return -1;

   result = adksys_curtime(&etime);
   if (result == -1)
      return -1;

   diff.tv_sec = etime.tv_sec - stime.tv_sec;
   if (etime.tv_usec < stime.tv_usec) {
      diff.tv_usec = 1000000 + etime.tv_usec - stime.tv_usec;
      diff.tv_sec--;
   }
   else
      diff.tv_usec = etime.tv_usec - stime.tv_usec;
   
   return adiak_namevalue("walltime", adiak_performance, "timing", "%t", &diff);
}

static int measure_systime()
{
   int result;
   struct timeval tm;

   result = adksys_get_times(&tm, NULL);
   if (result == -1)
      return -1;
   return adiak_namevalue("systime", adiak_performance, "timing", "%t", &tm);
}

static int measure_cputime()
{
   int result;
   struct timeval tm;

   result = adksys_get_times(NULL, &tm);
   if (result == -1)
      return -1;
   return adiak_namevalue("cputime", adiak_performance, "timing", "%t", &tm);
}

int adiak_user()
{
   int result;
   char *name;

   result = adksys_get_names(NULL, &name);
   if (result == -1)
      return -1;

   result = adiak_namevalue("user", adiak_general, "runinfo", "%s", name);
   free(name);
   return result;
}

int adiak_workdir()
{
   char cwd[FILENAME_MAX];
   int result = adksys_get_cwd(cwd, FILENAME_MAX); 
   if (result == 1)
      return -1;
   result = adiak_namevalue("working_directory", adiak_general, "runinfo", "%p", cwd); 
   return result;
}

int adiak_uid()
{
   int result;
   char *name;

   result = adksys_get_names(&name, NULL);
   if (result == -1)
      return -1;

   result = adiak_namevalue("uid", adiak_general, "runinfo", "%s", name);
   free(name);
   return result;   
}

int adiak_hostname()
{
   int result;
   char hostname[512];

   result = adksys_hostname(hostname, sizeof(hostname));
   if (result == -1)
      return -1;

   return adiak_namevalue("hostname", adiak_general, "host", "%s", hostname);
}

int adiak_clustername()
{
   char hostname[512];
   char clustername[512];
   int result;
   int i = 0;
   char *c;

   memset(hostname, 0, sizeof(hostname));
   memset(clustername, 0, sizeof(hostname));   
   result = adksys_hostname(hostname, sizeof(hostname));
   if (result == -1)
      return -1;

   for (c = hostname; *c; c++) {
      if (*c >= '0' && *c <= '9')
         continue;
      if (*c == '.')
         break;
      clustername[i++] = *c;
   }
   clustername[i] = '\0';

   return adiak_namevalue("cluster", adiak_general, "host", "%s", clustername);   
}

int adiak_hostlist()
{
   char **hostlist_array = NULL;
   int num_hosts = 0, result = -1;
   char *name_buffer = NULL;

#if defined(USE_MPI)
   if (adiak_config->use_mpi)
      result = adksys_hostlist(&hostlist_array, &num_hosts, &name_buffer, adiak_config->report_on_all_ranks);
#endif
   if (result == -1)
      return -1;
   
   if (hostlist_array)
      result = adiak_namevalue("hostlist", adiak_general, "host", "[%s]", hostlist_array, num_hosts);

   return result;
}

int adiak_num_hosts()
{
   char **hostlist_array = NULL;
   int num_hosts = 0, result = -1;
   char *name_buffer = NULL;

#if defined(USE_MPI)
   if (adiak_config->use_mpi)
      result = adksys_hostlist(&hostlist_array, &num_hosts, &name_buffer, adiak_config->report_on_all_ranks);
#endif

   if (hostlist_array && result != -1)
      result = adiak_namevalue("numhosts", adiak_general, "mpi", "%u", (unsigned int) num_hosts);
   else
      result = adiak_namevalue("numhosts", adiak_general, "mpi", "%u", (unsigned int) 1);
   
   return result;
}

int adiak_job_size()
{
   int result = -1, size = 1;

#if defined(USE_MPI)
   if (adiak_config->use_mpi)
      result = adksys_jobsize(&size);
   if (result == -1)
      return -1;
#endif
   return adiak_namevalue("jobsize", adiak_general, "mpi", "%u", size);
}

int adiak_libraries()
{
   int result;
   char **libraries = NULL;
   int libraries_size, names_need_free = 0;
   int retval = -1, i;

   result = adksys_get_libraries(&libraries, &libraries_size, &names_need_free);
   if (result == -1)
      goto error;

   result = adiak_namevalue("libraries", adiak_general, "binary", "[%p]", libraries, libraries_size);
   if (result == -1)
      goto error;
   retval = 0;

  error:
   if (names_need_free)
      for (i = 0; i < libraries_size; i++)
         if (libraries[i])
            free(libraries[i]);
   if (libraries)
      free(libraries);
   return retval;
}

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

static int adiak_type_string_helper(adiak_datatype_t *t, char *str, int len, int pos, int long_form, int size_calc_only)
{
   const char *simple = NULL;
   const char *lbracket = NULL;
   const char *rbracket = NULL;
   int i, result;
   int start_pos = pos;
   
   switch (t->dtype) {
      case adiak_type_unset:
         simple = "unset";
         break;
      case adiak_long:
         simple = long_form ? "long" : "%ld";
         break;
      case adiak_ulong:
         simple = long_form ? "unsigned long" : "%lu";
         break;
      case adiak_int:
         simple = long_form ? "int" : "%d";
         break;
      case adiak_uint:
         simple = long_form ? "unsigned int" : "%u";
         break;
      case adiak_double:
         simple = long_form ? "double" : "%f";
         break;
      case adiak_date:
         simple = long_form ? "date" : "%D";
         break;
      case adiak_timeval:
         simple = long_form ? "timeval" : "%t";
         break;
      case adiak_version:
         simple = long_form ? "version" : "%v";
         break;
      case adiak_string:
         simple = long_form ? "string" : "%s";
         break;
      case adiak_catstring:
         simple = long_form ? "catstring" : "%r";
         break;
      case adiak_path:
         simple = long_form ? "path" : "%p";
         break;
      case adiak_range:
         lbracket = long_form ? "range of " : "<";
         rbracket = long_form ? ">" : "";
         break;
      case adiak_set:
         lbracket = long_form ? "set of " :  "[";
         rbracket = long_form ? "" : "]";
         break;
      case adiak_list:
         lbracket = long_form ? "list of " : "{";
         rbracket = long_form ? "" : "}";
         break;
     case adiak_tuple:
        lbracket = long_form ? "tuple of (" : "(";
        rbracket = long_form ? ")" : ")";
        break;
   }
   if (simple) {
      if (!size_calc_only)
         result = snprintf(str + pos, len - pos, "%s", simple);
      else
         result = strlen(simple);
      if (result != -1)
         pos += result;
   }
   else if (lbracket) {
      if (!size_calc_only)
         result = snprintf(str + pos, len - pos, "%s", lbracket);
      else
         result = strlen(lbracket);
      if (result > 0) pos += result;      
      for (i = 0; i < t->num_subtypes; i++) {
         pos += adiak_type_string_helper(t->subtype[i], str, len, pos, long_form, size_calc_only);
         if (i + 1 != t->num_subtypes) {
            if (!size_calc_only)
               result = snprintf(str + pos, len - pos, ", ");
            else
               result = 2;
            if (result > 0) pos += result;
         }
      }
      if (!size_calc_only)
         result = snprintf(str + pos, len - pos, "%s", rbracket);
      else
         result = strlen(rbracket);
      if (result > 0) pos += result;
   }
   return pos - start_pos;
}

char *adiak_type_to_string(adiak_datatype_t *t, int long_form)
{
   int len;
   int len2;
   char *buffer;

   len = adiak_type_string_helper(t, NULL, 0, 0, long_form, 1);
   if (len <= 0)
      return NULL;

   buffer = (char *) malloc(len + 1);
   len2 = adiak_type_string_helper(t, buffer, len+1, 0, long_form, 0);
   assert(len == len2);
   buffer[len] = '\0';

   return buffer;
}
