#include <type_traits>

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
      template<> struct element_type<double>          { static const adiak_type_t dtype = adiak_double; };
      template<> struct element_type<const char *>    { static const adiak_type_t dtype = adiak_string; };
      template<> struct element_type<std::string>     { static const adiak_type_t dtype = adiak_string; };
      template<> struct element_type<struct timeval*> { static const adiak_type_t dtype = adiak_timeval; };
      template<> struct element_type<adiak::date>     { static const adiak_type_t dtype = adiak_date; };
      template<> struct element_type<adiak::version>  { static const adiak_type_t dtype = adiak_version; };
      template<> struct element_type<adiak::path>     { static const adiak_type_t dtype = adiak_path; };
      template<> struct element_type<adiak::catstring>{ static const adiak_type_t dtype = adiak_catstring; };

      template<typename T>
      class has_adiak_underlying_type {
         template<typename C> static char test(typename C::adiak_underlying_type);
         template<typename C> static int test(...);
      public:
         enum { value = sizeof(test<T>(0)) == sizeof(char) };
      };      

      template <typename T>
      struct c_type_helper {
         typedef T C;
         static const T obj(const T &u) { return u; }
      };

      template<>
      struct c_type_helper<std::string> {
         typedef const char* C;
         static const char *obj(const std::string &s) { return s.c_str(); }         
      };

      template <typename T, typename enable = void>
      struct c_type;

      template <typename T>
      struct c_type<T, typename std::enable_if<has_adiak_underlying_type<T>::value>::type>
      {
         typedef typename c_type_helper<typename T::adiak_underlying_type>::C ctype;
         static ctype obj(const T &t) { return c_type_helper<typename T::adiak_underlying_type>::obj(t.v); }
      };

      template <typename T>
      struct c_type<T, typename std::enable_if<!has_adiak_underlying_type<T>::value>::type>
      {
         typedef typename c_type_helper<T>::C ctype;
         static ctype obj(const T &t) { return c_type_helper<T>::obj(t); }
         //static const ctype obj(const T &t) { return c_type_helper<T>::obj(t); }         
      };

      template<typename T>
      class has_value_type {
         template<typename C> static char test(typename C::value_type);
         template<typename C> static int test(...);
      public:
         enum { value = sizeof(test<T>(0)) == sizeof(char) };
      };

      template <typename T>
      typename std::enable_if<has_value_type<T>::value, bool>::type
      handle_container(std::string name, const T &c, adiak_datatype_t &datatype) {
         typedef typename c_type<typename T::value_type>::ctype etype;
         if (datatype.dtype == adiak_type_unset)
            datatype.dtype = element_type<typename T::value_type>::dtype;
         if (datatype.numerical == adiak_numerical_unset)
            datatype.numerical = adiak_numerical_from_type(datatype.dtype);
         datatype.grouping = adiak_set;
         if (c.empty())
            return adiak_rawval(name.c_str(), NULL, sizeof(etype), 0, datatype);
         etype *rawarray = new etype[c.size()];
         size_t j = 0;
         for (typename T::const_iterator i = c.begin(); i != c.end(); i++) {
            rawarray[j++] = c_type<typename T::value_type>::obj(*i);
         }
         int ret = adiak_rawval(name.c_str(), rawarray, sizeof(etype), c.size(), datatype);
         delete[] rawarray;
         return (ret == 0);
      }

      template <typename T>
      bool handle_container(std::string name, const T &a, const T &b, adiak_datatype_t &datatype) {
         typedef typename c_type<T>::ctype etype;
         if (datatype.dtype == adiak_type_unset)
            datatype.dtype = element_type<T>::dtype;
         if (datatype.numerical == adiak_numerical_unset)
            datatype.numerical = adiak_numerical_from_type(datatype.dtype);
         datatype.grouping = adiak_range;
         etype elempair[2];

         elempair[0] = c_type<T>::obj(a);
         elempair[1] = c_type<T>::obj(b);
         
         return adiak_rawval(name.c_str(), elempair, sizeof(etype), 2, datatype);
      }

      template<typename T>
      typename std::enable_if<!has_value_type<T>::value, bool>::type      
      handle_container(std::string name, T element, adiak_datatype_t &datatype) {
         typedef typename c_type<T>::ctype etype;
         if (datatype.dtype == adiak_type_unset)
            datatype.dtype = element_type<T>::dtype;            
         if (datatype.numerical == adiak_numerical_unset)
            datatype.numerical = adiak_numerical_from_type(datatype.dtype);
         datatype.grouping = adiak_point;
         etype elementcopy = c_type<T>::obj(element);
         int result = adiak_rawval(name.c_str(), &elementcopy, sizeof(etype), 1, datatype);
         return (result == 0);
      }
   }
}
