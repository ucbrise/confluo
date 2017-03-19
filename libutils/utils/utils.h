#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <cstdio>
#include <ctime>
#include <random>

namespace utils {

class time_utils {
 public:
  static std::string current_date_time() {
    std::time_t rawtime;
    std::tm* timeinfo;
    char buffer[80];

    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d.%X", timeinfo);
    return std::string(buffer);
  }
};

class rand_utils {
 public:

  static int64_t rand_int64(const int64_t& max) {
    return rand_int64(0, max);
  }

  static int64_t rand_int64(const int64_t& min, const int64_t& max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int64_t> distribution(min, max);
    return distribution(generator);
  }

  static uint64_t rand_uint64(const uint64_t& max) {
    return rand_uint64(0, max);
  }

  static uint64_t rand_uint64(const uint64_t& min, const uint64_t& max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<uint64_t> distribution(min, max);
    return distribution(generator);
  }

  static uint64_t rand_int32(const int32_t& max) {
    return rand_int32(0, max);
  }

  static uint64_t rand_int32(const int32_t& min, const int32_t& max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int32_t> distribution(min, max);
    return distribution(generator);
  }

  static uint64_t rand_uint32(const uint32_t& max) {
    return rand_uint32(0, max);
  }

  static uint64_t rand_uint32(const uint32_t& min, const uint32_t& max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<uint32_t> distribution(min, max);
    return distribution(generator);
  }
};

}

#endif /* UTILS_UTILS_H_ */
