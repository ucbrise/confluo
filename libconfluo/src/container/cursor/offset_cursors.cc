#include "container/cursor/offset_cursors.h"

namespace confluo {

data_log_cursor::data_log_cursor(uint64_t version, uint64_t record_size, size_t batch_size)
    : offset_cursor(batch_size),
      current_offset_(0),
      version_(version),
      record_size_(record_size) {
  init();
}

size_t data_log_cursor::load_next_batch() {
  size_t i = 0;
  for (; i < current_batch_.size() && current_offset_ < version_; i++, current_offset_ += record_size_) {
    current_batch_[i] = current_offset_;
  }
  return i;
}

}