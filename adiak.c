#define _GNU_SOURCE
#include <stdint.h>
//typedef struct { long apart; long bpart } my_int128_t;
//#define __int128_t my_int128_t

#include "adiak.h"
#include "adiak_tool.h"
#include <stdint.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>
#include <link.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>

typedef struct adiak_tool_t {
   //Below fields are present in v1
   int version;
   struct adiak_tool_t *next;
   struct adiak_tool_t *prev;
   void *opaque_val;
   adiak_nameval_cb_t name_val_cb;
   int report_on_all_ranks;
   adiak_category_t category;
} adiak_tool_t;

static adiak_tool_t *local_tool_list = NULL;

typedef struct {
   int minimum_version;
   int version;
   int report_on_all_ranks;
   int reportable_rank;
   adiak_tool_t **tool_list;
} adiak_t;

adiak_t adiak_public = {ADIAK_VERSION, ADIAK_VERSION, 0, 1, &local_tool_list};
static adiak_t *global_adiak;
static adiak_tool_t **tool_list;

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

#if defined(MPI_VERSION)
static MPI_Comm adiak_communicator;
static int use_mpi;
#endif

static void adiak_common_init();
static void adiak_register(int adiak_version, adiak_category_t category,
                           adiak_nameval_cb_t nv,
                           int report_on_all_ranks, void *opaque_val);
static int get_library_name(struct dl_phdr_info *info, size_t size, void *data);
static int count_libraries(struct dl_phdr_info *info, size_t size, void *cnt);

static int find_end_brace(const char *typestr, char endchar, int typestr_start, int typestr_end);
static adiak_datatype_t *parse_typestr(const char *typestr, va_list *ap);
static adiak_datatype_t *parse_typestr_helper(const char *typestr, int typestr_start, int typestr_end,
                                              va_list *ap, int *new_typestr_start);
static void free_adiak_type(adiak_datatype_t *t);
static struct timeval starttime();
static adiak_type_t toplevel_type(const char *typestr);
static int copy_value(adiak_value_t *target, adiak_datatype_t *datatype, void *ptr);

#if defined(MPI_VERSION)
static int hostname_color(char *str, int index);
static int get_unique_host(char *name, int global_rank);
static int gethostlist(char **hostlist, int *num_hosts, int *max_hostlen);
#endif

adiak_datatype_t *adiak_new_datatype(const char *typestr, ...)
{
   va_list ap;
   adiak_datatype_t *t;
   va_start(ap, typestr);
   t = parse_typestr(typestr, &ap);
   va_end(ap);
   return t;
}

int adiak_raw_namevalue(const char *name, adiak_category_t category, adiak_value_t *value, adiak_datatype_t *type)
{
   adiak_tool_t *tool;
   for (tool = *tool_list; tool != NULL; tool = tool->next) {
      if (!tool->report_on_all_ranks && !global_adiak->reportable_rank)
         continue;
      if (tool->category != adiak_category_all && tool->category != category)
         continue;
      if (tool->name_val_cb)
         tool->name_val_cb(name, category, value, type, tool->opaque_val);
   }
   return 0;   
}

int adiak_namevalue(const char *name, adiak_category_t category, const char *typestr, ...)
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
   
   return adiak_raw_namevalue(name, category, value, t);
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

void adiak_register_cb(int adiak_version, adiak_category_t category,
                       adiak_nameval_cb_t nv, int report_on_all_ranks, void *opaque_val)
{
   adiak_register(adiak_version, category, nv, report_on_all_ranks, opaque_val);
}

int adiak_user()
{
   struct passwd *p;
   char *firstcomma, *user;
  
   p = getpwuid(getuid());
   if (!p)
      return -1;
   user = p->pw_gecos;
   firstcomma = strchr(user, ',');
   if (firstcomma)
      *firstcomma = '\0';

   adiak_namevalue("user", adiak_general, "%s", user);
   return 0;
}

int adiak_uid()
{  
   struct passwd *p;
   
   p = getpwuid(getuid());
   if (!p)
      return -1;
   
   adiak_namevalue("uid", adiak_general, "%s", p->pw_name);
   return 0;
}

int adiak_launchdate()
{
   struct timeval stime = starttime();
   if (stime.tv_sec == 0 && stime.tv_usec == 0)
      return -1;
   adiak_namevalue("date", adiak_general, "%D", stime.tv_sec);
   return 0;
}

int adiak_executable()
{
   char path[4097];
   char *filepart;
   ssize_t result;

   memset(path, 0, sizeof(path));
   result = readlink("/proc/self/exe", path, sizeof(path)-1);
   if (result == -1)
      return -1;
   filepart = strrchr(path, '/');
   if (!filepart)
      filepart = path;
   else
      filepart++;
   adiak_namevalue("executable", adiak_general, "%s", filepart);
   return 0;
}

int adiak_executablepath()
{
   char path[4097];
   char *result;

   memset(path, 0, sizeof(path));
   result = realpath("/proc/self/exe", path);
   if (!result)
      return -1;
   
   adiak_namevalue("executablepath", adiak_general, "%p", path);
   return 0;
}

typedef struct {
   char **names;
   int cur;
} lib_info_t;

int adiak_libraries()
{
   int num_libraries = 0, result;
   lib_info_t linfo;

   dl_iterate_phdr(count_libraries, &num_libraries);
   linfo.names = (char **) malloc(sizeof(char*) * num_libraries);
   linfo.cur = 0;
   dl_iterate_phdr(get_library_name, &linfo);

   result = adiak_namevalue("libraries", adiak_general, "[%p]", linfo.names, linfo.cur);
   free(linfo.names);
   return result;
}

int adiak_cmdline()
{
   FILE *f = NULL;
   long size = 0;
   size_t bytes_read;
   char *buffer = NULL;
   int result, myargc = 0, i = 0, j = 0;
   char **myargv = NULL;
   int retval = -1;
   
   f = fopen("/proc/self/cmdline", "r");
   if (!f)
      goto error;
   while (!feof(f)) {
      fgetc(f);
      size += 1;
   }
   size--;
   buffer = (char *) malloc(size+1);
   if (!buffer)
      goto error;
   result = fseek(f, SEEK_SET, 0);
   bytes_read = fread(buffer, 1, size, f);

   for (i = 0; i < bytes_read; i++) {
      if (buffer[i] == '\0')
         myargc++;
   }

   myargv = (char **) malloc(sizeof(char *) * (myargc+1));

   myargv[j++] = buffer;
   for (i = 0; i < bytes_read; i++) {
      if (buffer[i] == '\0')
          myargv[j++] = buffer + i + 1;
   }
          
   result = adiak_namevalue("cmdline", adiak_general, "{%s}", myargv, myargc);
   if (result == -1)
      return -1;

   retval = 0;
  error:
   if (myargv)
      free(myargv);
   if (buffer)
      free(buffer);
   if (f)
      fclose(f);
   return retval;
}

int adiak_hostname()
{
   char hostname[512];
   int result;
   
   result = gethostname(hostname, sizeof(hostname));
   if (result == -1)
      return -1;
   hostname[sizeof(hostname)-1] = '\0';
   
   return adiak_namevalue("hostname", adiak_general, "%s", hostname);
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
   result = gethostname(hostname, sizeof(hostname));
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

   return adiak_namevalue("cluster", adiak_general, "%s", clustername);
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

int adiak_job_size()
{
#if defined(MPI_VERSION)
   int size;
   if (!use_mpi)
      return -1;
   MPI_Comm_size(adiak_communicator, &size);
   return adiak_namevalue("jobsize", adiak_general, "%d", size);
#else
   return -1;
#endif
}

int adiak_hostlist()
{
#if MPI_VERSION
   int num_hosts = 0, max_hostlen = 0, result, i;
   char *hostlist = NULL;
   char **hostlist_array = NULL;
   int err_ret = -1;

   if (!use_mpi)
      return -1;
   
   result = gethostlist(&hostlist, &num_hosts, &max_hostlen);
   if (result == -1)
      goto error;

   if (!hostlist)
      goto done;

   hostlist_array = malloc(sizeof(char *) * num_hosts);
   for (i = 0; i < num_hosts; i++)
      hostlist_array[i] = hostlist + (i * max_hostlen);

   result = adiak_namevalue("hostlist", adiak_general, "[%s]", hostlist_array, num_hosts);
   if (result == -1)
      goto error;

  done:
   err_ret = 0;
  error:
   if (hostlist)
      free(hostlist);
   if (hostlist_array)
      free(hostlist_array);
   return err_ret;
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
      adiak_communicator = *communicator;
      MPI_Comm_rank(adiak_communicator, &rank);
      global_adiak->reportable_rank = (rank == 0);
      use_mpi = 1;
   }
}
#else
void adiak_init(void *unused)
{
   adiak_common_init();
}
#endif

static struct timeval starttime()
{
   struct stat buf;
   struct timeval tv;
   int result;
   tv.tv_sec = tv.tv_usec = 0;
   result = stat("/proc/self", &buf);
   if (result == -1)
      return tv;

   tv.tv_sec = buf.st_mtime;
   tv.tv_usec = buf.st_mtim.tv_nsec / 1000;
   return tv;
}

static int count_libraries(struct dl_phdr_info *info, size_t size, void *cnt) {
   int *count = (int *) cnt;
   if (info->dlpi_name && *info->dlpi_name)
      (*count)++;
   return 0;
}

static int get_library_name(struct dl_phdr_info *info, size_t size, void *data) {
   lib_info_t *linfo = (lib_info_t *) data;
   if (!info->dlpi_name || !*info->dlpi_name)
      return 0;
   linfo->names[linfo->cur++] = (char *) info->dlpi_name;
   return 0;
}

static int measure_times(int systime, int cputime)
{
   clock_t tics;
   struct tms buf;
   const char *name;
   long tics_per_sec;
   struct timeval v;

   tics = times(&buf);
   if (tics == (clock_t) -1)
      return -1;
      

   if (systime) {
      tics = buf.tms_stime;
      name = "systime";
   }
   else if (cputime) {
      tics = buf.tms_utime;
      name = "cputime";
   }
   else
      return -1;

   errno = 0;
   tics_per_sec = sysconf(_SC_CLK_TCK);
   if (errno) {
      tics_per_sec = 100;
   }
      
   v.tv_sec = tics / tics_per_sec;
   v.tv_usec = (tics % tics_per_sec) * (1000000 / tics_per_sec);
   return adiak_namevalue(name, adiak_performance, "%t", &v);
}

static int measure_walltime()
{
   struct timeval stime;
   struct timeval etime, diff;
   int result;

   stime = starttime();
   if (stime.tv_sec == 0 && stime.tv_usec == 0)
      return -1;

   result = gettimeofday(&etime, NULL);
   if (result == -1)
      return -1;

   diff.tv_sec = etime.tv_sec - stime.tv_sec;
   if (etime.tv_usec < stime.tv_usec) {
      diff.tv_usec = 1000000 + etime.tv_usec - stime.tv_usec;
      diff.tv_sec--;
   }
   else
      diff.tv_usec = etime.tv_usec - stime.tv_usec;
   
   return adiak_namevalue("walltime", adiak_performance, "%t", &diff);
}

void adiak_fini()
{
   if (measure_adiak_cputime)
      measure_times(0, 1);
   if (measure_adiak_systime)
      measure_times(1, 0);
   if (measure_adiak_walltime)
      measure_walltime();
}

static void adiak_common_init()
{
   char *errstr;
   static int initialized = 0;
   if (initialized)
      return;
   initialized = 1;
   
   dlerror();
   global_adiak = (adiak_t *) dlsym(RTLD_DEFAULT, "adiak_public");
   errstr = dlerror();
   if (errstr)
      global_adiak = &adiak_public;

   if (ADIAK_VERSION < global_adiak->minimum_version)
      global_adiak->minimum_version = ADIAK_VERSION;
   tool_list = global_adiak->tool_list;
}

static void adiak_register(int adiak_version, adiak_category_t category,
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
   newtool->next = *tool_list;
   newtool->prev = NULL;
   if (*tool_list) 
      (*tool_list)->prev = newtool;
   *tool_list = newtool;
   if (report_on_all_ranks && !global_adiak->report_on_all_ranks)
      global_adiak->report_on_all_ranks = 1;
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
   free(t);
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
         target->v_ptr = newvalues;
         return sizeof(void*);
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

#if defined(MPI_VERSION)
#define MAX_HOSTNAME_LEN 1025

static int hostname_color(char *str, int index)
{
   int hash = 5381;
   int c;
   
   while ((c = *str++))
      hash = ((hash << 5) + hash) + (c^index); /* hash * 33 + c */
   
   return hash;
}

static int get_unique_host(char *name, int global_rank)
{
   int size, rank, color;
   int set_oldcomm = 0;
   MPI_Comm newcomm = MPI_COMM_NULL, oldcomm = adiak_communicator;
   char oname[MAX_HOSTNAME_LEN];
   int result, err_ret = -1;
   int index = 0;

   MPI_Comm_rank(adiak_communicator, &global_rank);

   for (;;) {
      color = hostname_color(name, index++);
      if (color < 0)
         color *= -1;
      
      result = MPI_Comm_split(oldcomm, color, global_rank, &newcomm);
      if (result != MPI_SUCCESS) {
         goto error;
      }
      
      if (set_oldcomm) {
         MPI_Comm_free(&oldcomm);
         oldcomm = MPI_COMM_NULL;
      }

      MPI_Comm_rank(newcomm, &rank);
      MPI_Comm_size(newcomm, &size);
      
      if (rank == 0)
         strncpy(oname, name, MAX_HOSTNAME_LEN);
      result = MPI_Bcast(oname, MAX_HOSTNAME_LEN, MPI_CHAR, 0, newcomm);
      if (result != MPI_SUCCESS) {
         goto error;
      }

      int global_str_match = 0;
      int str_match = (strcmp(name, oname) == 0);
      result = MPI_Allreduce(&str_match, &global_str_match, 1, MPI_INT, MPI_LAND, newcomm);
      if (result != MPI_SUCCESS) {
         goto error;
      }

      if (global_str_match) {
         break;
      }
      
      set_oldcomm = 1;
      oldcomm = newcomm;
   }
   
   err_ret = 0;
  error:

   if (oldcomm != adiak_communicator && oldcomm != MPI_COMM_NULL)
      MPI_Comm_free(&oldcomm);
   if (newcomm != adiak_communicator && newcomm != MPI_COMM_NULL)
      MPI_Comm_free(&newcomm);
   
   if (err_ret == -1)
      return err_ret;
   return (rank == 0);
}

static int gethostlist(char **hostlist, int *num_hosts, int *max_hostlen)
{
   int unique_host;
   int rank;
   MPI_Comm hostcomm;
   char name[MAX_HOSTNAME_LEN], *firstdot;
   int namelen, hostlist_size;

   *hostlist = NULL;
   memset(name, 0, MAX_HOSTNAME_LEN);
   gethostname(name, MAX_HOSTNAME_LEN-1);
   firstdot = strchr(name, '.');
   if (firstdot)
      *firstdot = '\0';
   namelen = strlen(name);

   MPI_Comm_rank(adiak_communicator, &rank);

   unique_host = get_unique_host(name, rank);
   if (unique_host == -1)
      return -1;

   MPI_Comm_split(adiak_communicator, unique_host, rank, &hostcomm);
   if (unique_host) {
      MPI_Comm_size(hostcomm, num_hosts);
      MPI_Allreduce(&namelen, max_hostlen, 1, MPI_INT, MPI_MAX, hostcomm);
      *max_hostlen += 1;

      hostlist_size = (*max_hostlen) * (*num_hosts);
      if (rank == 0)
         *hostlist = (char *) malloc(hostlist_size);
      MPI_Gather(name, *max_hostlen, MPI_CHAR, *hostlist, *max_hostlen, MPI_CHAR, 0, hostcomm);
   }

   MPI_Comm_free(&hostcomm);

   if (!global_adiak->report_on_all_ranks)
      return 0;

   MPI_Bcast(max_hostlen, 1, MPI_INT, 0, adiak_communicator);
   MPI_Bcast(num_hosts, 1, MPI_INT, 0, adiak_communicator);   
   hostlist_size = (*max_hostlen) * (*num_hosts);
   if (!(*hostlist))
       *hostlist = (char *) malloc(hostlist_size);
   MPI_Bcast(*hostlist, hostlist_size, MPI_CHAR, 0, adiak_communicator);

   return 0;
}
#endif

