#include <string>
#include "time_utils.h"

using namespace ::std::chrono;

namespace utils {

std::string time_utils::current_date_time() {
  std::time_t rawtime;
  std::tm* timeinfo;
  char buffer[100];

  std::time(&rawtime);
  timeinfo = std::localtime(&rawtime);
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %X", timeinfo);
  return std::string(buffer);
}
uint64_t time_utils::cur_ns() {
  time_point<system_clock> now = system_clock::now();
  return static_cast<uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());
}
uint64_t time_utils::cur_us() {
  time_point<system_clock> now = system_clock::now();
  return static_cast<uint64_t>(duration_cast<microseconds>(now.time_since_epoch()).count());
}
uint64_t time_utils::cur_ms() {
  time_point<system_clock> now = system_clock::now();
  return static_cast<uint64_t>(duration_cast<milliseconds>(now.time_since_epoch()).count());
}
uint64_t time_utils::cur_s() {
  time_point<system_clock> now = system_clock::now();
  return static_cast<uint64_t>(duration_cast<seconds>(now.time_since_epoch()).count());
}
}