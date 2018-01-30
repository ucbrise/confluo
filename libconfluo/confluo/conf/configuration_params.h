#ifndef CONFLUO_CONF_CONFIGURATION_PARAMS_H_
#define CONFLUO_CONF_CONFIGURATION_PARAMS_H_

#include "conf/defaults.h"
#include "config_utils.h"

namespace confluo {

/**
 * Parameters used for the configuration of Confluo
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
  static uint64_t MONITOR_PERIODICITY_MS;

};

static utils::configuration_map confluo_conf(
    utils::config_utils::read_from_env(
        "CONFLUO_CONF", "/etc/conf/confluo.conf:./conf/confluo.conf"));

size_t configuration_params::MAX_MEMORY = confluo_conf.get<size_t>(
    "max_memory", defaults::DEFAULT_MAX_MEMORY);
int configuration_params::MAX_CONCURRENCY = confluo_conf.get<int>(
    "max_concurrency", defaults::HARDWARE_CONCURRENCY);
double configuration_params::INDEX_BUCKET_SIZE = confluo_conf.get<double>(
    "index_block_size", defaults::DEFAULT_INDEX_BUCKET_SIZE);
uint64_t configuration_params::TIME_RESOLUTION_NS = confluo_conf.get<uint64_t>(
    "time_resolution_ns", defaults::DEFAULT_TIME_RESOLUTION_NS);
uint64_t configuration_params::MONITOR_WINDOW_MS = confluo_conf.get<uint64_t>(
    "monitor_window_ms", defaults::DEFAULT_MONITOR_WINDOW_MS);
uint64_t configuration_params::MONITOR_PERIODICITY_MS = confluo_conf
    .get<uint64_t>("monitor_periodicity_ms",
                   defaults::DEFAULT_MONITOR_PERIODICITY_MS);

}

#endif /* CONFLUO_CONF_CONFIGURATION_PARAMS_H_ */
