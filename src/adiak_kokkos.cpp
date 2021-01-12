#include <adiak.hpp>
extern "C" void kokkosp_declare_metadata(const char* key, const char* value) {
  adiak::value(key, value);
}
