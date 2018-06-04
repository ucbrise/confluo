#ifndef UTILS_MATH_UTILS_H_
#define UTILS_MATH_UTILS_H_

#include <cstdint>
#include <cstddef>

namespace utils {

class math_utils {
 public:
  static uint64_t pow(uint64_t base, uint64_t exp);

  static uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed);
};

}

#endif /* UTILS_MATH_UTILS_H_ */
