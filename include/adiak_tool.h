// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#if !defined(ADIAK_TOOL_H_)
#define ADIAK_TOOL_H_

#include "adiak.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*adiak_nameval_cb_t)(const char *name, int category, const char *subcategory, adiak_value_t *value, adiak_datatype_t *t, void *opaque_value);

void adiak_register_cb(int adiak_version, int category, adiak_nameval_cb_t nv, int report_on_all_ranks, void *opaque_val);

void adiak_list_namevals(int adiak_version, int category, adiak_nameval_cb_t nv, void *opaque_val);

char *adiak_type_to_string(adiak_datatype_t *t, int long_form);

int adiak_get_nameval(const char *name, adiak_datatype_t **t, adiak_value_t **value, int *category, const char **subcat);
   
#ifdef __cplusplus
}
#endif

#endif
