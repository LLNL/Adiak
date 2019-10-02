// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include "adksys.h"

int adksys_starttime(struct timeval *tv)
{
   return -1;
}

int adksys_get_executable(char *outpath, size_t outpath_size)
{
   return -1;
}

int adksys_get_cmdline_buffer(char **output_buffer, int *output_size)
{
   return -1;
}

int adksys_get_libraries(char ***libraries, int *libraries_size, int *libnames_need_free)
{
   return -1;
}

void *adksys_get_public_adiak_symbol()
{
   return NULL;
}
