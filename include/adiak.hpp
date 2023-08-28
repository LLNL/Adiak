// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

/// \file adiak.hpp
/// \brief Adiak C++ API

#ifndef ADIAK_HPP_
#define ADIAK_HPP_

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

/// \brief Adiak C++ API
namespace adiak
{
   /// \addtogroup UserAPI
   /// \{
   /// \name C++ user API
   /// \{

   /**
    * \brief Initialize Adiak
    *
    * adiak::init should be called for MPI programs after MPI_Init has completed and
    * be passed a valid communicator such as MPI_COMM_WORLD.  You should include mpi.h
    * before including adiak.hpp if running an MPI job.
    *
    * If you pass NULL as a communicator, adiak will behave like it was running in a
    * single-process job.
    *
    * For both MPI and non-MPI jobs call adiak::init before registering any name/value
    * pairs.
    */
   inline void init(void *mpi_communicator_p) {
      adiak_init(mpi_communicator_p);
   }

   /**
    * \brief Call adiak::fini near the end of your job. For MPI programs, do so before a call to
    * MPI_Finalize()
    */
   inline void fini() {
      adiak_fini();
   }

   /**
    * \brief Register a name/value pair with Adiak.
    *
    * Adiak will make a shallow copy of the object and pass its name/value/type to
    * any underlying subscribers (which register via adiak_tool.h).
    * adiak::value returns true if it completes successfully, or false on an error.
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
    *
    * Special types like paths and versions should be passed in as strings and longs
    * using the adiak::date/version/path wrappers below so they're distinguishable.
    *
    * You can pass in other types of objects, or build a custom adiak_datatype_t,
    * but the underlying subscribers may-or-may-not know how to interpret your
    * underlying object.
    *
    * You can optionally also pass a category field, which can help subscribers
    * categorize data and decide what to respond to.
    *
    * Examples:
    *
    * \code
    * adiak::value("maxtempature", 70.2);
    *
    * adiak::value("compiler", adiak::version("gcc@8.1.0"));
    *
    * struct timeval starttime = ...;
    * struct timeval endtime = ...;
    * adiak::value("runtime", starttime, endtime, adiak_performance);
    *
    * std::set<std::string> names;
    * names.insert("matt");
    * names.insert("david");
    * names.insert("greg");
    * adiak::value("users", names);
    * \endcode
    *
    * \param name The name of the Adiak name/value
    * \param value The value of the Adiak name/value
    * \param category An Adiak category like adiak_general or adiak_performance. If in doubt,
    *    use adiak_general.
    * \param subcategory An optional user-defined subcategory
    *
    * \sa adiak_namevalue
    */
   template <typename T>
   bool value(std::string name, T value, int category = adiak_general, std::string subcategory = "") {
      adiak_datatype_t *datatype = adiak::internal::parse<T>::make_type();
      if (!datatype)
         return false;
      adiak_value_t *avalue = (adiak_value_t *) malloc(sizeof(adiak_value_t));
      bool result = adiak::internal::parse<T>::make_value(value, avalue, datatype);
      if (!result)
         return false;
      return adiak_raw_namevalue(name.c_str(), category, subcategory.c_str(), avalue, datatype) == 0;
   }

   template <typename T>
   bool value(std::string name, T valuea, T valueb,
              int category = adiak_general, std::string subcategory = "")
   {
      adiak_datatype_t *datatype = adiak::internal::make_range_t<T>();
      if (!datatype)
         return false;
      adiak_value_t *values = (adiak_value_t *) malloc(sizeof(adiak_value_t) * 2);
      bool result;
      result = adiak::internal::parse<T>::make_value(valuea, values, datatype->subtype[0]);
      result |= adiak::internal::parse<T>::make_value(valueb, values+1, datatype->subtype[0]);
      if (!result) {
         return false;
      }
      result = adiak_raw_namevalue(name.c_str(), category, subcategory.c_str(), values, datatype);
      if (result != 0) {
         return false;
      }
      return true;
   }

   /// \copydoc adiak_user
   inline bool user() {
      return adiak_user() == 0;
   }

   /// \copydoc adiak_uid
   inline bool uid() {
      return adiak_uid() == 0;
   }

   /// \copydoc adiak_launchdate
   inline bool launchdate() {
      return adiak_launchdate() == 0;
   }

   /// \copydoc adiak_launchday
   inline bool launchday() {
      return adiak_launchday() == 0;
   }

   /// \copydoc adiak_executable
   inline bool executable() {
      return adiak_executable() == 0;
   }

   /// \copydoc adiak_executablepath
   inline bool executablepath() {
      return adiak_executablepath() == 0;
   }

   /// \copydoc adiak_workdir
   inline bool workdir() {
     return adiak_workdir() == 0;
   }

   /// \copydoc adiak_libraries
   inline bool libraries() {
      return adiak_libraries() == 0;
   }

   /// \copydoc adiak_cmdline
   inline bool cmdline() {
      return adiak_cmdline() == 0;
   }

   /// \copydoc adiak_hostname
   inline bool hostname() {
      return adiak_hostname() == 0;
   }

   /// \copydoc adiak_clustername
   inline bool clustername() {
      return adiak_clustername() == 0;
   }

   /// \copydoc adiak_walltime
   inline bool walltime() {
      return adiak_walltime() == 0;
   }

   /// \copydoc adiak_systime
   inline bool systime() {
      return adiak_systime() == 0;
   }

   /// \copydoc adiak_cputime
   inline bool cputime() {
      return adiak_cputime() == 0;
   }

   /// \copydoc adiak_job_size
   inline bool jobsize() {
      return adiak_job_size() == 0;
   }

   /// \copydoc adiak_hostlist
   inline bool hostlist() {
      return adiak_hostlist() == 0;
   }

   /// \copydoc adiak_num_hosts
   inline bool numhosts() {
      return adiak_num_hosts() == 0;
   }

   /// \copydoc adiak_flush
   inline bool flush(std::string output) {
      return adiak_flush(output.c_str()) == 0;
   }

   /// \copydoc adiak_clean
   inline bool clean() {
      return adiak_clean() == 0;
   }

   /// \}
   /// \}
}

#endif
