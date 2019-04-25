#if !defined(ADIAK_H_)
#define ADIAK_H_

#if defined(USE_MPI)
#include <mpi.h>
#endif

#include <unistd.h>

#define ADIAK_VERSION 1

typedef enum {
   adiak_numerical_unset = 0,
   adiak_categorical, /* Means values are unique */
   adiak_ordinal,     /* Means the value has an ordering */
   adiak_interval,    /* Means the value has ranges (can be subtracted) */
   adiak_rational     /* Means the value is divisble */
} adiak_numerical_t;

typedef enum {
   adiak_grouping_unset = 0,
   adiak_point,       /* A single value */
   adiak_range,       /* All values between two specified values */
   adiak_set,         /* A set of values */
} adiak_grouping_t;

typedef enum {
   adiak_type_unset = 0,
   adiak_long,        /* adiak_value string: %ld (implies adiak_rational) */
   adiak_ulong,       /* adiak_value string: %lu (implies rational) */   
   adiak_int,         /* adiak_value string: %d (implies rational) */
   adiak_uint,        /* adiak_value string: %u (implies rational) */
   adiak_double,      /* adiak_value string: %f (implies rational) */
   adiak_date,        /* adiak_value string: %D (implies interval), passed as a unsigned long in seconds since epoch */
   adiak_timeval,     /* adiak_value string: %t (implies interval), passed as a 'struct timeval *' */
   adiak_version,     /* adiak_value string: %v (implies oridinal), passed as a char*  */
   adiak_string,      /* adiak_value string: %s (implies oridinal), passed as a char*  */
   adiak_catstring,   /* adiak_value string: %r (implies categorical), passed as a char* */
   adiak_path         /* adiak_value string: %p (implies categorical), passed as a char* */
} adiak_type_t;

typedef enum {
   adiak_category_unset = 0,
   adiak_category_all,
   adiak_general,    /* General information about runs */
   adiak_performance /* Information about performance */
} adiak_category_t;

typedef struct {
   adiak_numerical_t numerical;
   adiak_grouping_t grouping;
   adiak_type_t dtype;
   adiak_category_t category;
} adiak_datatype_t;

/**
 * Initializes the adiak interface.  When run in an MPI job, adiak takes a communicator used for reducing
 * the data passed to it.  This should be called after MPI_Init.
 **/
#if defined(MPI_VERSION)
void adiak_init(MPI_Comm *communicator);
#else
void adiak_init(void *unused);
#endif

/**
 * adiak_value registers a name/value pair.  The printf-style typestr describes the type of the 
 * value, which is constructed from the string specifiers above.  The varargs contain the
 * the actual values.  For example:
 *   adiak_value("gridvalues", "{%f}", 4, 5.4, 18.1, 24.0, 92.8);
 *   adiak_value("gridvalues", "[%f]", 4, gridarray);
 *   adiak_value("build_compiler", "%s", "intel@17.0.1");
 *   adiak_value("problemsize", "%lu", 14000);
 *   adiak_value("checkpointtime", "-%t-", cpoint_start, cpoint_end);
 *   adiak_value("maxheat", "%f", mheat);
 **/
int adiak_value(const char *name, adiak_category_t category, const char *typestr, ...);

int adiak_rawval(const char *name, void *val, size_t val_size, adiak_datatype_t valtype);

int adiak_user();  /* Makes a 'user' name/val with the real name of who's running the job */
int adiak_uid(); /* Makes a 'uid' name/val with the uid of who's running the job */
int adiak_launchdate(); /* Makes a 'date' name/val with the date of when this job started */
int adiak_executable(); /* Makes an 'executable' name/val with the executable file for this job */ 
int adiak_cmdline(); /* Makes a 'cmdline' name/val string set with the command line parameters */
int adiak_hostname(); /* Makes a 'hostname' name/val with the hostname */
int adiak_clustername(); /* Makes a 'cluster' name/val with the cluster name (hostname with numbers stripped) */
int adiak_runtime(); /* Makes a 'runtime' name/val with the walltime how long this job ran */
int adiak_iotime(); /* Makes a 'iotime' name/val with the timeval of how much time was spent in IO */


#if defined(MPI_VERSION)
int adiak_mpi_ranks(); /* Makes a 'mpiranks' name/val with the number of ranks in an MPI job */
int adiak_hostlist(); /* Makes a 'hostlist' name/val with the set of hostnames in this MPI job */
int adiak_mpitime(); /* Makes an 'mpitime' name/val with the timeval of how much time was spent in MPI */
#endif
                     
#endif
