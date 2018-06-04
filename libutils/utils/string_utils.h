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
  static std::vector<std::string> split(const std::string &s, char delim, size_t count);

  static std::vector<std::string> split(const std::string &s, char delim);

  static std::string mk_string(const std::vector<std::string> &v, const std::string &delim);

  template<typename functor>
  static std::string transform(const std::string &str, functor f) {
    std::string out;
    out.resize(str.length());
    std::transform(str.begin(), str.end(), out.begin(), f);
    return out;
  }

  static std::string to_upper(const std::string &str);

  static std::string to_lower(const std::string &str);

  template<typename T>
  static T lexical_cast(const std::string &s) {
    std::stringstream ss(s);

    T result;
    if ((ss >> result).fail() || !(ss >> std::ws).eof()) {
      throw std::bad_cast();
    }

    return result;
  }
};

template<>
bool string_utils::lexical_cast<bool>(const std::string &s);

}

#endif /* UTILS_STRING_UTILS_H_ */
