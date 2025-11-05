#ifndef ADIAK_INTERFACE_PYTHON_TYPES_HPP_
#define ADIAK_INTERFACE_PYTHON_TYPES_HPP_

#include "common.hpp"
#include <pybind11/pybind11.h>

#include <chrono>
#include <type_traits>

namespace adiak {
  inline bool operator<(const date& a, const date& b) {
      return a.v < b.v;
  }
  inline bool operator<(const version& a, const version& b) {
      return a.v < b.v;
  }
  inline bool operator<(const path& a, const path& b) {
      return a.v < b.v;
  }
  inline bool operator<(const catstring& a, const catstring& b) {
      return a.v < b.v;
  }
  inline bool operator<(const jsonstring& a, const jsonstring& b) {
      return a.v < b.v;
  }
namespace python {

// The 'is_templated_base_of' type trait is derived from the following
// StackOverflow post:
// https://stackoverflow.com/a/49163138
template <template <typename...> class Base, typename Derived,
          typename TCheck = void>
struct is_templated_base_of_tester;

template <template <typename...> class Base, typename Derived>
struct is_templated_base_of_tester<
    Base, Derived, pybind11::detail::enable_if_t<std::is_class<Derived>::value>> : Derived {
  template <typename... T> static constexpr std::true_type test(Base<T...> *);
  static constexpr std::false_type test(...);
  using is_base = decltype(test((is_templated_base_of_tester *)nullptr));
};

template <template <typename...> class Base, typename Derived>
struct is_templated_base_of_tester<
    Base, Derived, pybind11::detail::enable_if_t<!std::is_class<Derived>::value>> {
  using is_base = std::false_type;
};

template <template <typename...> class Base, typename Derived>
using is_templated_base_of =
    typename is_templated_base_of_tester<Base, Derived>::is_base;

template <typename T, class adiak_type> struct DataContainer {
  T m_v;

  DataContainer(T val) : m_v(val) {}
  virtual ~DataContainer() {}

  virtual T to_python() const { return m_v; }

  virtual adiak_type to_adiak() const = 0;
};

// Python-compatible struct for storing timepoint info
// that will be converted into 'struct timeval' for Adiak
struct Timepoint : public DataContainer<std::chrono::system_clock::time_point,
                                        struct timeval *> {
  mutable struct timeval m_time_for_adiak;

  Timepoint(std::chrono::system_clock::time_point time);

  struct timeval *to_adiak() const final;
};

// Python-compatible struct for storing date or datetime info
// that will be converted into 'adiak::date' for Adiak
struct Date
    : public DataContainer<std::chrono::system_clock::time_point, adiak::date> {
  Date(std::chrono::system_clock::time_point time);

  adiak::date to_adiak() const final;
};

// Python-compatible struct for storing version info
// that will be converted into 'adiak::version' for Adiak
struct Version : public DataContainer<std::string, adiak::version> {
  Version(const std::string &ver);

  adiak::version to_adiak() const final;
};

// TODO replace use of string with std::filesystem if/when C++17 is supported
// Python-compatible struct for storing path info
// that will be converted into 'adiak::path' for Adiak
struct Path : public DataContainer<std::string, adiak::path> {
  Path(const std::string &p);

  adiak::path to_adiak() const final;
};

// Python-compatible struct for storing categorical string info
// that will be converted into 'adiak::catstring' for Adiak
struct CatStr : public DataContainer<std::string, adiak::catstring> {
  CatStr(const std::string &cs);

  adiak::catstring to_adiak() const final;
};

// Python-compatible struct for storing JSON string info
// that will be converted into 'adiak::jsonstring' for Adiak
struct JsonStr : public DataContainer<std::string, adiak::jsonstring> {
  JsonStr(const std::string &js);

  adiak::jsonstring to_adiak() const final;
};

inline bool operator<(const Timepoint& a, const Timepoint& b) {
    if (a.m_time_for_adiak.tv_sec != b.m_time_for_adiak.tv_sec)
        return a.m_time_for_adiak.tv_sec < b.m_time_for_adiak.tv_sec;
    return a.m_time_for_adiak.tv_usec < b.m_time_for_adiak.tv_usec;
}
inline bool operator<(const Date& a, const Date& b) {
    return a.to_adiak().v < b.to_adiak().v;
}
inline bool operator<(const Version& a, const Version& b) {
    return a.to_adiak().v < b.to_adiak().v;
}
inline bool operator<(const Path& a, const Path& b) {
    return a.to_adiak().v < b.to_adiak().v;
}
inline bool operator<(const CatStr& a, const CatStr& b) {
    return a.to_adiak().v < b.to_adiak().v;
}
inline bool operator<(const JsonStr& a, const JsonStr& b) {
    return a.to_adiak().v < b.to_adiak().v;
}

void create_adiak_types_mod(py::module_ &mod);

} // namespace python
} // namespace adiak

#endif /* ADIAK_INTERFACE_PYTHON_TYPES_HPP_ */