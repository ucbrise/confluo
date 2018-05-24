#include "archival/archival_utils.h"

namespace confluo {
namespace archival {

std::string archival_utils::filter_archival_path(const std::string &filter_log_path, size_t filter_log_idx) {
  return filter_log_path + "/filter_" + std::to_string(filter_log_idx) + "/";
}

std::string archival_utils::index_archival_path(const std::string &index_log_path, size_t index_id) {
  return index_log_path + "/index_" + std::to_string(index_id) + "/";
}

void archival_utils::swap_bucket_ptr(reflog &refs, size_t idx, encoded_reflog_ptr encoded_bucket) {
  size_t bucket_idx, container_idx;
  refs.raw_data_location(idx, container_idx, bucket_idx);
  atomic::load(&refs.data()[container_idx])[bucket_idx].swap_ptr(encoded_bucket);
}

uint64_t archival_utils::max_in_reflog_bucket(uint64_t *bucket) {
  uint64_t max = 0;
  for (size_t i = 0; i < reflog_constants::BUCKET_SIZE && bucket[i] != limits::ulong_max; i++)
    max = std::max(max, bucket[i]);
  return max;
}

}
}