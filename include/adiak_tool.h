#if !defined(ADIAK_TOOL_H_)
#define ADIAK_TOOL_H_

#include "adiak.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*adiak_nameval_cb_t)(const char *name, adiak_category_t category, adiak_value_t *value, adiak_datatype_t *t, void *opaque_value);

void adiak_register_cb(int adiak_version, adiak_category_t category, adiak_nameval_cb_t nv, int report_on_all_ranks, void *opaque_val);

void adiak_list_namevals(int adiak_version, adiak_category_t category, adiak_nameval_cb_t nv, void *opaque_val);

char *adiak_type_to_string(adiak_datatype_t *t, int long_form);

#ifdef __cplusplus
}
#endif

#endif
