#include "conf/defaults.h"

namespace confluo {

const size_t archival_defaults::DEFAULT_MAX_FILE_SIZE;
const uint64_t archival_defaults::DEFAULT_PERIODICITY_MS;
const std::string archival_defaults::DEFAULT_DATA_LOG_ENCODING_TYPE = encoding_params::LZ4;
const std::string archival_defaults::DEFAULT_REFLOG_ENCODING_TYPE = encoding_params::ELIAS_GAMMA;
const uint64_t archival_defaults::DEFAULT_IN_MEMORY_DATALOG_WINDOW_BYTES;
const uint64_t archival_defaults::DEFAULT_IN_MEMORY_FILTER_WINDOW_NS;

const int defaults::HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();
constexpr double defaults::DEFAULT_INDEX_BUCKET_SIZE;
constexpr size_t defaults::DEFAULT_MAX_MEMORY;
constexpr uint64_t defaults::DEFAULT_TIME_RESOLUTION_NS;
constexpr uint64_t defaults::DEFAULT_MEMORY_MONITOR_PERIODICITY_MS;
constexpr uint64_t defaults::DEFAULT_MONITOR_WINDOW_MS;
constexpr uint64_t defaults::DEFAULT_MONITOR_PERIODICITY_MS;

}
