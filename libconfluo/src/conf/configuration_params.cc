#include "conf/configuration_params.h"

namespace confluo {

static utils::configuration_map confluo_conf(
    utils::config_utils::read_from_env(
        "CONFLUO_CONF", "/etc/conf/confluo.conf:./conf/confluo.conf"));

uint64_t archival_configuration_params::PERIODICITY_MS = confluo_conf.get<uint64_t>(
    "archival_periodicity_ms", archival_defaults::DEFAULT_PERIODICITY_MS);
uint64_t archival_configuration_params::IN_MEMORY_DATALOG_WINDOW_BYTES = confluo_conf.get<uint64_t>(
    "archival_in_memory_datalog_window_bytes", archival_defaults::DEFAULT_IN_MEMORY_DATALOG_WINDOW_BYTES);
uint64_t archival_configuration_params::IN_MEMORY_FILTER_WINDOW_NS = confluo_conf.get<uint64_t>(
    "archival_in_memory_filter_window_ns", archival_defaults::DEFAULT_IN_MEMORY_FILTER_WINDOW_NS);
size_t archival_configuration_params::MAX_FILE_SIZE = confluo_conf.get<size_t>(
    "max_archival_file_size", archival_defaults::DEFAULT_MAX_FILE_SIZE);
uint8_t archival_configuration_params::DATA_LOG_ENCODING_TYPE = configuration_parser::to_encoding_type(
    confluo_conf.get<std::string>("data_log_archival_encoding",
                                  archival_defaults::DEFAULT_DATA_LOG_ENCODING_TYPE));
uint8_t archival_configuration_params::REFLOG_ENCODING_TYPE = configuration_parser::to_encoding_type(
    confluo_conf.get<std::string>("reflog_archival_encoding",
                                  archival_defaults::DEFAULT_REFLOG_ENCODING_TYPE));

size_t configuration_params::MAX_MEMORY = confluo_conf.get<size_t>(
    "max_memory", defaults::DEFAULT_MAX_MEMORY);
int configuration_params::MAX_CONCURRENCY = confluo_conf.get<int>(
    "max_concurrency", defaults::HARDWARE_CONCURRENCY);
double configuration_params::INDEX_BUCKET_SIZE = confluo_conf.get<double>(
    "index_block_size", defaults::DEFAULT_INDEX_BUCKET_SIZE);
uint64_t configuration_params::TIME_RESOLUTION_NS = confluo_conf.get<uint64_t>(
    "time_resolution_ns", defaults::DEFAULT_TIME_RESOLUTION_NS);
uint64_t configuration_params::MEMORY_MONITOR_PERIODICITY_MS = confluo_conf.get<uint64_t>(
    "memory_monitor_periodicity_ms", defaults::DEFAULT_MEMORY_MONITOR_PERIODICITY_MS);
uint64_t configuration_params::MONITOR_WINDOW_MS = confluo_conf.get<uint64_t>(
    "monitor_window_ms", defaults::DEFAULT_MONITOR_WINDOW_MS);
uint64_t configuration_params::MONITOR_PERIODICITY_MS = confluo_conf.get<uint64_t>(
    "monitor_periodicity_ms", defaults::DEFAULT_MONITOR_PERIODICITY_MS);

}
