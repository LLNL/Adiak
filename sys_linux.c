#include "adiak.h"
#include "adiak_internal.h"

#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>

#include <sys/stat.h>

#include <time.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static adiak_tool_t *local_tool_list = NULL;

adiak_t adiak_public = { ADIAK_VERSION, ADIAK_VERSION, 0, 1, &local_tool_list };

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

int adiak_launchdate()
{
   struct timeval stime = starttime();
   if (stime.tv_sec == 0 && stime.tv_usec == 0)
      return -1;
   adiak_value("date", adiak_general, "%D", stime.tv_sec);
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

int adiak_executablepath()
{
   char path[4097];
   char *result;

   memset(path, 0, sizeof(path));
   result = realpath("/proc/self/exe", path);
   if (!result)
      return -1;
   
   adiak_value("executablepath", adiak_general, "%p", strdup(path));
   return 0;
}

typedef struct {
   char **names;
   int cur;
} lib_info_t;

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
   linfo->names[linfo->cur++] = strdup(info->dlpi_name);
   return 0;
}

int adiak_libraries()
{
   int num_libraries = 0, result;
   lib_info_t linfo;

   dl_iterate_phdr(count_libraries, &num_libraries);
   linfo.names = (char **) malloc(sizeof(char*) * num_libraries);
   linfo.cur = 0;
   dl_iterate_phdr(get_library_name, &linfo);

   result = adiak_value("libraries", adiak_general, "[%p]", linfo.cur, linfo.names);
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

int adiak_measure_walltime()
{
   struct timeval stime;
   struct timeval etime, *diff;
   int result;

   stime = starttime();
   if (stime.tv_sec == 0 && stime.tv_usec == 0)
      return -1;

   result = gettimeofday(&etime, NULL);
   if (result == -1)
      return -1;

   diff = malloc(sizeof(struct timeval));
   diff->tv_sec = etime.tv_sec - stime.tv_sec;
   if (etime.tv_usec < stime.tv_usec) {
      diff->tv_usec = 1000000 + etime.tv_usec - stime.tv_usec;
      diff->tv_sec--;
   }
   else
      diff->tv_usec = etime.tv_usec - stime.tv_usec;
   
   return adiak_value("walltime", adiak_performance, "%t", diff);   
}

adiak_t* adiak_sys_init()
{
   char* errstr = NULL;
   adiak_t* global_adiak = NULL;
   
   dlerror();
   global_adiak = (adiak_t *) dlsym(RTLD_DEFAULT, "adiak_public");
   errstr = dlerror();
   if (errstr)
      global_adiak = &adiak_public;

   return global_adiak;
}
