#ifndef CONFLUO_CONF_CONFIGURATION_PARAMS_H_
#define CONFLUO_CONF_CONFIGURATION_PARAMS_H_

#include "configuration_parser.h"
#include "config_utils.h"
#include "defaults.h"

namespace confluo {

/**
 * Confluo archival configuration parameters
 */
class archival_configuration_params {
 public:
  static uint64_t PERIODICITY_MS;
  static uint64_t IN_MEMORY_DATALOG_WINDOW_BYTES;
  static uint64_t IN_MEMORY_FILTER_WINDOW_NS;

  // Maximum archival file size in bytes. Cannot be smaller than a data log bucket.
  static size_t MAX_FILE_SIZE;

  // Archival compression parameters; TODO parse types
  static uint8_t DATA_LOG_ENCODING_TYPE;
  static uint8_t REFLOG_ENCODING_TYPE;
};

/**
 * Confluo Configuration parameters
 */
class configuration_params {
 public:
  /** Memory configuration parameters */
  static size_t MAX_MEMORY;

  /** Thread configuration parameters */
  static int MAX_CONCURRENCY;

  /** Index configuration parameters */
  static double INDEX_BUCKET_SIZE;

  /** Time resolution */
  static uint64_t TIME_RESOLUTION_NS;

  /** Monitor configuration parameters */
  static uint64_t MONITOR_WINDOW_MS;
  /** Monitor periodicity in milliseconds */
  static uint64_t MONITOR_PERIODICITY_MS;
};

extern utils::configuration_map confluo_conf;

}

#endif /* CONFLUO_CONF_CONFIGURATION_PARAMS_H_ */
