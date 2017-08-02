#ifndef DIALOG_CONSTANTS_H_
#define DIALOG_CONSTANTS_H_

#include <thread>

namespace dialog {

class constants {
 public:
  static const int HARDWARE_CONCURRENCY;
  static constexpr double DEFAULT_INDEX_BUCKET_SIZE = 1.0;
  static constexpr uint64_t DEFAULT_MONITOR_WINDOW_MS = 10;
  static constexpr uint64_t DEFAULT_MONITOR_PERIODICITY_MS = 1;
};

const int constants::HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();
const double constants::DEFAULT_INDEX_BUCKET_SIZE;
const uint64_t constants::DEFAULT_MONITOR_WINDOW_MS;
const uint64_t constants::DEFAULT_MONITOR_PERIODICITY_MS;

}

#endif /* DIALOG_CONSTANTS_H_ */
