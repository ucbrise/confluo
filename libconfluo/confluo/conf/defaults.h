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
  static inline uint64_t DEFAULT_PERIODICITY_MS() {
    return static_cast<const uint64_t>(5 * 60 * 1e3);
  }

  static inline size_t DEFAULT_MAX_FILE_SIZE() {
    return 1024 * 1024 * 1024;
  }

  static inline std::string DEFAULT_DATA_LOG_ENCODING_TYPE() {
    return encoding_params::LZ4();
  }

  static inline std::string DEFAULT_REFLOG_ENCODING_TYPE() {
    return encoding_params::ELIAS_GAMMA();
  }

  // TODO % of physical memory
  static inline uint64_t DEFAULT_IN_MEMORY_DATALOG_WINDOW_BYTES() {
    return static_cast<const uint64_t>(1e10);
  }

  static inline uint64_t DEFAULT_IN_MEMORY_FILTER_WINDOW_NS() {
    return static_cast<const uint64_t>(10 * 1e3);
  }
};

/**
 * Default configuration parameters
 */
class defaults {
 public:
  /** The thread hardware concurrency */
  static inline int HARDWARE_CONCURRENCY() {
    return std::thread::hardware_concurrency();
  }

  /** Default bucket size for index */
  static inline double DEFAULT_INDEX_BUCKET_SIZE() {
    return 1.0;
  }

  /** Default time resolution in nanoseconds */
  static inline uint64_t DEFAULT_TIME_RESOLUTION_NS() {
    return static_cast<const uint64_t>(1e6);
  }

  /** Default maximum amount of memory */
  static inline size_t DEFAULT_MAX_MEMORY() {
    return static_cast<const size_t>(1e9);
  }

  /** Default memory monitor periodicity in milliseconds */
  static inline uint64_t DEFAULT_MEMORY_MONITOR_PERIODICITY_MS() {
    return 1;
  }

  /** Default monitor window in milliseoncds */
  static inline uint64_t DEFAULT_MONITOR_WINDOW_MS() {
    return 10;
  }

  /** Default periodicity for monitor in milliseconds */
  static inline uint64_t DEFAULT_MONITOR_PERIODICITY_MS() {
    return 1;
  }
};

}

#endif /* CONFLUO_CONF_DEFAULTS_H_ */
