// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

/**
 * \file adiak.h
 * \brief The Adiak C API
 */

#if !defined(ADIAK_H_)
#define ADIAK_H_

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ADIAK_VERSION 0
#define ADIAK_MINOR_VERSION 2
#define ADIAK_POINT_VERSION 2

/** \brief Determine if Adiak supports "long long" types */
#define ADIAK_HAVE_LONGLONG 1

/**
 * \addtogroup UserAPI
 * \{
 * \name Data type representation
 * \{
 */

// ADIAK DATA CATEGORIES
// Please treat values through 1000 as reserved. 1001 onward can be used as
// custom categories

/** \brief Adiak category unset */
#define adiak_category_unset 0
/** \brief Any Adiak category (for use in tool API) */
#define adiak_category_all 1
/** \brief Adiak category for general information about runs */
#define adiak_general 2
/** \brief Adiak category for information about performance */
#define adiak_performance 3
/** \brief Adiak category for control and meta commands (e.g., flush) */
#define adiak_control 4

/**
 * \brief Describes value category of an Adiak datatype.
 */
typedef enum {
   adiak_numerical_unset = 0,
   adiak_categorical, /**< Means values are unique */
   adiak_ordinal,     /**< Means the value has an ordering */
   adiak_interval,    /**< Means the value has ranges (can be subtracted) */
   adiak_rational     /**< Means the value is divisible */
} adiak_numerical_t;

/**
 * \brief Adiak datatype descriptor.
 *
 * The "typestring" values shown below are the printf-style type descriptors
 * that must be passed as the \a typestr argument in \ref adiak_namevalue for
 * values of the corresponding type.
 */
typedef enum {
   adiak_type_unset = 0, /**< Can be used to un-set a previously set name/value pair */
   adiak_long,        /**< typestring: "%ld" (implies rational) */
   adiak_ulong,       /**< typestring: "%lu" (implies rational) */
   adiak_int,         /**< typestring: "%d" (implies rational) */
   adiak_uint,        /**< typestring: "%u" (implies rational) */
   adiak_double,      /**< typestring: "%f" (implies rational) */
   adiak_date,        /**< typestring: "%D" (implies interval), passed as a signed long in seconds since epoch */
   adiak_timeval,     /**< typestring: "%t" (implies interval), passed as a 'struct timeval *' */
   adiak_version,     /**< typestring: "%v" (implies oridinal), passed as a char*  */
   adiak_string,      /**< typestring: "%s" (implies oridinal), passed as a char*  */
   adiak_catstring,   /**< typestring: "%r" (implies categorical), passed as a char* */
   adiak_path,        /**< typestring: "%p" (implies categorical), passed as a char* */
   adiak_range,       /**< typestring: "<subtype>" (implies categorical), */
   adiak_set,         /**< typestring: "[subtype]", passed as a size integer */
   adiak_list,        /**< typestring: "{subtype}", passed as a size integer */
   adiak_tuple,       /**< typestring: "(subtype1, subtype2, ..., subtypeN)", passed as an N integer */
   adiak_longlong,    /**< typestring: "%lld" (implies rational) */
   adiak_ulonglong    /**< typestring: "%llu" (implies rational) */
} adiak_type_t;

typedef int adiak_category_t;

/**
 * \brief Adiak datatype specification.
 *
 * This specification contains the datatype's descriptor, category, and
 * (for compound types) its potentially recursive sub-types.
 */
typedef struct adiak_datatype_t {
   /** \brief Type descriptor */
   adiak_type_t dtype;
   /** \brief Value category of this datatype */
   adiak_numerical_t numerical;
   /** \brief Number of elements for compound types (e.g., number of list elements). */
   int num_elements;
   /** \brief Number of sub-types.
    *
    * Should be N for tuples, 1 for other compound types, and 0 for basic types.
    */
   int num_subtypes;
   /** \brief List of subtypes of compound types. NULL for other types. */
   struct adiak_datatype_t **subtype;
} adiak_datatype_t;

/** \brief Adiak datatype value
 *
 * The adiak_value_t union contains the value part of a name/value pair.
 * It is used with an adiak_datatype_t, which describes how interpret the
 * value. The following table describes what union value should be read given
 * an accompanying adiak_type_t:
 *
 * Type            | Union value
 * ----------------|------------
 * adiak_long      | v_long
 * adiak_ulong     | v_long (cast to unsigned long)
 * adiak_int       | v_int
 * adiak_uint      | v_int (cast to unsigned int)
 * adiak_double    | v_double
 * adiak_date      | v_long (as seconds since epoch)
 * adiak_timeval   | v_ptr (cast to struct timeval*)
 * adiak_version   | v_ptr (cast to char*)
 * adiak_string    | v_ptr (cast to char*)
 * adiak_catstring | v_ptr (cast to char*)
 * adiak_path      | v_ptr (cast to char*)
 * adiak_range     | v_subval (as adiak_value_t[2])
 * adiak_set       | v_subval (as adiak_value_t array)
 * adiak_list      | v_subval (as adiak_value_t array)
 * adiak_tuple     | v_subval (as adiak_value_t array)
 * adiak_longlong  | v_longlong
 * adiak_ulonglong | v_longlong (as unsigned long long)
 */
typedef union adiak_value_t {
   signed long v_long;
   int v_int;
   short v_short;
   char v_char;
   double v_double;
   void *v_ptr;
   union adiak_value_t *v_subval;
   long long v_longlong;
} adiak_value_t;

static const adiak_datatype_t adiak_unset_datatype = { adiak_type_unset, adiak_numerical_unset, 0, 0, NULL };

/**
 * \}
 * \}
 *
 * \addtogroup UserAPI
 * \{
 * \name C user API
 * \{
 */

/**
 * \brief Initializes the Adiak interface.
 *
 * Must be called by the application before registering name/value pairs.
 *
 * In an MPI job, adiak_init takes a pointer to an MPI communicator. MPI_COMM_WORLD
 * would typically be the correct communicator to pass. The init routine should be
 * called after MPI_Init has completed from each rank in the MPI job.
 *
 * This routine can safely be called multiple times. Subsequent calls have no
 * effect.
 *
 * \param mpi_communicator_p Pointer to an MPI communicator, cast to void*. NULL
 *   if running without MPI.
 */
void adiak_init(void *mpi_communicator_p);

/**
 * \brief Finalizes the Adiak interface.
 *
 * adiak_fini should be called before program exit. In an MPI job, it should be called
 * before MPI_finalize.
 */
void adiak_fini();

/**
 * \brief Register a name/value pair with Adiak.
 *
 * Values are associated with the specified \a name and described by the specified type.
 * The printf-style \a typestr describes the type of the value, which is constructed
 * from the string specifiers shown in \ref adiak_type_t. The varargs contains parameters
 * for the type. The entire type describes how value is encoded. For example:
 *
 * \code
 * adiak_namevalue("numrecords", adiak_general, NULL, "%d", 10);
 *
 * adiak_namevalue("buildcompiler", adiak_general, "build data", "%v", "gcc@4.7.3");
 *
 * double gridvalues[] = { 5.4, 18.1, 24.0, 92.8 };
 * adiak_namevalue("gridvals", adiak_general, NULL, "[%f]", gridvalues, 4);
 *
 * struct { int pos; const char *val; } letters[3] = { {1, "a"}, {2, "b"}, {3, "c"} };
 * adiak_namevalue("alphabet", adiak_general, NULL, "[(%u, %s)]", letters, 3, 2);
 * \endcode
 *
 * \param name Name of the Adiak name/value pair. Adiak makes a copy of the string, and
 *   memory associated with it can be freed after this call completes.
 * \param category Describes how the name/value pair should be categorized. Tools can
 *   subscribe to a set of categories. Users can use Adiak-provided categories like
 *   \ref adiak_general, \ref adiak_performance, etc. or create their own. Values <=1000
 *   are reserved for Adiak. User-defined categories must have a value >1000.
 *   The \ref adiak_general category is a good default value.
 * \param subcategory An optional user-defined category string that is passed to
 *   underlying tools. Can be NULL.
 * \param typestr A printf-style type string describing the datatype of the value.
 *   As example, the \a typestr could be "%d", "%s", or "[%p]" to describe an integer,
 *   string, or set-of-paths value, respectively. See \ref adiak_type_t.
 * \param ... The value of the name/value pair. It is interpreted based on the
 *   \a typestr. For scalar types and strings,
 *   this is just the value itself. For compound types like arrays or tuples, this
 *   is an array or struct pointer followed by the list dimensions, from the outermost
 *   to the innermost type (see examples above).
 * \returns On success, returns 0. On a failure, returns -1.
 *
 * There is also a convenient C++ interface for registering values, see adiak::value.
 */
int adiak_namevalue(const char *name, int category, const char *subcategory, const char *typestr, ...);

/**
 * \brief Constructs a new adiak_datatype_t that can be passed to adiak_raw_namevalue.
 *
 * \note It is not safe to free this datatype.
 */
adiak_datatype_t *adiak_new_datatype(const char *typestr, ...);

/**
 * \brief Register a new name/value pair with Adiak, but with an already constructed datatype
 *    and value.
 */
int adiak_raw_namevalue(const char *name, int category, const char *subcategory,
                        adiak_value_t *value, adiak_datatype_t *type);

/** \brief Makes a 'user' name/val with the real name of who's running the job */
int adiak_user();
/** \brief Makes a 'uid' name/val with the uid of who's running the job */
int adiak_uid();
/** \brief Makes a 'launchdate' name/val with the date of when this job started
 *
 * Creates an adiak_date value named "launchdate" of the time when this process was started,
 * but rounded down to the previous midnight at GMT+0. This can be used to group jobs that
 * ran on the same day.
 */
int adiak_launchdate();
/** \brief Makes a 'launchday' name/val with date when this job started, but truncated to midnight */
int adiak_launchday();
/** \brief Makes an 'executable' name/val with the executable file for this job */
int adiak_executable();
/** \brief Makes an 'executablepath' name/value with the full executable file path. */
int adiak_executablepath();
/** \brief Makes a 'working_directory' name/val with the cwd for this job */
int adiak_workdir();
/** \brief Makes a 'libraries' name/value with the set of shared library paths. */
int adiak_libraries();
/** \brief Makes a 'cmdline' name/val string set with the command line parameters */
int adiak_cmdline();
/** \brief Makes a 'hostname' name/val with the hostname */
int adiak_hostname();
/** \brief Makes a 'cluster' name/val with the cluster name (hostname with numbers stripped) */
int adiak_clustername();
/** \brief Makes a 'walltime' name/val with the walltime how long this job ran */
int adiak_walltime();
/** \brief Makes a 'systime' name/val with the timeval of how much time was spent in IO */
int adiak_systime();
/** \brief Makes a 'cputime' name/val with the timeval of how much time was spent on the CPU */
int adiak_cputime();

/** \brief Makes a 'jobsize' name/val with the number of ranks in an MPI job */
int adiak_job_size();
/** \brief Makes a 'hostlist' name/val with the set of hostnames in this MPI job */
int adiak_hostlist();
/** \brief Makes a 'numhosts' name/val with the number of hosts in this MPI job */
int adiak_num_hosts();

/** \brief Flush values through Adiak. Currently unused. */
int adiak_flush(const char *location);
/** \brief Clear all adiak name/values.
 *
 * This routine frees all heap memory used by adiak. This includes the cache of all
 * name/value pairs passed to Adiak. After this call completes, adiak will report it
 * has no name/value pairs. Adiak should not be used again after this call completes.
 */
int adiak_clean();

/** \brief Return the value category for \a dtype */
adiak_numerical_t adiak_numerical_from_type(adiak_type_t dtype);

/**
 * \}
 * \}
 */

#if defined(__cplusplus)
}
#endif

#endif
