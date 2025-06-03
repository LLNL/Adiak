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

#include <time.h>

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
 * \brief Contains metadata for an adiak name/value pair
 */
typedef struct adiak_record_info_t {
    /** \brief The adiak category, e.g. \ref adiak_general */
    int category;
    /** \brief An optional user-defined subcategory */
    const char* subcategory;
    /** \brief Timestamp when the value was last set
     *
     * In POSIX systems, this is a CLOCK_REALTIME timestamp.
     */
    struct timespec timestamp;
} adiak_record_info_t;

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
 * \brief Callback function for processing an Adiak name/value pair including metadata
 *
 * This callback function is called once for each queried name/value pair. It provides
 * name, value and record metadata such as the timestamp.
 *
 * \param name Name of the name/value pair
 * \param value The value of the name/value pair. Refer to \ref adiak_value_t to see which
 *   union value to choose based on the datatype \a t.
 * \param t The datatype specification of the name/value pair.
 * \param info Metadata for the name/value pair
 * \param opaque_value Optional user-defined pass-through argument
 *
 * \sa adiak_register_cb, adiak_list_namevals
 */
typedef void (*adiak_nameval_info_cb_t)(const char *name, adiak_value_t *value, adiak_datatype_t *t, adiak_record_info_t *info, void *opaque_value);

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
 * \copydoc adiak_register_cb
 */
void adiak_register_cb_with_info(int adiak_version, int category, adiak_nameval_info_cb_t nv, int report_on_all_ranks, void *opaque_val);

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
 * \copydoc adiak_list_namevals
 */
void adiak_list_namevals_with_info(int adiak_version, int category, adiak_nameval_info_cb_t nv, void *opaque_val);

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
 * \brief Query the currently set value and info for \a name
 *
 * \param[in] name Name of the name/value pair to query
 * \param[out] t Datatype specification of the name/value pair
 * \param[out] value Value of the name/value pair
 * \param[out] info Metadata for the name/value pair
 */
int adiak_get_nameval_with_info(const char* name, adiak_datatype_t **t, adiak_value_t **value, adiak_record_info_t **info);

/**
 * \brief Return the number of sub-values for the given container type \a t
 */
int adiak_num_subvals(adiak_datatype_t* t);

/**
 * \brief Return the nth sub-value from a container-type value
 *
 * Returns a subvalue from a container value as a \ref adiak_value_t,
 * regardless of whether the container type is a reference type or
 * a Adiak copy.
 *
 * This function works for both Adiak-created deep copies (where values
 * are stored as individual \a adiak_value_t entries in \a val->v_subvals)
 * and reference entries (where the original pointer is stored as
 * \a val->v_ptr).
 *
 * Returns NULL in \a subtype and in \a subvalue.v_ptr if the given
 * value is not a container type or \a elem is out-of-bounds.
 *
 * \param[in] t The container datatype
 * \param[in] val The container value
 * \param[in] elem Index of the sub-element to return
 * \param[out] subtype Returns the selected sub-value's datatype
 * \param[out] subval Returns the selected sub-value
 */
int adiak_get_subval(adiak_datatype_t* t, adiak_value_t* val, int elem, adiak_datatype_t** subtype, adiak_value_t* subval);
/**
 * \}
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif
