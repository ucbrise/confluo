#ifndef UTILS_TIME_UTILS_H_
#define UTILS_TIME_UTILS_H_

#include <cstdio>
#include <ctime>
#include <chrono>

using namespace ::std::chrono;

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

  static uint64_t cur_ns() {
    time_point<system_clock> now = system_clock::now();
    return duration_cast<nanoseconds>(now.time_since_epoch()).count();
  }

  static uint64_t cur_us() {
    time_point<system_clock> now = system_clock::now();
    return duration_cast<microseconds>(now.time_since_epoch()).count();
  }

  static uint64_t cur_ms() {
    time_point<system_clock> now = system_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch()).count();
  }

  static uint64_t cur_s() {
    time_point<system_clock> now = system_clock::now();
    return duration_cast<seconds>(now.time_since_epoch()).count();
  }

  template<typename F, typename ...Args>
  static uint64_t time_function_ns(F&& f, Args&&... args) {
    time_point<system_clock> start = system_clock::now();
    f(std::forward<Args>(args)...);
    time_point<system_clock> end = system_clock::now();
    return duration_cast<nanoseconds>(end - start).count();
  }
};

}

#endif /* LIBUTILS_UTILS_TIME_UTILS_H_ */
