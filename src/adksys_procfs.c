// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "adksys.h"

int adksys_starttime(struct timeval *tv)
{
   struct stat buf;
   int result;
   tv->tv_sec = tv->tv_usec = 0;
   result = stat("/proc/self", &buf);
   if (result == -1)
      return -1;

   tv->tv_sec = buf.st_mtime;
   tv->tv_usec = buf.st_mtim.tv_nsec / 1000;
   return 0;
}

int adksys_get_executable(char *outpath, size_t outpath_size)
{
   char *result;

   result = realpath("/proc/self/exe", NULL);
   if (result == NULL)
      return -1;
   strncpy(outpath, result, outpath_size);
   outpath[outpath_size-1] = '\0';
   free(result);
   
   return 0;
}

int adksys_get_cmdline_buffer(char **output_buffer, int *output_size)
{
   FILE *f = NULL;
   size_t bytes_read = 0, size = 0;
   char *buffer = NULL;
   size_t result;
   
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
   memset(buffer, 0, size+1);
   
   result = fseek(f, SEEK_SET, 0);
   do {
      result = fread(buffer + bytes_read, 1, size - bytes_read, f);
      if (result == 0 && errno == EINTR)
         continue;
      else if (result == 0 && feof(f))
         break;
      else if (result == 0)
         goto error;
      bytes_read += result;
   } while (bytes_read < size);

   fclose(f);
   *output_buffer = buffer;
   *output_size = size;
   return 0;
   
  error:
   if (f)
      fclose(f);
   if (buffer)
      free(buffer);
   return -1;
}
