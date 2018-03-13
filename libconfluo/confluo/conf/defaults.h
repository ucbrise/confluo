#ifndef CONFLUO_CONF_DEFAULTS_H_
#define CONFLUO_CONF_DEFAULTS_H_

#include <thread>

namespace confluo {

/**
 * Default configuration parameters
 */
class defaults {
 public:
  /** The thread hardware concurrency */
  static const int HARDWARE_CONCURRENCY;
  /** Default bucket size for index */
  static constexpr double DEFAULT_INDEX_BUCKET_SIZE = 1.0;
  /** Default time resolution in nanoseconds */
  static constexpr uint64_t DEFAULT_TIME_RESOLUTION_NS = 1e6;
  /** Default maximum amount of memory */
  static constexpr size_t DEFAULT_MAX_MEMORY = 1e9;
  /** Default monitor window in milliseoncds */
  static constexpr uint64_t DEFAULT_MONITOR_WINDOW_MS = 10;
  /** Default periodicity for monitor in milliseconds */
  static constexpr uint64_t DEFAULT_MONITOR_PERIODICITY_MS = 1;
};

const int defaults::HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();
constexpr double defaults::DEFAULT_INDEX_BUCKET_SIZE;
constexpr size_t defaults::DEFAULT_MAX_MEMORY;
constexpr uint64_t defaults::DEFAULT_TIME_RESOLUTION_NS;
constexpr uint64_t defaults::DEFAULT_MONITOR_WINDOW_MS;
constexpr uint64_t defaults::DEFAULT_MONITOR_PERIODICITY_MS;

}

#endif /* CONFLUO_CONF_DEFAULTS_H_ */
