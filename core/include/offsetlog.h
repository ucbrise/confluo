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
    for (auto& v : valid_) {
      v.store(false);
    }
  }

  uint64_t start() {
    uint64_t record_id = current_id_.fetch_add(1L);
    return record_id;
  }

  void end(uint64_t record_id, uint64_t offset, uint16_t length) {
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_[record_id] = offlen;
    valid_[record_id].store(true);
  }

  void lookup(uint64_t record_id, uint64_t& offset, uint16_t& length) {
    uint64_t ol = offlens_.get(record_id);
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

 private:
  __monolog_base <uint64_t, 32> offlens_;
  std::array<std::atomic<bool>, 134217728> valid_;
  std::atomic<uint64_t> current_id_;
};

}

#endif /* SLOG_KVMAP_H_ */
