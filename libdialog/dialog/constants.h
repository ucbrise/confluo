#ifndef DIALOG_CONSTANTS_H_
#define DIALOG_CONSTANTS_H_

#include <thread>

namespace dialog {

class constants {
 public:
  static const size_t DEFAULT_MAX_ARCHIVAL_FILE_SIZE = 1024 * 1024 * 64;
  static const int HARDWARE_CONCURRENCY;
  static constexpr size_t DEFAULT_MAX_MEMORY = 1e9;
  static constexpr double DEFAULT_INDEX_BUCKET_SIZE = 1.0;
  static constexpr uint64_t DEFAULT_TIME_RESOLUTION_NS = 1e6;
  static constexpr uint64_t DEFAULT_MONITOR_WINDOW_MS = 10;
  static constexpr uint64_t DEFAULT_MONITOR_PERIODICITY_MS = 1;
};

const size_t constants::DEFAULT_MAX_ARCHIVAL_FILE_SIZE;
const int constants::HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();
constexpr size_t constants::DEFAULT_MAX_MEMORY;
constexpr double constants::DEFAULT_INDEX_BUCKET_SIZE;
constexpr uint64_t constants::DEFAULT_TIME_RESOLUTION_NS;
constexpr uint64_t constants::DEFAULT_MONITOR_WINDOW_MS;
constexpr uint64_t constants::DEFAULT_MONITOR_PERIODICITY_MS;

}

#endif /* DIALOG_CONSTANTS_H_ */
