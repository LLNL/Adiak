// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <type_traits>
#include <set>
#include <vector>
#include <unordered_set>
#include <list>
#include <array>
#include <deque>

#include <iostream>
#include <cassert>

#include <cstring>
#include <cstdlib>

#if !defined(_ADIAK_INTERNAL_H_)
#define _ADIAK_INTERNAL_H_

extern "C" {
   adiak_datatype_t *adiak_get_basetype(adiak_type_t t);
}

namespace adiak
{
   struct date;
   struct version;
   struct path;
   struct catstring;
   struct jsonstring;

   namespace internal {
      template<typename T> struct element_type {
         static const adiak_type_t dtype = adiak_type_unset;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };
      template<> struct element_type<long> {
         static const adiak_type_t dtype = adiak_long;
         static void set(adiak_value_t &v, long p) { v.v_long = p; }
      };
      template<> struct element_type<unsigned long> {
         static const adiak_type_t dtype = adiak_ulong;
         static void set(adiak_value_t &v, unsigned long p) { v.v_long = p; }
      };
      template<> struct element_type<long long> {
         static const adiak_type_t dtype = adiak_longlong;
         static void set(adiak_value_t &v, long long p) { v.v_longlong = p; }
      };
      template<> struct element_type<unsigned long long> {
         static const adiak_type_t dtype = adiak_ulonglong;
         static void set(adiak_value_t &v, unsigned long long p) { v.v_longlong = p; }
      };
      template<> struct element_type<int> {
         static const adiak_type_t dtype = adiak_int;
         static void set(adiak_value_t &v, int p) { v.v_int = p; }
      };
      template<> struct element_type<unsigned int> {
         static const adiak_type_t dtype = adiak_uint;
         static void set(adiak_value_t &v, unsigned int p) { v.v_int = p; }
      };
      template<> struct element_type<short> {
         static const adiak_type_t dtype = adiak_int;
         static void set(adiak_value_t &v, short p) { v.v_int = p; }
      };
      template<> struct element_type<unsigned short> {
         static const adiak_type_t dtype = adiak_uint;
         static void set(adiak_value_t &v, unsigned short p) { v.v_int = p; }
      };
      template<> struct element_type<char> {
         static const adiak_type_t dtype = adiak_int;
         static void set(adiak_value_t &v, char p) { v.v_int = p; }
      };
      template<> struct element_type<unsigned char> {
         static const adiak_type_t dtype = adiak_uint;
         static void set(adiak_value_t &v, unsigned char p) { v.v_int = p; }
      };
      template<> struct element_type<double> {
         static const adiak_type_t dtype = adiak_double;
         static void set(adiak_value_t &v, double p) { v.v_double = p; }
      };
      template<> struct element_type<float> {
         static const adiak_type_t dtype = adiak_double;
         static void set(adiak_value_t &v, float p) { v.v_double = p; }
      };
      template<> struct element_type<const char *> {
         static const adiak_type_t dtype = adiak_string;
         static void set(adiak_value_t &v, const char * p) { v.v_ptr = strdup(p); }
      };
      template<> struct element_type<std::string> {
         static const adiak_type_t dtype = adiak_string;
         static void set(adiak_value_t &v, const std::string &p) { v.v_ptr = strdup(p.c_str()); }
      };
      template<> struct element_type<struct timeval*> {
         static const adiak_type_t dtype = adiak_timeval;
         static void set(adiak_value_t &v, struct timeval* p) {
            struct timeval *cpy = (struct timeval *) malloc(sizeof(struct timeval));
            *cpy = *p;
            v.v_ptr = cpy;
         }
      };
      template<> struct element_type<adiak::date> {
         static const adiak_type_t dtype = adiak_date;
         static void set(adiak_value_t &v, adiak::date p) { v.v_long = p; }
      };
      template<> struct element_type<adiak::version> {
         static const adiak_type_t dtype = adiak_version;
         static void set(adiak_value_t &v, adiak::version p) { v.v_ptr = strdup(p.v.c_str()); }
      };
      template<> struct element_type<adiak::path> {
         static const adiak_type_t dtype = adiak_path;
         static void set(adiak_value_t &v, adiak::path p) { v.v_ptr = strdup(p.v.c_str()); }
      };
      template<> struct element_type<adiak::catstring> {
         static const adiak_type_t dtype = adiak_catstring;
         static void set(adiak_value_t &v, adiak::catstring p) { v.v_ptr = strdup(p.v.c_str()); }
      };
      template<> struct element_type<adiak::jsonstring> {
         static const adiak_type_t dtype = adiak_jsonstring;
         static void set(adiak_value_t &v, adiak::jsonstring p) { v.v_ptr = strdup(p.v.c_str()); }
      };
      template<typename U> struct element_type<std::set<U> > {
         static const adiak_type_t dtype = adiak_set;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };
      template<typename U> struct element_type<std::unordered_set<U> > {
         static const adiak_type_t dtype = adiak_set;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };
      template<typename U> struct element_type<std::multiset<U> > {
         static const adiak_type_t dtype = adiak_set;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };
      template<typename U> struct element_type<std::vector<U> > {
         static const adiak_type_t dtype = adiak_list;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };
      template<typename U> struct element_type<std::list<U> > {
         static const adiak_type_t dtype = adiak_list;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };
      template<typename U, std::size_t N> struct element_type<std::array<U, N> > {
         static const adiak_type_t dtype = adiak_list;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };
      template<typename U> struct element_type<std::deque<U> > {
         static const adiak_type_t dtype = adiak_list;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };
      template<typename... U> struct element_type<std::tuple<U...> > {
         static const adiak_type_t dtype = adiak_tuple;
         static void set(adiak_value_t &v, void *p) { v.v_ptr = p; }
      };

      template <typename T>
      struct parse
      {
         static adiak_datatype_t *make_type() {
            adiak_type_t dtype = element_type<T>::dtype;
            adiak_datatype_t *datatype = adiak_get_basetype(dtype);
            return datatype;
         }
         static bool make_value(T &obj, adiak_value_t *val, adiak_datatype_t *) {
            element_type<T>::set(*val, obj);
            return true;
         }
         static bool make_value(const T &obj, adiak_value_t *val, adiak_datatype_t *) {
            element_type<T>::set(*val, obj);
            return true;
         }
      };

      template <typename T>
      static adiak_datatype_t *make_range_t() {
         adiak_datatype_t *datatype = (adiak_datatype_t *) malloc(sizeof(adiak_datatype_t));
         memset(datatype, 0, sizeof(*datatype));
         datatype->dtype = adiak_range;
         datatype->num_elements = 2;
         datatype->num_subtypes = 1;
         datatype->subtype = (adiak_datatype_t **) malloc(sizeof(adiak_datatype_t *));
         datatype->subtype[0] = parse<T>::make_type();
         if (datatype->subtype[0] == NULL) {
            free(datatype);
            datatype = NULL;
         }
         return datatype;
      }

      template <typename T>
      struct create_container_type
      {
         static adiak_datatype_t *make_type() {
            adiak_datatype_t *datatype = (adiak_datatype_t *) malloc(sizeof(adiak_datatype_t));
            memset(datatype, 0, sizeof(*datatype));
            datatype->dtype = element_type<T>::dtype;
            datatype->numerical = adiak_numerical_from_type(datatype->dtype);
            datatype->num_elements = 0;
            datatype->num_subtypes = 1;
            datatype->subtype = (adiak_datatype_t **) malloc(sizeof(adiak_datatype_t *));
            datatype->subtype[0] = parse<typename T::value_type>::make_type();
            if (datatype->subtype[0] == NULL) {
               free(datatype);
               datatype = NULL;
            }
            return datatype;
         }

         static bool make_value(T &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            adiak_value_t *valarray = (adiak_value_t *) malloc(sizeof(adiak_value_t) * c.size());
            int j = 0;
            for (typename T::iterator i = c.begin(); i != c.end(); i++) {
               bool result = parse<typename T::value_type>::make_value(*i, valarray + j++, dtype->subtype[0]);
               if (!result)
                  return false;
            }
            element_type<T>::set(*value, valarray);
            dtype->num_elements = c.size();
            return true;
         }
      };

      template<typename T> struct parse<std::set<T> > {
         static adiak_datatype_t *make_type() {
            return create_container_type<std::set<T> >::make_type();
         }
         static bool make_value(std::set<T> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            return create_container_type<std::set<T> >::make_value(c, value, dtype);
         }
      };
      template<typename T> struct parse<std::unordered_set<T> > {
         static adiak_datatype_t *make_type() {
            return create_container_type<std::unordered_set<T> >::make_type();
         }
         static bool make_value(std::unordered_set<T> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            return create_container_type<std::unordered_set<T> >::make_value(c, value, dtype);
         }
      };
      template<typename T> struct parse<std::multiset<T> > {
         static adiak_datatype_t *make_type() {
            return create_container_type<std::multiset<T> >::make_type();
         }
         static bool make_value(std::multiset<T> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            return create_container_type<std::multiset<T> >::make_value(c, value, dtype);
         }
      };
      template<typename T> struct parse<std::vector<T> > {
         static adiak_datatype_t *make_type() {
            return create_container_type<std::vector<T> >::make_type();
         }
         static bool make_value(std::vector<T> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            return create_container_type<std::vector<T> >::make_value(c, value, dtype);
         }
      };
      template<typename T> struct parse<std::list<T> > {
         static adiak_datatype_t *make_type() {
            return create_container_type<std::list<T> >::make_type();
         }
         static bool make_value(std::list<T> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            return create_container_type<std::list<T> >::make_value(c, value, dtype);
         }
      };
      template<typename T, std::size_t N> struct parse<std::array<T, N> > {
         static adiak_datatype_t *make_type() {
            return create_container_type<std::array<T, N> >::make_type();
         }
         static bool make_value(std::array<T, N> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            return create_container_type<std::array<T, N> >::make_value(c, value, dtype);
         }
      };
      template<typename T> struct parse<std::deque<T> > {
         static adiak_datatype_t *make_type() {
            return create_container_type<std::deque<T> >::make_type();
         }
         static bool make_value(std::deque<T> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            return create_container_type<std::deque<T> >::make_value(c, value, dtype);
         }
      };

      template <typename T, std::size_t I>
      struct set_tuple_type {
         static void settype(adiak_datatype_t **dtypes) {
            dtypes[I-1] = parse<typename std::tuple_element<I-1, T>::type>::make_type();
            set_tuple_type<T, I-1>::settype(dtypes);
         }
         static bool setvalue(T &c, adiak_value_t *values, adiak_datatype_t *dtype) {
            bool result = set_tuple_type<T, I-1>::setvalue(c, values, dtype);
            if (!result)
               return false;
            return parse<typename std::tuple_element<I-1, T>::type>::make_value(std::get<I-1>(c), values+I-1, dtype->subtype[I-1]);
         }
      };

      template <typename T>
      struct set_tuple_type<T, 0> {
         static void settype(adiak_datatype_t **/*dtypes*/) {
         }
         static bool setvalue(T &/*c*/, adiak_value_t */*values*/, adiak_datatype_t */*dtype*/) {
            return true;
         }
      };


      template<typename... Ts>
      struct parse<std::tuple<Ts...> > {
         static adiak_datatype_t *make_type() {
            adiak_datatype_t *datatype = (adiak_datatype_t *) malloc(sizeof(adiak_datatype_t));
            const size_t N = std::tuple_size<std::tuple<Ts...> >::value;
            memset(datatype, 0, sizeof(*datatype));
            datatype->dtype = adiak_tuple;
            datatype->numerical = adiak_numerical_from_type(datatype->dtype);
            datatype->num_elements = N;
            datatype->num_subtypes = N;
            datatype->subtype = (adiak_datatype_t **) malloc(sizeof(adiak_datatype_t *) * N);
            set_tuple_type<std::tuple<Ts...>, N>::settype(datatype->subtype);
            return datatype;
         }
         static bool make_value(std::tuple<Ts...> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
            const size_t N = std::tuple_size<std::tuple<Ts...> >::value;
            adiak_value_t *valarray = (adiak_value_t *) malloc(sizeof(adiak_value_t) * N);
            bool result = set_tuple_type<std::tuple<Ts...>, N>::setvalue(c, valarray, dtype);
            if (!result)
               return false;
            value->v_ptr = valarray;
            return true;
         }
      };
   }
}

#endif
