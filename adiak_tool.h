#if !defined(ADIAK_TOOL_H_)
#define ADIAK_TOOL_H_

#include "adiak.h"

typedef enum {
   walltime,
   iotime,
   mpitime
} adiak_request_t;

typedef void (*adiak_nameval_cb_t)(const char *name, void *elems, size_t elem_size, size_t num_elems, adiak_datatype_t valtype, void *opaque_val);
typedef void (*adiak_request_cb_t)(adiak_request_t, void*);

void adiak_register_cb(int adiak_version, adiak_category_t category, adiak_nameval_cb_t nv, int report_on_all_ranks, void *opaque_val);
void adiak_register_request(int adiak_version, adiak_category_t category, adiak_request_cb_t req, int report_on_all_ranks, void *opaque_val);

#endif
