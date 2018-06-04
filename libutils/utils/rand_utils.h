#ifndef UTILS_RAND_UTILS_H_
#define UTILS_RAND_UTILS_H_

#include <cstdio>
#include <random>

namespace utils {

class rand_utils {
 public:

  static int64_t rand_int64(const int64_t &max);

  static int64_t rand_int64(const int64_t &min, const int64_t &max);

  static uint64_t rand_uint64(const uint64_t &max);

  static uint64_t rand_uint64(const uint64_t &min, const uint64_t &max);

  static int32_t rand_int32(const int32_t &max);

  static int32_t rand_int32(const int32_t &min, const int32_t &max);

  static uint64_t rand_uint32(const uint32_t &max);

  static uint64_t rand_uint32(const uint32_t &min, const uint32_t &max);
};

}

#endif /* UTILS_RAND_UTILS_H_ */
