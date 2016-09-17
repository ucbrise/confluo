#ifndef SLOG_KVMAP_H_
#define SLOG_KVMAP_H_

#include <cstdint>

#include "monolog.h"
#include "utils.h"

namespace slog {

class offsetlog {
 public:
  offsetlog() {
  }

  void add(uint32_t record_id, uint32_t offset, uint16_t length) {
    offsets_[record_id] = ((uint64_t) length) << 48 | offset;
  }

  void lookup(uint32_t record_id, uint32_t& offset, uint16_t& length) {
    uint64_t ol = offsets_.get(record_id);
    offset = ol & 0xFFFFFFFF;
    length = ol >> 48;
  }

 private:
  __monolog_base <uint64_t, 32> offsets_;
};

}

#endif /* SLOG_KVMAP_H_ */
