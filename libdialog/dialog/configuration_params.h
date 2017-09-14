#ifndef DIALOG_CONFIGURATION_PARAMS_H_
#define DIALOG_CONFIGURATION_PARAMS_H_

#include "constants.h"
#include "config_utils.h"

namespace dialog {

class configuration_params {
 public:
  // Thread configuration parameters
  static int MAX_CONCURRENCY;

  // Index configuration parameters
  static double INDEX_BUCKET_SIZE;

  // Time resolution
  static uint64_t TIME_RESOLUTION_NS;

  // Monitor configuration parameters
  static uint64_t MONITOR_WINDOW_MS;
  static uint64_t MONITOR_PERIODICITY_MS;
};

static utils::configuration_map dialog_conf(
    utils::config_utils::read_from_env("DIALOG_CONF", "conf/dialog.conf"));

int configuration_params::MAX_CONCURRENCY = dialog_conf.get<int>(
    "max_concurrency", constants::HARDWARE_CONCURRENCY);
double configuration_params::INDEX_BUCKET_SIZE = dialog_conf.get<double>(
    "index_block_size", constants::DEFAULT_INDEX_BUCKET_SIZE);
uint64_t configuration_params::TIME_RESOLUTION_NS = dialog_conf.get<uint64_t>(
    "time_resolution_ns", constants::DEFAULT_TIME_RESOLUTION_NS);
uint64_t configuration_params::MONITOR_WINDOW_MS = dialog_conf.get<uint64_t>(
    "monitor_window_ms", constants::DEFAULT_MONITOR_WINDOW_MS);
uint64_t configuration_params::MONITOR_PERIODICITY_MS = dialog_conf
    .get<uint64_t>("monitor_periodicity_ms",
                   constants::DEFAULT_MONITOR_PERIODICITY_MS);

}

#endif /* DIALOG_CONFIGURATION_PARAMS_H_ */
