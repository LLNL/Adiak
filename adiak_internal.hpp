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
   
   namespace internal {
      template<typename T> struct element_type        { static const adiak_type_t dtype = adiak_type_unset; };
      template<> struct element_type<long>            { static const adiak_type_t dtype = adiak_long; };
      template<> struct element_type<unsigned long>   { static const adiak_type_t dtype = adiak_ulong; };
      template<> struct element_type<int>             { static const adiak_type_t dtype = adiak_int; };      
      template<> struct element_type<unsigned int>    { static const adiak_type_t dtype = adiak_uint; };
      template<> struct element_type<short>           { static const adiak_type_t dtype = adiak_int; };      
      template<> struct element_type<unsigned short>  { static const adiak_type_t dtype = adiak_uint; };
      template<> struct element_type<char>            { static const adiak_type_t dtype = adiak_int; };      
      template<> struct element_type<unsigned char>   { static const adiak_type_t dtype = adiak_uint; };      
      template<> struct element_type<double>          { static const adiak_type_t dtype = adiak_double; };
      template<> struct element_type<float>           { static const adiak_type_t dtype = adiak_double; };      
      template<> struct element_type<const char *>    { static const adiak_type_t dtype = adiak_string; };
      template<> struct element_type<std::string>     { static const adiak_type_t dtype = adiak_string; };
      template<> struct element_type<struct timeval*> { static const adiak_type_t dtype = adiak_timeval; };
      template<> struct element_type<adiak::date>     { static const adiak_type_t dtype = adiak_date; };
      template<> struct element_type<adiak::version>  { static const adiak_type_t dtype = adiak_version; };
      template<> struct element_type<adiak::path>     { static const adiak_type_t dtype = adiak_path; };
      template<> struct element_type<adiak::catstring>{ static const adiak_type_t dtype = adiak_catstring; };
      template<typename U> struct element_type<std::set<U> >
      { static const adiak_type_t dtype = adiak_set; };
      template<typename U> struct element_type<std::unordered_set<U> >
      { static const adiak_type_t dtype = adiak_set; };
      template<typename U> struct element_type<std::multiset<U> >
      { static const adiak_type_t dtype = adiak_set; };
      template<typename U> struct element_type<std::vector<U> >
      { static const adiak_type_t dtype = adiak_list; };
      template<typename U> struct element_type<std::list<U> >
      { static const adiak_type_t dtype = adiak_list; };
      template<typename U, std::size_t N> struct element_type<std::array<U, N> >
      { static const adiak_type_t dtype = adiak_list; };
      template<typename U> struct element_type<std::deque<U> >
      { static const adiak_type_t dtype = adiak_list; };
      template<typename... U> struct element_type<std::tuple<U...> >
      { static const adiak_type_t dtype = adiak_tuple; };

      template<typename T> struct is_container { static const bool value = false; };
      template<typename U> struct is_container<std::set<U> >          { static const bool value = true; };
      template<typename U> struct is_container<std::unordered_set<U> >{ static const bool value = true; };
      template<typename U> struct is_container<std::multiset<U> >     { static const bool value = true; };
      template<typename U> struct is_container<std::vector<U> >       { static const bool value = true; };
      template<typename U> struct is_container<std::list<U> >         { static const bool value = true; };
      template<typename U, std::size_t N> struct is_container<std::array<U, N> > { static const bool value = true; };
      template<typename U> struct is_container<std::deque<U> >        { static const bool value = true; };


      template<typename T>
      class has_adiak_underlying_type {
         template<typename C> static char test(typename C::adiak_underlying_type);
         template<typename C> static int test(...);
      public:
         enum { value = sizeof(test<T>(0)) == sizeof(char) };
      };      

      template <typename T>
      struct make_type
      {
         static adiak_datatype_t *create() {            
            adiak_type_t dtype = element_type<T>::dtype;
            adiak_datatype_t *datatype = adiak_get_basetype(dtype);
            return datatype;      
         }
      };

      template <typename T>
      struct create_container_type
      {
         static adiak_datatype_t *create() {
            adiak_datatype_t *datatype = (adiak_datatype_t *) malloc(sizeof(adiak_datatype_t));
            memset(datatype, 0, sizeof(*datatype));
            datatype->dtype = element_type<T>::dtype;
            datatype->numerical = adiak_numerical_from_type(datatype->dtype);
            datatype->num_elements = 0;
            datatype->num_subtypes = 1;
            datatype->subtype = (adiak_datatype_t **) malloc(sizeof(adiak_datatype_t *));
            datatype->subtype[0] = make_type<typename T::value_type>::create();
            if (datatype->subtype[0] == NULL) {
               free(datatype);
               datatype = NULL;
            }
            return datatype;
         }               
      };
      
      template<typename T> struct make_type<std::set<T> > {
         static adiak_datatype_t *create() {
            return create_container_type<std::set<T> >::create();
         }
      };
      template<typename T> struct make_type<std::unordered_set<T> > {
         static adiak_datatype_t *create() {
            return create_container_type<std::unordered_set<T> >::create();
         }
      };
      template<typename T> struct make_type<std::multiset<T> > {
         static adiak_datatype_t *create() {
            return create_container_type<std::multiset<T> >::create();
         }
      };
      template<typename T> struct make_type<std::vector<T> > {
         static adiak_datatype_t *create() {
            return create_container_type<std::vector<T> >::create();
         }
      };
      template<typename T> struct make_type<std::list<T> > {
         static adiak_datatype_t *create() {
            return create_container_type<std::list<T> >::create();
         }
      };
      template<typename T, std::size_t N> struct make_type<std::array<T, N> > {
         static adiak_datatype_t *create() {
            return create_container_type<std::array<T, N> >::create();
         }
      };
      template<typename T> struct make_type<std::deque<T> > {
         static adiak_datatype_t *create() {
            return create_container_type<std::deque<T> >::create();
         }
      };
      
      template<>
      struct make_type<std::string> {
         static adiak_datatype_t *create() {
            adiak_type_t dtype = adiak_string;
            adiak_datatype_t *datatype = adiak_get_basetype(dtype);
            return datatype;
         }
      };

      template <typename T, std::size_t I>
      struct set_tuple_type {
         static void settype(adiak_datatype_t **dtypes) {
            dtypes[I-1] = make_type<typename std::tuple_element<I-1, T>::type>::create();
            set_tuple_type<T, I-1>::settype(dtypes);
         }
      };

      template <typename T>
      struct set_tuple_type<T, 0> {
         static void settype(adiak_datatype_t **dtypes) {
         }
      };


      template<typename... Ts>
      struct make_type<std::tuple<Ts...> > {
         static adiak_datatype_t *create() {
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
      };
      
      
      typedef char* str_ptr;
      typedef struct timeval *timeval_ptr;

      inline bool make_value(const unsigned long &c, adiak_value_t *value, adiak_datatype_t *) { value->v_long = c; return true; }
      inline bool make_value(const signed long &c, adiak_value_t *value, adiak_datatype_t *) { value->v_long = c; return true; }
      inline bool make_value(const signed int &c, adiak_value_t *value, adiak_datatype_t *) { value->v_int = c; return true; }
      inline bool make_value(const unsigned int &c, adiak_value_t *value, adiak_datatype_t *) { value->v_int = c; return true; }
      inline bool make_value(const double &c, adiak_value_t *value, adiak_datatype_t *) { value->v_double = c; return true; }
      inline bool make_value(const float &c, adiak_value_t *value, adiak_datatype_t *) { value->v_double = c; return true; }
      inline bool make_value(const std::string &c, adiak_value_t *value, adiak_datatype_t *) { value->v_ptr = strdup(c.c_str()); return true; }
      inline bool make_value(const str_ptr &c, adiak_value_t *value, adiak_datatype_t *) { value->v_ptr = strdup(c); return true; }

      inline bool make_value(const timeval_ptr &c, adiak_value_t *value, adiak_datatype_t *) {
         struct timeval *cpy = (struct timeval *) malloc(sizeof(struct timeval));
         *cpy = *c;
         value->v_ptr = cpy;
         return true;
      }
      
      template <typename T, typename... Ts> bool make_value(const std::tuple<Ts...> &c, adiak_value_t *value, adiak_datatype_t *);
      template <typename T> typename std::enable_if<has_adiak_underlying_type<T>::value, bool>::type make_value(const T &c, adiak_value_t *value, adiak_datatype_t *);
      template <typename T> typename std::enable_if<is_container<T>::value, bool>::type make_value(const T &c, adiak_value_t *value, adiak_datatype_t *);
      
      template <typename T>
      inline typename std::enable_if<has_adiak_underlying_type<T>::value, bool>::type
      make_value(const T &c, adiak_value_t *value, adiak_datatype_t * dtype) {
         return make_value(c.v, value, dtype);
      }

      template <std::size_t I = 0, typename... Ts>
      typename std::enable_if<I == sizeof...(Ts), bool>::type
      set_tuple_value(const std::tuple<Ts...> &c, adiak_value_t *value, adiak_datatype_t *) {
         return true;
      }
      
      template <std::size_t I = 0, typename... Ts>
      typename std::enable_if<I < sizeof...(Ts), bool>::type
      set_tuple_value(const std::tuple<Ts...> &c, adiak_value_t *values, adiak_datatype_t *dtype) {
         bool result = make_value(std::get<I>(c), values+I, dtype->subtype[I]);
         if (!result)
            return false;
         set_tuple_value<I + 1, Ts...>(c, values, dtype);
         return true;
      }
      
      template <typename... Ts>
      bool make_value(const std::tuple<Ts...> &c, adiak_value_t *value, adiak_datatype_t *dtype) {
         size_t n = std::tuple_size<std::tuple<Ts...> >::value;         
         adiak_value_t *valarray = (adiak_value_t *) malloc(sizeof(adiak_value_t) * n);
         bool result = set_tuple_value(c, valarray, dtype);
         if (!result)
            return false;
         value->v_ptr = valarray;
         return true;
      }

      template <typename T>
      typename std::enable_if<is_container<T>::value, bool>::type
      make_value(const T &c, adiak_value_t *value, adiak_datatype_t *dtype) {
         adiak_value_t *valarray = (adiak_value_t *) malloc(sizeof(adiak_value_t) * c.size());
         int j = 0;
         for (typename T::const_iterator i = c.begin(); i != c.end(); i++) {
            bool result = make_value(*i, valarray + j++, dtype->subtype[0]);
            if (!result)
               return false;
         }         
         value->v_ptr = valarray;
         dtype->num_elements = c.size();
         return true;
      }
   }
}

#endif
