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
  static const uint64_t DEFAULT_PERIODICITY_MS = 5 * 60 * 1e3;
  static const size_t DEFAULT_MAX_FILE_SIZE = 1024 * 1024 * 1024;
  static const std::string DEFAULT_DATA_LOG_ENCODING_TYPE;
  static const std::string DEFAULT_REFLOG_ENCODING_TYPE;
  static const uint64_t DEFAULT_IN_MEMORY_DATALOG_WINDOW_BYTES = 1e10; // TODO % of physical memory
  static const uint64_t DEFAULT_IN_MEMORY_FILTER_WINDOW_NS = 10 * 1e3;
};

const size_t archival_defaults::DEFAULT_MAX_FILE_SIZE;
const uint64_t archival_defaults::DEFAULT_PERIODICITY_MS;
const std::string archival_defaults::DEFAULT_DATA_LOG_ENCODING_TYPE = encoding_params::LZ4;
const std::string archival_defaults::DEFAULT_REFLOG_ENCODING_TYPE = encoding_params::ELIAS_GAMMA;
const uint64_t archival_defaults::DEFAULT_IN_MEMORY_DATALOG_WINDOW_BYTES;
const uint64_t archival_defaults::DEFAULT_IN_MEMORY_FILTER_WINDOW_NS;

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
