// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#if !defined(ADIAK_H_)
#define ADIAK_H_

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ADIAK_VERSION 0
#define ADIAK_MINOR_VERSION 2
#define ADIAK_POINT_VERSION 1

// ADIAK DATA CATEGORIES 
// Please treat values through 1000 as reserved. 1001 onward can be used as
// custom categories
#define adiak_category_unset 0
#define adiak_category_all 1
#define adiak_general 2     /* General information about runs */
#define adiak_performance 3 /* Information about performance */
#define adiak_control 4     /* Control and meta commands (e.g., flush) */

typedef enum {
   adiak_numerical_unset = 0,
   adiak_categorical, /* Means values are unique */
   adiak_ordinal,     /* Means the value has an ordering */
   adiak_interval,    /* Means the value has ranges (can be subtracted) */
   adiak_rational     /* Means the value is divisible */
} adiak_numerical_t;

typedef enum {
   adiak_type_unset = 0,
   adiak_long,        /* adiak_value string: %ld (implies adiak_rational) */
   adiak_ulong,       /* adiak_value string: %lu (implies rational) */   
   adiak_int,         /* adiak_value string: %d (implies rational) */
   adiak_uint,        /* adiak_value string: %u (implies rational) */
   adiak_double,      /* adiak_value string: %f (implies rational) */
   adiak_date,        /* adiak_value string: %D (implies interval), passed as a signed long in seconds since epoch */
   adiak_timeval,     /* adiak_value string: %t (implies interval), passed as a 'struct timeval *' */
   adiak_version,     /* adiak_value string: %v (implies oridinal), passed as a char*  */
   adiak_string,      /* adiak_value string: %s (implies oridinal), passed as a char*  */
   adiak_catstring,   /* adiak_value string: %r (implies categorical), passed as a char* */
   adiak_path,        /* adiak_value string: %p (implies categorical), passed as a char* */
   adiak_range,       /* adiak_value string <typestring> (implies categorical), */
   adiak_set,         /* adiak_value string [typestring], passed as a size integer */
   adiak_list,        /* adiak_value string {typestring}, passed as a size integer */
   adiak_tuple        /* adiak_value string (typestring1, typestring2, ..., typestringN), passed as an N integer */
} adiak_type_t;

typedef int adiak_category_t;

typedef struct adiak_datatype_t {
   adiak_type_t dtype;
   adiak_numerical_t numerical;
   int num_elements;
   int num_subtypes;
   struct adiak_datatype_t **subtype;
} adiak_datatype_t;

typedef union adiak_value_t {
   signed long v_long;
   int v_int;
   short v_short;
   char v_char;
   double v_double;
   void *v_ptr;
   union adiak_value_t *v_subval;
} adiak_value_t;
   
static const adiak_datatype_t adiak_unset_datatype = { adiak_type_unset, adiak_numerical_unset, 0, 0, NULL };

/**
 * Initializes the adiak interface.  When run in an MPI job, adiak takes a communicator pointer used for reducing
 * the data passed to it.  This should be called after MPI_Init.  When run without MPI adiak_init can be passed
 * NULL.
 **/
void adiak_init(void *mpi_communicator_p);

/**
 * adiak_fini should be called before program exit.  If using MPI, it should be called before
 * MPI_finalize.
 **/
void adiak_fini();
   
/**
 * adiak_namevalue registers a name/value pair.  The printf-style typestr describes the type of the 
 * value, which is constructed from the string specifiers above.  The varargs contains parameters
 * for the type.  The entire type describes how value is encoded.  For example:
 * 
 * adiak_namevalue("numrecords", adiak_general, "%d", 10);
 *
 * adiak_namevalue("buildcompiler", adiak_general, "%v", "gcc@4.7.3");
 *
 * double gridvalues[] = { 5.4, 18.1, 24.0, 92.8 };
 * adiak_namevalue("gridvals", adiak_general, "[%f]", gridvalues, 4);
 *
 * struct { int pos; const char *val; } letters[3] = { {1, 'a'}, {2, 'b'}, {3, 'c} }
 * adiak_namevalue("alphabet", adiak_general, "[(%u, %s)]", letters, 3, 2);
 **/
int adiak_namevalue(const char *name, int category, const char *subcategory, const char *typestr, ...);

/**
 * adiak_new_datatype construts a new adiak_datatype_t that can be
 * passed to adiak_raw_namevalue.  It is not safe to free this datatype.
 **/
adiak_datatype_t *adiak_new_datatype(const char *typestr, ...);

/**
 * Similar to adiak_namevalue, adiak_raw_namevalue registers a new name/value pair,
 * but with an already constructed datatype and value.
 **/
int adiak_raw_namevalue(const char *name, int category, const char *subcategory,
                        adiak_value_t *value, adiak_datatype_t *type);

int adiak_user();  /* Makes a 'user' name/val with the real name of who's running the job */
int adiak_uid(); /* Makes a 'uid' name/val with the uid of who's running the job */
int adiak_launchdate(); /* Makes a 'launchdate' name/val with the date of when this job started */
int adiak_launchday(); /* Makes a 'launchday' name/val with date when this job started, but truncated to midnight */
int adiak_executable(); /* Makes an 'executable' name/val with the executable file for this job */ 
int adiak_executablepath(); /* Makes an 'executablepath' name/value with the full executable file path. */
int adiak_workdir(); /* Makes a 'working_directory' name/val with the cwd for this job */
int adiak_libraries(); /* Makes a 'libraries' name/value with the set of shared library paths. */
int adiak_cmdline(); /* Makes a 'cmdline' name/val string set with the command line parameters */
int adiak_hostname(); /* Makes a 'hostname' name/val with the hostname */
int adiak_clustername(); /* Makes a 'cluster' name/val with the cluster name (hostname with numbers stripped) */
int adiak_walltime(); /* Makes a 'walltime' name/val with the walltime how long this job ran */
int adiak_systime(); /* Makes a 'systime' name/val with the timeval of how much time was spent in IO */
int adiak_cputime(); /* Makes a 'cputime' name/val with the timeval of how much time was spent on the CPU */

int adiak_job_size(); /* Makes a 'jobsize' name/val with the number of ranks in an MPI job */
int adiak_hostlist(); /* Makes a 'hostlist' name/val with the set of hostnames in this MPI job */
int adiak_num_hosts(); /* Makes a 'numhosts' name/val with the number of hosts in this MPI job */   

int adiak_flush(const char *location);
int adiak_clean();
   
adiak_numerical_t adiak_numerical_from_type(adiak_type_t dtype);

#if defined(__cplusplus)
}
#endif

#endif
