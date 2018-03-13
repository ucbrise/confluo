#ifndef CONFLUO_CONTAINER_CURSOR_OFFSET_CURSORS_H_
#define CONFLUO_CONTAINER_CURSOR_OFFSET_CURSORS_H_

#include "batched_cursor.h"

namespace confluo {

typedef batched_cursor<uint64_t> offset_cursor;

class data_log_cursor : public offset_cursor {
 public:
  data_log_cursor(uint64_t version, uint64_t record_size,
                  size_t batch_size = 64)
      : offset_cursor(batch_size),
        current_offset_(0),
        version_(version),
        record_size_(record_size) {
    init();
  }

  virtual size_t load_next_batch() override {
    size_t i = 0;
    for (; i < current_batch_.size() && current_offset_ < version_;
        i++, current_offset_ += record_size_) {
      current_batch_[i] = current_offset_;
    }
    return i;
  }

 private:
  uint64_t current_offset_;
  uint64_t version_;
  uint64_t record_size_;
};

template<typename iterator>
class offset_iterator_cursor : public offset_cursor {
 public:
  offset_iterator_cursor(const iterator& begin, const iterator& end,
                         uint64_t version, size_t batch_size = 64)
      : offset_cursor(batch_size),
        cur_(begin),
        end_(end),
        version_(version) {
    init();
  }

  virtual size_t load_next_batch() override {
    size_t i = 0;
    for (; i < current_batch_.size() && cur_ != end_; ++i, ++cur_) {
      if ((current_batch_[i] = *cur_) >= version_) {
        i--;
      }
    }
    return i;
  }

 private:
  iterator cur_;
  iterator end_;
  uint64_t version_;
};

}

#endif /* CONFLUO_CONTAINER_CURSOR_OFFSET_CURSORS_H_ */
