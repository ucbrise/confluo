#ifndef SLOG_KVMAP_H_
#define SLOG_KVMAP_H_

#include <cstdint>

#include "monolog.h"
#include "utils.h"

namespace slog {

class offsetlog {
 public:
  offsetlog() {
    current_id_.store(0L);
    offlens_ = new __monolog_base <uint64_t, 32>;
    valid_ = new std::atomic_bool[134217728];
    for (uint64_t i = 0; i < 134217728; i++) {
      valid_[i].store(false);
    }
  }

  uint64_t start(uint64_t offset, uint16_t length) {
    uint64_t record_id = current_id_.fetch_add(1L);
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_->set(record_id, offlen);
    return record_id;
  }

  void end(uint64_t record_id) {
    valid_[record_id].store(true);
  }

  void lookup(uint64_t record_id, uint64_t& offset, uint16_t& length) {
    uint64_t ol = offlens_->get(record_id);
    offset = ol & 0xFFFFFFFF;
    length = ol >> 48;
  }

  bool is_valid(uint64_t record_id) {
    return record_id < current_id_.load() && valid_[record_id].load();
  }

  bool is_valid(uint64_t record_id, uint64_t max_rid) {
    return record_id < max_rid && valid_[record_id].load() > 0;
  }

  uint64_t num_ids() {
    return current_id_.load();
  }

  __monolog_base <uint64_t, 32> *offlens_;
  std::atomic_bool* valid_;
  std::atomic<uint64_t> current_id_;
};

}

#endif /* SLOG_KVMAP_H_ */
