// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include "adksys.h"

#include <unistd.h>

#include <libproc.h>

int adksys_starttime(struct timeval *tv)
{
   pid_t pid;
   struct proc_bsdinfo proc;
   pid = getpid();
   int st = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0,
                         &proc, PROC_PIDTBSDINFO_SIZE);
   if (st != PROC_PIDTBSDINFO_SIZE) {
     return -1;
   }
   tv->tv_sec = proc.pbi_start_tvsec;
   tv->tv_usec = proc.pbi_start_tvusec;
   return 0;
}

int adksys_get_executable(char *outpath, size_t outpath_size)
{
   (void)outpath;
   (void)outpath_size;
   return -1;
}

int adksys_get_cmdline_buffer(char **output_buffer, int *output_size)
{
   (void)output_buffer;
   (void)output_size;
   return -1;
}

int adksys_get_libraries(char ***libraries, int *libraries_size, int *libnames_need_free)
{
   (void)libraries;
   (void)libraries_size;
   (void)libnames_need_free;
   return -1;
}

void *adksys_get_public_adiak_symbol()
{
   return NULL;
}
