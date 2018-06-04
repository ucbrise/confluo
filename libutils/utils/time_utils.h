#ifndef UTILS_TIME_UTILS_H_
#define UTILS_TIME_UTILS_H_

#include <cstdio>
#include <ctime>
#include <chrono>

namespace utils {

class time_utils {
 public:
  static std::string current_date_time();

  static uint64_t cur_ns();

  static uint64_t cur_us();

  static uint64_t cur_ms();

  static uint64_t cur_s();
};

}

#endif /* LIBUTILS_UTILS_TIME_UTILS_H_ */
