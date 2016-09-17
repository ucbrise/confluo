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
  }

  uint64_t start() {
    uint64_t record_id = current_id_.fetch_add(1L);
    offlens_.alloc(record_id);
    return record_id;
  }

  void end(uint64_t record_id, uint64_t offset, uint16_t length) {
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_[record_id].store(offlen);
  }

  void lookup(uint64_t record_id, uint64_t& offset, uint16_t& length) {
    uint64_t ol = offlens_.load(record_id);
    offset = ol & 0xFFFFFFFF;
    length = ol >> 48;
  }

  bool is_valid(uint64_t record_id) {
    return record_id < current_id_.load() && offlens_.load(record_id) > 0;
  }

  bool is_valid(uint64_t record_id, uint64_t max_rid) {
    return record_id < max_rid && offlens_.load(record_id) > 0;
  }

  uint64_t num_ids() {
    return current_id_.load();
  }

 private:
  __atomic_monolog_base <uint64_t, 32> offlens_;
  std::atomic<uint64_t> current_id_;
};

}

#endif /* SLOG_KVMAP_H_ */
