#ifndef SLOG_KVMAP_H_
#define SLOG_KVMAP_H_

#include <cstdint>

#include "monolog.h"
#include "utils.h"

namespace slog {

class offsetlog {
 public:
  struct atomic_bool {
    std::atomic_bool valid;
    atomic_bool() {
      valid.store(0);
    }

    bool load() {
      return valid.load();
    }

    void set() {
      valid.store(true);
    }
  };

  offsetlog() {
    current_id_.store(0L);
  }

  uint64_t start(uint64_t offset, uint16_t length) {
    uint64_t record_id = current_id_.fetch_add(1L, std::memory_order_release);
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_.set(record_id, offlen);
    return record_id;
  }

  void end(uint64_t record_id) {
    valid_[record_id].set();
  }

  uint64_t request_id_block(uint32_t num_records) {
    uint64_t start_id = current_id_.fetch_add(num_records,
                                              std::memory_order_release);
    offlens_.ensure_alloc(start_id, start_id + num_records);
    valid_.ensure_alloc(start_id, start_id + num_records);
    return start_id;
  }

  void set(uint32_t record_id, uint64_t offset, uint16_t length) {
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_.set(record_id, offlen);
  }

  void lookup(uint64_t record_id, uint64_t& offset, uint16_t& length) {
    uint64_t ol = offlens_.get(record_id);
    offset = ol & 0xFFFFFFFF;
    length = ol >> 48;
  }

  bool is_valid(uint64_t record_id) {
    return record_id < current_id_.load(std::memory_order_acquire)
        && valid_[record_id].load();
  }

  bool is_valid(uint64_t record_id, uint64_t max_rid) {
    return record_id < max_rid && valid_[record_id].load() > 0;
  }

  uint64_t num_ids() {
    return current_id_.load(std::memory_order_acquire);
  }

  size_t storage_size() {
    return offlens_.storage_size() + valid_.storage_size();
  }

  __monolog_base <uint64_t, 32> offlens_;
  __monolog_base <atomic_bool, 32> valid_;
  std::atomic<uint64_t> current_id_;
};

}

#endif /* SLOG_KVMAP_H_ */
