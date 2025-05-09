// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#if !defined(ADKSYS_H_)
#define ADKSYS_H_

#include <sys/time.h> /* struct timeval */
#include <time.h>

int adksys_get_libraries(char ***libraries, int *libraries_size, int *libnames_need_free);
int adksys_hostlist(char ***out_hostlist_array, int *out_num_hosts, char **out_name_buffer, int all_ranks);
int adksys_jobsize(int *size);
int adksys_reportable_rank();
int adksys_mpi_init(void *mpi_communicator_p);
int adksys_get_times(struct timeval *sys, struct timeval *cpu);
int adksys_curtime(struct timeval *tm);
int adksys_clock_realtime(struct timespec* ts);
int adksys_hostname(char *outbuffer, int buffer_size);
int adksys_starttime(struct timeval *tv);
int adksys_get_executable(char *outpath, size_t outpath_size);
int adksys_get_cmdline_buffer(char **output_buffer, int *output_size);
int adksys_get_names(char **uid, char **user);
int adksys_mpi_initialized();
int adksys_get_cwd(char *cwd, size_t max_size);
void *adksys_get_public_adiak_symbol();
int adksys_mpi_version(char* output, size_t output_size);
int adksys_mpi_library(char* output, size_t output_size);
int adksys_mpi_library_version(char* vendor, size_t vendor_len, char* version, size_t version_len);

#endif
