#ifndef DIALOG_CONSTANTS_H_
#define DIALOG_CONSTANTS_H_

#include <thread>

namespace confluo {

class defaults {
 public:
  static const int HARDWARE_CONCURRENCY;
  static constexpr double DEFAULT_INDEX_BUCKET_SIZE = 1.0;
  static constexpr uint64_t DEFAULT_TIME_RESOLUTION_NS = 1e6;
  static constexpr size_t DEFAULT_MAX_MEMORY = 1e9;
  static constexpr uint64_t DEFAULT_MONITOR_WINDOW_MS = 10;
  static constexpr uint64_t DEFAULT_MONITOR_PERIODICITY_MS = 1;
};

const int defaults::HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();
constexpr double defaults::DEFAULT_INDEX_BUCKET_SIZE;
constexpr size_t defaults::DEFAULT_MAX_MEMORY;
constexpr uint64_t defaults::DEFAULT_TIME_RESOLUTION_NS;
constexpr uint64_t defaults::DEFAULT_MONITOR_WINDOW_MS;
constexpr uint64_t defaults::DEFAULT_MONITOR_PERIODICITY_MS;

}

#endif /* DIALOG_CONSTANTS_H_ */
