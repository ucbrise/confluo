#ifndef UTILS_MATH_UTILS_H_
#define UTILS_MATH_UTILS_H_

#include <cstdint>

namespace utils {

class math_utils {
 public:
  static uint64_t pow(uint64_t base, uint64_t exp) {
    uint64_t result = 1;
    while (exp) {
      if (exp & 1)
        result *= base;
      exp >>= 1;
      base *= base;
    }
    return result;
  }
};

}

#endif /* UTILS_MATH_UTILS_H_ */
