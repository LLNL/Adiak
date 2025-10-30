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
#define ADIAK_MINOR_VERSION 5
#define ADIAK_POINT_VERSION 0

/** \brief Adiak supports "long long" types */
#define ADIAK_HAVE_LONGLONG 1
/** \brief Adiak supports jsonstring type */
#define ADIAK_HAVE_JSONSTRING 1
/** \brief Adiak supports timestamp values in record info */
#define ADIAK_HAVE_TIMESTAMPS 1

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
 *
 * For integer and floating point scalar values, an optional size specifier
 * can be added in the typestring to specify the exact input type (8, 16,
 * 32, or 64 bit). The available specifiers are i8, i16, i32, i64, u8, u16,
 * u32, u64, f32, and f64. For example, use "%f32" for 32-bit float values,
 * "%i16" for signed short (16-bit) integers, etc.
 *
 * Type             | Typestring   | Value category
 * -----------------|--------------|---------------
 * adiak_long       | "%ld"        | rational
 * adiak_ulong      | "%lu"        | rational
 * adiak_int        | "%d", "%i"   | rational
 * adiak_uint       | "%u"         | rational
 * adiak_double     | "%f"         | rational
 * adiak_date       | "%D"         | interval
 * adiak_timeval    | "%t"         | interval
 * adiak_version    | "%v"         | ordinal
 * adiak_string     | "%s"         | ordinal
 * adiak_catstring  | "%r"         | categorical
 * adiak_jsonstring | "%j"         | categorical
 * adiak_path       | "%p"         | categorical
 * adiak_range      | "<subtype>"  | categorical
 * adiak_set        | "[subtype]"  | categorical
 * adiak_list       | "{subtype}"  | categorical
 * adiak_tuple      | "(t1,t2,..)" | categorical
 * adiak_longlong   | "%lld"       | rational
 * adiak_ulonglong  | "%llu"       | rational
 */
typedef enum {
   /** \brief A placeholder for uninitialized types */
   adiak_type_unset = 0,
   /** \brief A C \c long */
   adiak_long,
   /** \brief A C <tt> unsigned long </tt> */
   adiak_ulong,
   /** \brief A C \c int */
   adiak_int,
   /** \brief A C <tt> unsigned int </tt> */
   adiak_uint,
   /** \brief A C \c double */
   adiak_double,
   /** \brief A date. Passed as a signed long in seconds since epoch. */
   adiak_date,
   /** \brief A time interval. Passed as a <tt> struct timeval* </tt> */
   adiak_timeval,
   /** \brief A program version number. Passed as \c char*. */
   adiak_version,
   adiak_string,
   /** \brief A categorical (i.e., unordered) string. Passed as \c char*. */
   adiak_catstring,
   /** \brief A file path. Passed as \c char*. */
   adiak_path,
   /** \brief A compound type representing a range of values.
    *
    * Another typestring should be passed between the "< >" arrow brackets,
    * e.g. "<%d>" for an interval of ints.
    *
    * The values passed to the C API should be passed as two values
    * v1 and v2 representing the range [v1, v2). In the C++ interface,
    * use the overloaded \c adiak::value() function that takes two value
    * parameters.
    */
   adiak_range,
   /** \brief A compound type representing a set of values.
    *
    * The set is unordered and should not contain duplicates. Another typestring
    * should be passed between the "[ ]" square brackets, e.g. "[%u]" for a set
    * of uints. The values passed to the C interface should be a C-style array
    * of the subtype and a size integer.
    *
    * The value passed to the C++ interface should be a \c std::set with the
    * subtype.
    */
   adiak_set,
   /** \brief A compound type representing a list of values.
    *
    * The list order is preserved. Another typestring
    * should be passed between the "{ }" braces, e.g. "{%s}" for a list
    * of strings. The values passed to the C interface should be a C-style
    * array of the subtype and a size integer.
    *
    * The value passed to the C++ interface should be a \c std::vector or
    * a \c std::list with the subtype.
    */
   adiak_list,
   /** \brief A compound type representing a tuple.
    *
    * N typestrings should be passed between the "( )" parentheses, e.g.
    * "(%d,%s,%p)" for a (int,string,path) tuple. In the C interface, the
    * values should be passed as a pointer to a C struct with the tuple
    * values and an int with the number of tuple elements.
    *
    * In the C++ interface, the value should be a \c std::tuple with
    * N elements.
    */
   adiak_tuple,
   /** \brief A C <tt> long long </tt> */
   adiak_longlong,
   /** \brief A C <tt> unsigned long long </tt> */
   adiak_ulonglong,
   /** \brief A JSON string. Passed as \c char*. */
   adiak_jsonstring
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
   /** \brief Number of elements for container types (e.g., number of list elements).
    *
    * This is the number of sub-elements if the container is a
    * Adiak-created copy (which is the default). The number of sub-elements
    * of reference (zero copy) containers is in \ref num_ref_elements.
    */
   int num_elements;
   /** \brief Number of sub-types.
    *
    * Should be N for tuples, 1 for other container types, and 0 for basic types.
    */
   int num_subtypes;
   /** \brief List of subtypes of container types. NULL for other types. */
   struct adiak_datatype_t **subtype;
   /** \brief 1 if this a reference (i.e., zero-copy) entry, 0 if not */
   int is_reference;
   /** \brief Number of sub-elements for reference container values */
   int num_ref_elements;
   /** \brief Size in bytes of the original input data (currently only for base types) */
   size_t num_bytes;
} adiak_datatype_t;

/** \brief Adiak value union
 *
 * The adiak_value_t union contains the value part of a name/value pair.
 * It is used with an adiak_datatype_t, which describes how to interpret the
 * value. The following table describes what union value should be read for
 * each \ref adiak_type_t:
 *
 * Type             | Union value
 * -----------------|------------
 * adiak_long       | v_long
 * adiak_ulong      | v_long (cast to unsigned long)
 * adiak_int        | v_int
 * adiak_uint       | v_int (cast to unsigned int)
 * adiak_double     | v_double
 * adiak_date       | v_long (as seconds since epoch)
 * adiak_timeval    | v_ptr (cast to struct timeval*)
 * adiak_version    | v_ptr (cast to char*)
 * adiak_string     | v_ptr (cast to char*)
 * adiak_catstring  | v_ptr (cast to char*)
 * adiak_jsonstring | v_ptr (cast to char*)
 * adiak_path       | v_ptr (cast to char*)
 * adiak_range      | v_subval (as adiak_value_t[2]) [*]
 * adiak_set        | v_subval (as adiak_value_t array) [*]
 * adiak_list       | v_subval (as adiak_value_t array) [*]
 * adiak_tuple      | v_subval (as adiak_value_t array) [*]
 * adiak_longlong   | v_longlong
 * adiak_ulonglong  | v_longlong (as unsigned long long)
 *
 * [*] Reference (zero-copy) container types store the original
 * input pointer in v_ptr.
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

static const adiak_datatype_t adiak_unset_datatype = { adiak_type_unset, adiak_numerical_unset, 0, 0, NULL, 0, 0, 0 };

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
 * from the string specifiers shown in \ref adiak_type_t. The varargs contain the value
 * and, if needed, any additional parameters for the type (e.g., list length).
 *
 * When scalar values other than (unsigned) \c int, \c long, or \c double are used in
 * compound types, use a length specifier to indicate the exact type in \a typestr,
 * e.g. "%f32" for 32-bit \c float or "%i16" for \c short.
 *
 * Examples:
 *
 * \code
 * adiak_namevalue("numrecords", adiak_general, NULL, "%d", 10);
 * adiak_namevalue("buildcompiler", adiak_general, "build data", "%v", "gcc@4.7.3");
 *
 * double gridvalues[] = { 5.4, 18.1, 24.0, 92.8 };
 * adiak_namevalue("gridvals", adiak_general, NULL, "[%f]", gridvalues, 4);
 *
 * unsigned char bytes[] = { 32, 33, 34, 35 };
 * adiak_namevalue("bytes", adiak_general, NULL, "{%u8}", bytes, 4);
 *
 * struct { int pos; const char *val; } letters[3] = { {1, "a"}, {2, "b"}, {3, "c"} };
 * adiak_namevalue("alphabet", adiak_general, NULL, "[(%d, %s)]", letters, 3, 2);
 * \endcode
 *
 * By default, Adiak makes deep copies of the provided names and values, so it is
 * safe to free the provided objects. Optionally, one can prefix the type specifier
 * with \a & to create a reference entry where Adiak does not copy the data
 * but only stores the provided pointer. In this case, the data objects must be
 * retained until program exit or until \ref adiak_clean is called.
 *
 * Only string or compound/container types (list, set, etc.) can be stored as
 * references. For compound types, the `&` can be applied to an inner type to create
 * a shallow copy, e.g. \a {&%s} for a shallow-copy list of zero-copy strings.
 *
 * Examples:
 *
 * \code
 * static const char* strings[2] = { "A string", "Another string" };
 *
 * // zero-copy string
 * adiak_namevalue("string", adiak_general, NULL, "&%s", strings[0]);
 * // deep-copy list of strings
 * adiak_namevalue("list_0", adiak_general, NULL, "{%s}", strings);
 * // zero-copy list of strings
 * adiak_namevalue("list_1", adiak_general, NULL, "&{%s}", strings);
 * const char* reverse[2] = { strings[1], strings[0] };
 * // shallow-copy list of zero-copy strings
 * adiak_namevalue("list_2", adiak_general, NULL, "{&%s}", reverse);
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
 *   \a typestr. For scalars and strings,
 *   this is typically just the value itself. Compound types like arrays or tuples
 *   require additional parameters like an array length. See
 *   \ref adiak_type_t to learn how to encode the value for each Adiak datatype.
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

/** \brief Makes a 'adiakversion' name/val with the Adiak library version */
int adiak_adiakversion();
/** \brief Makes a 'user' name/val with the real name of who's running the job */
int adiak_user();
/** \brief Makes a 'uid' name/val with the uid of who's running the job */
int adiak_uid();
/** \brief Makes a 'launchdate' name/val with the seconds since UNIX epoch of when this job started */
int adiak_launchdate();
/** \brief Makes a 'launchday' name/val with the date when this job started, but truncated to midnight
 *
 * Creates an adiak_date value named "launchday" of the time when this process was started,
 * in the form of seconds since UNIX epoch,
 * but rounded down to the previous midnight at GMT+0. This can be used to group jobs that
 * ran on the same day.
 */
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
/** \brief Makes a 'hostlist' name/val with the set of hostnames in this MPI job
 *
 * This function invokes MPI collective operations and must be called by all MPI
 * ranks in the communicator provided to \ref adiak_init.
 */
int adiak_hostlist();
/** \brief Makes a 'numhosts' name/val with the number of hosts in this MPI job
 *
 * This function invokes MPI collective operations and must be called by all MPI
 * ranks in the communicator provided to \ref adiak_init.
 */
int adiak_num_hosts();
/** \brief Makes a 'mpi_version' name/val with the MPI standard version */
int adiak_mpi_version();
/** \brief Makes a 'mpi_library' name/val with MPI library info
 *
 * Returns the information given by MPI_Get_library_version(). The output
 * is different for each MPI library implementation, and can be a long,
 * multi-line string.
 */
int adiak_mpi_library();
/** \brief Reports MPI library version and vendor information
 *
 * Makes mpi_library_version and mpi_library_vendor name/value pairs
 * with the MPI library version and vendor. It parses out the version
 * and vendor information from the string provided by
 * \a MPI_Get_library_version(). Currently this works for
 * CRAY MPICH, IBM Spectrum MPI, Open MPI, MVAPICH2, and MPICH.
 */
int adiak_mpi_library_version();
/** \brief Collect all available built-in Adiak name/value pairs
 *
 * This shortcut invokes all of the pre-defined routines that collect common
 * metadata (like \ref adiak_uid, \ref adiak_launchdate, etc).
 *
 * If MPI support is enabled then this function is a collective all and must
 * be called by all MPI ranks in the communicator provided to \ref adiak_init.
 *
 * \return -1 if *no* data could be collected, otherwise 0.
 */
int adiak_collect_all();

/** \brief Trigger a flush in registered tools. */
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
