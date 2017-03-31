#ifndef UTILS_STRING_UTILS_H_
#define UTILS_STRING_UTILS_H_

#include <string>
#include <sstream>
#include <vector>

namespace utils {

class string_utils {
 public:
  static std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim))
      elems.push_back(std::move(item));
    return elems;
  }
};

}

#endif /* UTILS_STRING_UTILS_H_ */
