#ifndef CONFLUO_CONF_DEFAULTS_H_
#define CONFLUO_CONF_DEFAULTS_H_

#include <thread>
#include "storage/ptr_metadata.h"

namespace confluo {

/**
 * Default archival configuration parameters
 */
class archival_defaults {
 public:
  static const size_t DEFAULT_PERIODIC_ARCHIVAL_WINDOW = 1024 * 1024;
  static const uint64_t DEFAULT_PERIODICITY_MS = 5 * 60 * 1e3;
  static const size_t DEFAULT_MAX_FILE_SIZE = 1024 * 1024 * 64;
  static const uint8_t DEFAULT_DATA_LOG_ENCODING_TYPE = storage::encoding_type::D_UNENCODED;
  static const uint8_t DEFAULT_REFLOG_ENCODING_TYPE = storage::encoding_type::D_UNENCODED;
  static const uint64_t DEFAULT_RECENCY_RESTRICTION_WINDOW = 1e6;
  static const uint64_t DEFAULT_RECENCY_FILTER_RESTRICTION_WINDOW_NS = 10 * 1e3;
};

const size_t archival_defaults::DEFAULT_MAX_FILE_SIZE;
const uint64_t archival_defaults::DEFAULT_PERIODICITY_MS;
const uint8_t archival_defaults::DEFAULT_DATA_LOG_ENCODING_TYPE;
const uint8_t archival_defaults::DEFAULT_REFLOG_ENCODING_TYPE;
const uint64_t archival_defaults::DEFAULT_RECENCY_RESTRICTION_WINDOW;
const uint64_t archival_defaults::DEFAULT_RECENCY_FILTER_RESTRICTION_WINDOW_NS;
const size_t archival_defaults::DEFAULT_PERIODIC_ARCHIVAL_WINDOW;

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
  static constexpr uint64_t DEFAULT_MEMORY_MONITOR_PERIODICITY_MS = 1;
  static constexpr uint64_t DEFAULT_MONITOR_WINDOW_MS = 10;
  /** Default periodicity for monitor in milliseconds */
  static constexpr uint64_t DEFAULT_MONITOR_PERIODICITY_MS = 1;
};

const int defaults::HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();
constexpr double defaults::DEFAULT_INDEX_BUCKET_SIZE;
constexpr size_t defaults::DEFAULT_MAX_MEMORY;
constexpr uint64_t defaults::DEFAULT_TIME_RESOLUTION_NS;
constexpr uint64_t defaults::DEFAULT_MEMORY_MONITOR_PERIODICITY_MS;
constexpr uint64_t defaults::DEFAULT_MONITOR_WINDOW_MS;
constexpr uint64_t defaults::DEFAULT_MONITOR_PERIODICITY_MS;

}

#endif /* CONFLUO_CONF_DEFAULTS_H_ */
