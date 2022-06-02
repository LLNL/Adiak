// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#define _GNU_SOURCE
#include <link.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "adksys.h"

typedef struct {
   char **names;
   int cur;
} lib_info_t;

static int count_libraries(struct dl_phdr_info *info, size_t size, void *cnt) {
   int *count = (int *) cnt;
   if (info->dlpi_name && *info->dlpi_name)
      (*count)++;
   (void) size;
   return 0;
}

static int get_library_name(struct dl_phdr_info *info, size_t size, void *data) {
   lib_info_t *linfo = (lib_info_t *) data;
   if (!info->dlpi_name || !*info->dlpi_name)
      return 0;
   linfo->names[linfo->cur++] = (char *) info->dlpi_name;
   (void) size;
   return 0;
}

int adksys_get_libraries(char ***libraries, int *libraries_size, int *libnames_need_free) {
   int num_libraries = 0;
   lib_info_t linfo;

   dl_iterate_phdr(count_libraries, &num_libraries);
   linfo.names = (char **) malloc(sizeof(char*) * (num_libraries+1));
   linfo.cur = 0;
   dl_iterate_phdr(get_library_name, &linfo);
   linfo.names[num_libraries] = NULL;

   *libraries = linfo.names;
   *libraries_size = num_libraries;
   *libnames_need_free = 0;
   return 0;
}

void *adksys_get_public_adiak_symbol()
{
   void *result;
   
   dlerror();
   result = dlsym(RTLD_DEFAULT, "adiak_public");
   if (dlerror())
      return NULL;
   return result;
}

