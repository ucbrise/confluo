#ifndef MONOLOG_OFFSETLOG_H_
#define MONOLOG_OFFSETLOG_H_

#include <cstdint>

#include "monolog.h"

namespace monolog {

class offsetlog {
 public:
  typedef monolog_linear_base<uint64_t, 1024, 16777216> offlen_type;

  offsetlog()
      : current_write_id_(UINT64_C(0)),
        current_read_id_(UINT64_C(0)) {
  }

  uint64_t start(uint64_t offset, uint16_t length) {
    uint64_t record_id = atomic::faa(&current_write_id_, UINT64_C(1));
    uint64_t offlen = ((uint64_t) length) << 48 | (offset & 0xFFFFFFFFFFFF);
    offlens_.set(record_id, offlen);
    return record_id;
  }

  void end(const uint64_t record_id) {
    uint64_t expected = record_id;
    while (!atomic::weak::cas(&current_read_id_, &expected, record_id + 1))
      expected = record_id;
  }

  void end(const uint64_t start_id, const uint64_t count) {
    uint64_t expected = start_id;
    while (!atomic::weak::cas(&current_read_id_, &expected, start_id + count))
      expected = start_id;
  }

  uint64_t request_id_block(uint64_t num_records) {
    uint64_t start_id = atomic::faa(&current_write_id_, num_records);
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
    return record_id < atomic::load(&current_write_id_);
  }

  bool is_valid(uint64_t record_id, uint64_t max_rid) {
    return record_id < max_rid;
  }

  uint64_t num_ids() {
    return atomic::load(&current_write_id_);
  }

  size_t storage_size() {
    return offlens_.storage_size();
  }

  offlen_type offlens_;
  atomic::type<uint64_t> current_write_id_;
  atomic::type<uint64_t> current_read_id_;
};

}

#endif /* MONOLOG_OFFSETLOG_H_ */
