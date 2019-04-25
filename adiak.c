#define _GNU_SOURCE
#include "adiak.h"
#include "adiak_tool.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <pwd.h>

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

const adiak_datatype_t adiak_v1_types = {adiak_rational, adiak_set, adiak_path, adiak_performance};
static const adiak_datatype_t adiak_types_by_version[] = { adiak_v1_types };

#if defined(MPI_VERSION)
static MPI_Comm adiak_communicator;
static int use_mpi;
#endif

static void adiak_common_init();
static int format_match(const char *users, const char *reference);
static adiak_valentries_t *get_valentry_from_char(const char *c);
static int get_size_from_type(adiak_datatype_t valtype);
static int adiak_nameval(const char *name, void *buffer, size_t buffer_size, size_t elem_size, adiak_datatype_t valtype);
static int adiak_request(adiak_request_t req, int request_version);
static struct passwd *get_passwd();
static void adiak_register(int adiak_version, adiak_category_t category,
                           adiak_nameval_cb_t nv, adiak_request_cb_t rq,
                           int report_on_all_ranks, void *opaque_val);

#if defined(MPI_VERSION)
static int hostname_color(char *str, int index);
static int get_unique_host(char *name, int global_rank);
static int gethostlist(char **hostlist, int *num_hosts, int *max_hostlen);
#endif

static const adiak_datatype_t adiak_unset_datatype = { adiak_numerical_unset, adiak_grouping_unset, adiak_type_unset, adiak_category_unset };

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

   result = adiak_nameval(name, (void *) buffer, buffer_size, elem_size, valtype);
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

int adiak_rawval(const char *name, void *val, size_t val_size, adiak_datatype_t valtype)
{
   int elem_size;

   elem_size = get_size_from_type(valtype);
   return adiak_nameval(name, (void *) val, val_size, elem_size, valtype);
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

int adiak_user()
{
   struct passwd *p;
   char *firstcomma, *user;
  
   p = get_passwd();
   if (!p)
      return -1;
   user = p->pw_gecos;
   firstcomma = strchr(user, ',');
   if (firstcomma)
      *firstcomma = '\0';

   adiak_value("user", adiak_general, "%s", user);
   return 0;
}

int adiak_uid()
{  
   struct passwd *p;
   
   p = get_passwd();
   if (!p)
      return -1;
   
   adiak_value("uid", adiak_general, "%s", p->pw_name);
   return 0;
}

int adiak_launchdate()
{
   struct stat buf;
   unsigned long modtime;
   int result;
   result = stat("/proc/self", &buf);
   if (result == -1)
      return -1;
   modtime = (unsigned long) buf.st_mtime;
   adiak_value("date", adiak_general, "%D", modtime);
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
   adiak_value("executable", adiak_general, "%s", filepart);
   return 0;
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
          
   result = adiak_value("cmdline", adiak_general, "[%s]", myargc, myargv);
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

   return adiak_value("hostname", adiak_general, "%s", hostname);
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

   return adiak_value("cluster", adiak_general, "%s", clustername);
}

int adiak_runtime()
{
   return adiak_request(walltime, 1);
}

int adiak_iotime()
{
   return adiak_request(iotime, 1);
}

int adiak_mpi_ranks()
{
#if defined(MPI_VERSION)
   int size;
   if (!use_mpi)
      return -1;
   MPI_Comm_size(adiak_communicator, &size);
   return adiak_value("mpiranks", adiak_general, "%d", size);
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

   result = adiak_value("hostlist", adiak_general, "[%s]", num_hosts, hostlist_array);
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

int adiak_mpitime()
{
#if defined(MPI_VERSION)
   if (!use_mpi)
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

static int get_size_from_type(adiak_datatype_t valtype)
{
   adiak_valentries_t *i;
   for (i = adiak_valentries; i->typename != NULL; i++) {
      if (i->dtype == valtype.dtype)
         return i->elem_size;
   }
   return -1;
}

static int adiak_nameval(const char *name, void *buffer, size_t buffer_size, size_t elem_size, adiak_datatype_t valtype)
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
         tool->name_val_cb(name, buffer, elem_size, buffer_size/elem_size, valtype, tool->opaque_val);
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

static struct passwd *get_passwd()
{
   static struct passwd pwd;
   static int found = 0;
   struct passwd *p;

   if (found == -1)
      return NULL;
   if (found)
      return &pwd;

   p = getpwuid(getuid());
   if (!p) {
      found = -1;
      return NULL;
   }
   pwd.pw_name = strdup(p->pw_name);
   pwd.pw_passwd = NULL;
   pwd.pw_uid = p->pw_uid;
   pwd.pw_gid = p->pw_gid;
   pwd.pw_gecos = strdup(p->pw_gecos);
   pwd.pw_dir = NULL;
   pwd.pw_shell = NULL;
   found = 1;
   return &pwd;
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

