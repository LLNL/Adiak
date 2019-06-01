#pragma once

#include "adiak.h"

struct adiak_tool_t;
typedef struct adiak_tool_t adiak_tool_t;

typedef struct {
   int minimum_version;
   int version;
   int report_on_all_ranks;
   int reportable_rank;
   adiak_tool_t **tool_list;
  
#if defined(MPI_VERSION)
   MPI_Comm adiak_communicator;
   int use_mpi;
#endif
} adiak_t;

extern adiak_t *global_adiak;

adiak_t* adiak_sys_init();

int adiak_measure_times(int systime, int cputime);

int adiak_measure_walltime();
