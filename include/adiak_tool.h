// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

/**
 * \file adiak_tool.h
 * \brief The Adiak tool API
 */

#if !defined(ADIAK_TOOL_H_)
#define ADIAK_TOOL_H_

#include "adiak.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup ToolAPI
 * \{
 * \name Adiak tool API
 * \{
 */

/**
 * \brief Callback function for processing an Adiak name/value pair
 *
 * This callback function is called once for each queried name/value pair.
 *
 * \param name Name of the name/value pair
 * \param category The Adiak category, e.g. \ref adiak_general
 * \param subcategory Optional user-defined sub-category. Can be NULL.
 * \param value The value of the name/value pair. Refer to \ref adiak_value_t to see which
 *   union value to choose based on the datatype \a t.
 * \param t The datatype specification of the name/value pair.
 * \param opaque_value Optional user-defined pass-through argument
 *
 * \sa adiak_register_cb, adiak_list_namevals
 */
typedef void (*adiak_nameval_cb_t)(const char *name, int category, const char *subcategory, adiak_value_t *value, adiak_datatype_t *t, void *opaque_value);

/**
 * \brief Register a callback function to be invoked when a name/value pair is set
 *
 * \param[in] adiak_version Adiak API version. Currently 1.
 * \param[in] category The Adiak category (e.g., \ref adiak_general) to capture.
 *   Callbacks will only be invoked for name/values of \a category. Can be
 *   \ref adiak_category_all to capture all name/value pairs.
 * \param[in] nv User-provided callback function
 * \param[in] report_on_all_ranks If set to 0, reports only on the root rank in an MPI program.
 *   Otherwise reports on all ranks.
 * \param[in] opaque_val User-provided value passed through to the callback function.
 */
void adiak_register_cb(int adiak_version, int category, adiak_nameval_cb_t nv, int report_on_all_ranks, void *opaque_val);

/**
 * \brief Iterate over the name/value pairs currently registered with Adiak
 *
 * Iterates over all currently set name/value pairs of \a category and invokes the callback
 * function \a nv for each name/value pair.
 *
 * \param[in] adiak_version Adiak API version. Currently 1.
 * \param[in] category The Adiak category (e.g., \ref adiak_general) to capture.
 *   Callbacks will only be invoked for name/values of \a category. Can be
 *   \ref adiak_category_all to capture all name/value pairs.
 * \param[in] nv Pointer to the user-provided callback function.
 * \param[in] opaque_val User-provided value passed through to the callback function.
 */
void adiak_list_namevals(int adiak_version, int category, adiak_nameval_cb_t nv, void *opaque_val);

/**
 * \brief Return the type string descriptor for an Adiak datatype specification
 *
 * \param[in] t The Adiak datatype specification
 * \param[in] long_form If non-zero, the string will be human readable, such as "set of int".
 *   If zero, the string will be in the printf-style format described in \ref adiak_type_t.
 *   documentation, such as "[%d]".
 *
 * \note The returned string is malloc'd. The user must free the returned string pointer.
 */
char *adiak_type_to_string(adiak_datatype_t *t, int long_form);

/**
 * \brief Query the currently set value for \a name
 *
 * \param[in] name Name of the name/value pair to query
 * \param[out] t The datatype specification of the name/value pair
 * \param[out] value Value of the name/value pair
 * \param[out] category The Adiak category, e.g. \ref adiak_general
 * \param[out] subcat Optional user-defined sub-category. Can be NULL.
 */
int adiak_get_nameval(const char *name, adiak_datatype_t **t, adiak_value_t **value, int *category, const char **subcat);

/**
 * \}
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif
