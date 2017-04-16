#ifndef UTILS_STRING_UTILS_H_
#define UTILS_STRING_UTILS_H_

#include <string>
#include <sstream>
#include <vector>

namespace utils {

class string_utils {
 public:
  static std::vector<std::string> split(const std::string &s, char delim,
                                        size_t count) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    size_t i = 0;
    while (std::getline(ss, item, delim) && i <= count) {
      elems.push_back(std::move(item));
      i++;
    }
    while (std::getline(ss, item, delim))
      elems.back() += item;
    return elems;
  }

  static std::vector<std::string> split(const std::string &s, char delim) {
    return split(s, delim, UINT64_MAX);
  }
};

}

#endif /* UTILS_STRING_UTILS_H_ */
