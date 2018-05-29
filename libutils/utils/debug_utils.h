#ifndef UTILS_DEBUG_UTILS_H_
#define UTILS_DEBUG_UTILS_H_

#include <type_traits>
#include <typeinfo>
#ifndef _MSC_VER
#   include <cxxabi.h>
#endif
#include <memory>
#include <string>
#include <cstdlib>

namespace utils {

class debug_utils {
 public:
  template<class T>
  static std::string type_name() {
    typedef typename std::remove_reference<T>::type TR;
    std::unique_ptr<char, void (*)(void *)> own(
        abi::__cxa_demangle(typeid(TR).name(), nullptr, nullptr, nullptr),
        std::free);
    std::string r = own != nullptr ? own.get() : typeid(TR).name();
    if (std::is_const<TR>::value)
      r += " const";
    if (std::is_volatile<TR>::value)
      r += " volatile";
    if (std::is_lvalue_reference<T>::value)
      r += "&";
    else if (std::is_rvalue_reference<T>::value)
      r += "&&";
    return r;
  }
};

}

#endif /* UTILS_DEBUG_UTILS_H_ */
