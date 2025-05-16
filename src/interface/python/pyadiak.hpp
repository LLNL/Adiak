#ifndef ADIAK_INTERFACE_PYTHON_PYADIAK_HPP_
#define ADIAK_INTERFACE_PYTHON_PYADIAK_HPP_

#include "common.hpp"

#include <chrono>
#include <type_traits>

namespace adiak {
namespace python {

// The 'is_templated_base_of' type trait is derived from the following
// StackOverflow post:
// https://stackoverflow.com/a/49163138
template <template <typename...> class Base, typename Derived,
          typename TCheck = void>
struct is_templated_base_of_tester;

template <template <typename...> class Base, typename Derived>
struct is_templated_base_of_tester<
    Base, Derived, std::enable_if_t<std::is_class<Derived>::value>> : Derived {
  template <typename... T> static constexpr std::true_type test(Base<T...> *);
  static constexpr std::false_type test(...);
  using is_base = decltype(test((is_templated_base_of_tester *)nullptr));
};

template <template <typename...> class Base, typename Derived>
struct is_templated_base_of_tester<
    Base, Derived, std::enable_if_t<!std::is_class<Derived>::value>> {
  using is_base = std::false_type;
};

template <template <typename...> class Base, typename Derived>
using is_templated_base_of =
    typename is_templated_base_of_tester<Base, Derived>::is_base;

template <typename T, class adiak_type> struct DataContainer {
  T m_v;

  DataContainer(T val) : m_v(val) {}

  virtual T to_python() const { return m_v; }

  virtual adiak_type to_adiak() const = 0;
};

// Python-compatible struct for storing timepoint info
// that will be converted into 'struct timeval' for Adiak
struct Timepoint : public DataContainer<std::chrono::system_clock::time_point,
                                        struct timeval *> {
  struct timeval m_time_for_adiak;

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

void create_adiak_annotation_mod(py::module_ &mod);

} // namespace python
} // namespace adiak

#endif /* ADIAK_INTERFACE_PYTHON_PYADIAK_HPP_ */
