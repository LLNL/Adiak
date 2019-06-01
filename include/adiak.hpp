#include <string>

#include "adiak.h"

namespace adiak
{
   /**
    * date/version/path/catpath are used to distinguise these types from strings and longs
    * when you pass values to adiak::value.
    **/
   //A date, represented as seconds since epcoh.
   struct date {
      unsigned long v;
      date(unsigned long sec_since_epoch) { v = sec_since_epoch; }
      operator unsigned long() const { return v; }
      typedef unsigned long adiak_underlying_type;
   };

   //A version number, as a string.
   struct version {
      std::string v;
      version(std::string verstring) { v = verstring; }
      operator std::string() const { return v; }
      typedef std::string adiak_underlying_type;
   };

   //A file path, as a string.
   struct path {
      std::string v;      
      path(std::string filepath) { v = filepath; }
      operator std::string() const { return v; }
      typedef std::string adiak_underlying_type;      
   };

   //A categorical string, which functions like a string but flags it as unsortable.
   struct catstring {
      std::string v;
      catstring(std::string cs) { v = cs; }
      operator std::string() const { return v; }
      typedef std::string adiak_underlying_type;
   };     
}

#include "adiak_internal.hpp"

namespace adiak
{
   /**
    * adiak::init should be called for MPI programs after MPI_Init has completed and 
    * be passed a valid communicator such as MPI_COMM_WORLD.  You should include mpi.h
    * before including adiak.hpp if running an MPI job.
    *
    * If you pass NULL as a communicator, adiak will behave like it was running in a 
    * single-process job.
    * 
    * For both MPI and non-MPI jobs call adiak::init before registering any name/value
    * pairs.
    **/
#if defined(MPI_VERSION)
   void init(MPI_Comm *communicator);
#else
   void init();
#endif

   /**
    * Call adiak::fini near the end of your job.  For MPI programs, do so before a call to
    * MPI_Finalize()
    **/
   void fini();

   /**
    * adiak::value() registers a name/value pair with Adiak.  Adiak will make a shallow
    * copy of the object and pass its name/value/type to any underlying subscribers
    * (which register via adiak_tool.h).  adiak::value returns true if it completes
    * successfully, or false on an error.
    * 
    * For the value, you can pass in:
    *  - single objects
    *  - An STL container (which will pass through as a set of objects)
    *  - A pair of objects, A and B, which represents a range [A, B)
    * 
    * For the underlying type, Adiak understands:
    *  - Integral objects (int, long, double and signed/unsigned)
    *  - Strings
    *  - UNIX time objects (struct timeval pointers)
    *  - Dates, as seconds since epcoh*
    *  - version numbers*
    *  - paths*
    *  * = These types shouldn't just be passed in as strings and longs, use the
    *      adiak:: date/version/path wrappers below so they're distinguishable.
    *  You can pass in other types of objects, or build a custom adiak_datatype_t,
    *  but the underlying subscribers may-or-may-not know how to interpret your 
    *  underlying object.
    * 
    * You can optionally also pass a category field, which can help subscribers
    * categorize data and decide what to respond to.
    *
    * Examples:
    * adiak::value("maxtempature", 70.2);
    *
    * adiak::value("compiler", adiak::version("gcc@8.1.0"));
    *
    * struct timeval starttime = ...;
    * struct timeval endtime = ...;
    * adiak::value("runtime", starttime, endtime, adiak_performance);
    *
    * set<string> names;
    * names.insert("matt");
    * names.insert("david");
    * names.insert("greg");
    * adiak::value("users", names);
    *
    * There is also a c-interface to register name/values, see the adiak.h file.
    **/
   template <typename T>
   bool value(std::string name, T value, adiak_category_t category = adiak_general) {
      adiak_datatype_t *datatype = adiak::internal::parse<T>::make_type();
      if (!datatype)
         return false;      
      adiak_value_t *avalue = (adiak_value_t *) malloc(sizeof(adiak_value_t));
      bool result = adiak::internal::parse<T>::make_value(value, avalue, datatype);
      if (!result)
         return false;
      return adiak_raw_namevalue(name.c_str(), category, avalue, datatype) == 0;
   }
   
   template <typename T>
   bool value(std::string name, T valuea, T valueb, adiak_category_t category = adiak_general) {
      //adiak_datatype_t datatype = adiak_unset_datatype;
      //return adiak::internal::handle_container(name, valuea, valueb, datatype);
      return true;
   }

#if defined(MPI_VERSION)
   inline void init(MPI_Comm *communicator) {
      adiak_init(communicator);
   }
#else
   inline void init() {
      adiak_init(NULL);
   }   
#endif

   inline void fini() {
      adiak_fini();
   }

   //Registers a name/value string of "user" with the real name of the person running this process
   // (e.g., "John Smith").
   inline bool user() {
      return adiak_user() == 0;
   }

   //Registers a name/value string of "uid" with the account name running this process
   // (e.g., "jsmith").
   inline bool uid() {
      return adiak_uid() == 0;
   }
   
   //Registers a name/value date of "date" with the starttime of this process.
   inline bool launchdate() {
      return adiak_launchdate() == 0;
   }

   //Registers a name/value string of "executable" with executable file (with path stripped) running this process.
   inline bool executable() {
      return adiak_executable() == 0;
   }

   //Registers a name/value path of "executablepath" with the full path to the executable file running this process.
   inline bool executablepath() {
      return adiak_executablepath() == 0;
   }

   //Registers a name/value set of paths of "libaries" with the full path to every shared library loaded in this
   // process.   
   inline bool libraries() {
      return adiak_libraries() == 0;
   }

   //Registers a name/value list of strings with the command line arguments running this job.   
   inline bool cmdline() {
      return adiak_cmdline() == 0;
   }

   //Registers a name/value string of "hostname" with the hostname running this job.      
   inline bool hostname() {
      return adiak_hostname() == 0;
   }

   //Registers a name/value string of "clustername" with the cluster name (hostname with numbers stripped)
   inline bool clustername() {
      return adiak_clustername() == 0;
   }

   //Registers a name/value timeval of "walltime" with the walltime this process ran for.  Requires adiak_fini()
   // to be called before MPI_finalize() or the end of a non-mpi job.   
   inline bool walltime() {
      return adiak_walltime() == 0;
   }

   //Registers a name/value timeval of "systime" with the time this process spent in system calls (typicall IO
   // and communication).  Requires adiak_fini() to be called before MPI_finalize() or the end of a non-mpi job.   
   inline bool systime() {
      return adiak_systime() == 0;
   }

   //Registers a name/value timeval of "cputime" with the time this process spent on the CPU.
   // Requires adiak_fini() to be called before MPI_finalize() or the end of a non-mpi job.   
   inline bool cputime() {
      return adiak_cputime() == 0;
   }   

   //Registers a name/value integer of "jobsize" with the number of ranks in this job.
   // Only works if adiak_init was called with a valid MPI communicator.   
   inline bool jobsize() {
      return adiak_job_size() == 0;
   }

   //Registers a name/value set of strings with the hostnames that are part of this job.
   // Only works if adiak_init was called with a valid MPI communicator.   
   inline bool hostlist() {
      return adiak_hostlist() == 0;
   }

   inline bool flush(std::string output) {
      return adiak_flush(output.c_str()) == 0;
   }

   inline bool clean() {
      return adiak_clean() == 0;
   }
}
