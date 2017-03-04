#ifndef MONOLOG_OFFSETLOG_H_
#define MONOLOG_OFFSETLOG_H_

#include <cstdint>

#include "monolog.h"
#include "utils.h"

namespace monolog {

class offsetlog {
 public:
  typedef __monolog_linear_base <uint64_t, 1024, 16777216> offlen_type;

  offsetlog() {
    current_write_id_.store(0L);
    current_read_id_.store(0L);
  }

  uint64_t start(uint64_t offset, uint16_t length) {
    uint64_t record_id = current_write_id_.fetch_add(1L,
                                                     std::memory_order_release);
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_.set(record_id, offlen);
    return record_id;
  }

  void end(const uint64_t record_id) {
    uint64_t expected = record_id;
    while (!current_read_id_.compare_exchange_weak(expected, record_id + 1,
                                                   std::memory_order_release,
                                                   std::memory_order_acquire))
      expected = record_id;
  }

  void end(const uint64_t start_id, const uint64_t count) {
    uint64_t expected = start_id;
    while (!current_read_id_.compare_exchange_weak(expected, start_id + count,
                                                   std::memory_order_release,
                                                   std::memory_order_acquire))
      expected = start_id;
  }

  uint64_t request_id_block(uint64_t num_records) {
    uint64_t start_id = current_write_id_.fetch_add(num_records,
                                                    std::memory_order_release);
    offlens_.ensure_alloc(start_id, start_id + num_records);
    return start_id;
  }

  void set(uint64_t record_id, uint64_t offset, uint16_t length) {
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_.set(record_id, offlen);
  }

  void set_without_alloc(uint64_t record_id, uint64_t offset, uint16_t length) {
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_.set_unsafe(record_id, offlen);
  }

  void lookup(uint64_t record_id, uint64_t& offset, uint16_t& length) {
    uint64_t ol = offlens_.get(record_id);
    offset = ol & 0xFFFFFFFFFFFF;
    length = ol >> 48;
  }

  bool is_valid(uint64_t record_id) {
    return record_id < current_write_id_.load(std::memory_order_acquire);
  }

  bool is_valid(uint64_t record_id, uint64_t max_rid) {
    return record_id < max_rid;
  }

  uint64_t num_ids() {
    return current_read_id_.load(std::memory_order_acquire);
  }

  size_t storage_size() {
    return offlens_.storage_size();
  }

  offlen_type offlens_;
  std::atomic<uint64_t> current_write_id_;
  std::atomic<uint64_t> current_read_id_;
};

}

#endif /* MONOLOG_OFFSETLOG_H_ */
