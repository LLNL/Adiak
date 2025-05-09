// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#define _XOPEN_SOURCE 600
#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

#include "adksys.h"

int adksys_get_times(struct timeval *sys, struct timeval *cpu)
{
   clock_t tics;
   struct tms buf;
   long tics_per_sec;

   errno = 0;
   tics_per_sec = sysconf(_SC_CLK_TCK);
   if (errno)
      tics_per_sec = 100;

   tics = times(&buf);
   if (tics == (clock_t) -1)
      return -1;

   if (sys) {
      tics = buf.tms_stime;
      sys->tv_sec = tics / tics_per_sec;
      sys->tv_usec = (tics % tics_per_sec) * (1000000 / tics_per_sec);
   }
   if (cpu) {
      tics = buf.tms_utime;
      cpu->tv_sec = tics / tics_per_sec;
      cpu->tv_usec = (tics % tics_per_sec) * (1000000 / tics_per_sec);
   }

   return 0;
}

int adksys_curtime(struct timeval *tm) {
   return gettimeofday(tm, NULL);
}

int adksys_clock_realtime(struct timespec *ts) {
   return clock_gettime(CLOCK_REALTIME, ts);
}

int adksys_hostname(char *outbuffer, int buffer_size)
{
   int result = gethostname(outbuffer, buffer_size);
   if (result == -1)
      return -1;
   outbuffer[buffer_size-1] = '\0';
   return 0;
}
