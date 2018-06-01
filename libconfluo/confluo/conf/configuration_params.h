#ifndef CONFLUO_CONF_CONFIGURATION_PARAMS_H_
#define CONFLUO_CONF_CONFIGURATION_PARAMS_H_

#include "configuration_parser.h"
#include "config_utils.h"
#include "defaults.h"

namespace confluo {

class conf {
 public:
  static utils::configuration_map &instance() {
    static std::string
        paths = utils::config_utils::read_from_env("CONFLUO_CONF", "/etc/conf/confluo.conf:./conf/confluo.conf");
    static utils::configuration_map confluo_conf(paths);
    return confluo_conf;
  }
};

/**
 * Confluo archival configuration parameters
 */
class archival_configuration_params {
 public:
  static uint64_t PERIODICITY_MS() {
    return conf::instance().get<uint64_t>("archival_periodicity_ms", archival_defaults::DEFAULT_PERIODICITY_MS());
  }

  static uint64_t IN_MEMORY_DATALOG_WINDOW_BYTES() {
    return conf::instance().get<uint64_t>("archival_in_memory_datalog_window_bytes",
                                          archival_defaults::DEFAULT_IN_MEMORY_DATALOG_WINDOW_BYTES());
  }

  static uint64_t IN_MEMORY_FILTER_WINDOW_NS() {
    return conf::instance().get<uint64_t>("archival_in_memory_filter_window_ns",
                                          archival_defaults::DEFAULT_IN_MEMORY_FILTER_WINDOW_NS());
  }

  // Maximum archival file size in bytes. Cannot be smaller than a data log bucket.
  static size_t MAX_FILE_SIZE() {
    return conf::instance().get<size_t>("max_archival_file_size", archival_defaults::DEFAULT_MAX_FILE_SIZE());
  }

  // Archival compression parameters; TODO parse types
  static uint8_t DATA_LOG_ENCODING_TYPE() {
    return configuration_parser::to_encoding_type(
        conf::instance().get<std::string>("data_log_archival_encoding",
                                          archival_defaults::DEFAULT_DATA_LOG_ENCODING_TYPE()));
  }
  static uint8_t REFLOG_ENCODING_TYPE() {
    return configuration_parser::to_encoding_type(
        conf::instance().get<std::string>("reflog_archival_encoding",
                                          archival_defaults::DEFAULT_REFLOG_ENCODING_TYPE()));
  }
};

/**
 * Confluo Configuration parameters
 */
class configuration_params {
 public:
  /** Memory configuration parameters */
  static size_t MAX_MEMORY() {
    return conf::instance().get<size_t>("max_memory", defaults::DEFAULT_MAX_MEMORY());
  }

  /** Thread configuration parameters */
  static int MAX_CONCURRENCY() {
    return conf::instance().get<int>("max_concurrency", defaults::HARDWARE_CONCURRENCY());
  }

  /** Index configuration parameters */
  static double INDEX_BUCKET_SIZE() {
    return conf::instance().get<double>("index_block_size", defaults::DEFAULT_INDEX_BUCKET_SIZE());
  }

  /** Time resolution */
  static uint64_t TIME_RESOLUTION_NS() {
    return conf::instance().get<uint64_t>("time_resolution_ns", defaults::DEFAULT_TIME_RESOLUTION_NS());
  }

  /** Monitor configuration parameters */
  static uint64_t MONITOR_WINDOW_MS() {
    return conf::instance().get<uint64_t>("monitor_window_ms", defaults::DEFAULT_MONITOR_WINDOW_MS());
  }

  /** Monitor periodicity in milliseconds */
  static uint64_t MONITOR_PERIODICITY_MS() {
    return conf::instance().get<uint64_t>("monitor_periodicity_ms", defaults::DEFAULT_MONITOR_PERIODICITY_MS());
  }
};

}

#endif /* CONFLUO_CONF_CONFIGURATION_PARAMS_H_ */
