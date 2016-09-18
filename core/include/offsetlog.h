#ifndef SLOG_KVMAP_H_
#define SLOG_KVMAP_H_

#include <cstdint>

#include "monolog.h"
#include "utils.h"

namespace slog {

class offsetlog {
 public:
  struct offlen_entry {
    offlen_entry() {
      offlen = 0;
      valid.store(false);
    }

    uint64_t offlen;
    std::atomic<bool> valid;
  };

  offsetlog() {
    current_id_.store(0L);
  }

  uint64_t start(uint64_t offset, uint16_t length) {
    uint64_t record_id = current_id_.fetch_add(1L);
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_[record_id].offlen = offlen;
    return record_id;
  }

  void end(uint64_t record_id) {
    offlens_[record_id].valid.store(true);
  }

  void lookup(uint64_t record_id, uint64_t& offset, uint16_t& length) {
    uint64_t ol = offlens_[record_id].offlen;
    offset = ol & 0xFFFFFFFF;
    length = ol >> 48;
  }

  bool is_valid(uint64_t record_id) {
    return record_id < current_id_.load() && offlens_[record_id].valid.load();
  }

  bool is_valid(uint64_t record_id, uint64_t max_rid) {
    return record_id < max_rid && offlens_[record_id].valid.load() > 0;
  }

  uint64_t num_ids() {
    return current_id_.load();
  }

  __monolog_base <offlen_entry, 32> offlens_;
  std::atomic<uint64_t> current_id_;
};

}

#endif /* SLOG_KVMAP_H_ */
