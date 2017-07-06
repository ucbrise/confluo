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

  uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed) {
    uint32_t h = seed;
    if (len > 3) {
      const uint32_t* key_x4 = (const uint32_t*) key;
      size_t i = len >> 2;
      do {
        uint32_t k = *key_x4++;
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        h ^= k;
        h = (h << 13) | (h >> 19);
        h = (h * 5) + 0xe6546b64;
      } while (--i);
      key = (const uint8_t*) key_x4;
    }
    if (len & 3) {
      size_t i = len & 3;
      uint32_t k = 0;
      key = &key[i - 1];
      do {
        k <<= 8;
        k |= *key--;
      } while (--i);
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
    }
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
  }
};

}

#endif /* UTILS_MATH_UTILS_H_ */
