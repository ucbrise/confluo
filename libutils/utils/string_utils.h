#ifndef UTILS_STRING_UTILS_H_
#define UTILS_STRING_UTILS_H_

#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <typeinfo>

namespace utils {

class string_utils {
 public:
  inline static std::vector<std::string> split(const std::string &s, char delim,
                                        size_t count) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    size_t i = 0;
    while (std::getline(ss, item, delim) && i < count) {
      elems.push_back(std::move(item));
      i++;
    }
    while (std::getline(ss, item, delim))
      elems.back() += item;
    return elems;
  }

  inline static std::vector<std::string> split(const std::string &s, char delim) {
    return split(s, delim, UINT64_MAX);
  }

  template<typename functor>
  inline static std::string transform(const std::string& str, functor f) {
    std::string out;
    out.resize(str.length());
    std::transform(str.begin(), str.end(), out.begin(), f);
    return out;
  }

  inline static std::string to_upper(const std::string& str) {
    return transform(str, ::toupper);
  }

  inline static std::string to_lower(const std::string& str) {
    return transform(str, ::tolower);
  }

  template<typename T>
  inline static T lexical_cast(const std::string& s) {
    std::stringstream ss(s);

    T result;
    if ((ss >> result).fail() || !(ss >> std::ws).eof()) {
      throw std::bad_cast();
    }

    return result;
  }
};

template<>
inline bool string_utils::lexical_cast<bool>(const std::string& s) {
  std::stringstream ss(s);

  bool result;
  if ((ss >> std::boolalpha >> result).fail() || !(ss >> std::ws).eof()) {
    throw std::bad_cast();
  }

  return result;
}

}

#endif /* UTILS_STRING_UTILS_H_ */
