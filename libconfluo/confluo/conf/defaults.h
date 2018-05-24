#ifndef CONFLUO_CONF_DEFAULTS_H_
#define CONFLUO_CONF_DEFAULTS_H_

#include <thread>
#include "configuration_parser.h"
#include "storage/ptr_aux_block.h"

namespace confluo {

/**
 * Default archival configuration parameters
 */
class archival_defaults {
 public:
  static const uint64_t DEFAULT_PERIODICITY_MS = static_cast<const uint64_t>(5 * 60 * 1e3);
  static const size_t DEFAULT_MAX_FILE_SIZE = 1024 * 1024 * 1024;
  static const std::string DEFAULT_DATA_LOG_ENCODING_TYPE;
  static const std::string DEFAULT_REFLOG_ENCODING_TYPE;
  // TODO % of physical memory
  static const uint64_t DEFAULT_IN_MEMORY_DATALOG_WINDOW_BYTES = static_cast<const uint64_t>(1e10);
  static const uint64_t DEFAULT_IN_MEMORY_FILTER_WINDOW_NS = static_cast<const uint64_t>(10 * 1e3);
};


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
  static constexpr uint64_t DEFAULT_TIME_RESOLUTION_NS = static_cast<const uint64_t>(1e6);
  /** Default maximum amount of memory */
  static constexpr size_t DEFAULT_MAX_MEMORY = static_cast<const size_t>(1e9);
  /** Default memory monitor periodicity in milliseconds */
  static constexpr uint64_t DEFAULT_MEMORY_MONITOR_PERIODICITY_MS = 1;
  /** Default monitor window in milliseoncds */
  static constexpr uint64_t DEFAULT_MONITOR_WINDOW_MS = 10;
  /** Default periodicity for monitor in milliseconds */
  static constexpr uint64_t DEFAULT_MONITOR_PERIODICITY_MS = 1;
};

}

#endif /* CONFLUO_CONF_DEFAULTS_H_ */
