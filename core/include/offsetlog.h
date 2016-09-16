#ifndef SLOG_KVMAP_H_
#define SLOG_KVMAP_H_

#include <cstdint>

#include "monolog.h"
#include "utils.h"

namespace slog {

#define VALID UINT_MAX

class offsetlog {
 public:
  offsetlog() {
  }

  void add(uint32_t record_id, uint32_t offset, uint32_t length) {
    offsets_[record_id] = offset;
    marker_[record_id] = VALID;
    lengths_[record_id] = length;
  }

  bool is_valid(const uint32_t record_id, const uint32_t mark) {
    uint32_t record_marker = marker_[record_id].load();
    return record_marker == VALID || mark < record_marker;
  }

  bool invalidate(uint32_t record_id, uint32_t mark) {
    uint32_t current_mark = marker_[record_id].load();

    // If entry was invalidated by an earlier operation, no need to invalidate
    if (mark > current_mark)
      return false;

    bool success;
    do {
      success = marker_[record_id].compare_exchange_strong(current_mark, mark);
    } while (mark < current_mark && !success);  // Repeat until success, or an
                                                // earlier operation succeeds
                                                // in invalidating

    return success;
  }

  bool lookup(uint32_t record_id, uint32_t mark, uint32_t& offset,
              uint32_t& length) {
    offset = offsets_.get(record_id);
    length = lengths_.get(record_id);
    return is_valid(record_id, mark);
  }

 private:
  __monolog_base <uint32_t, 32> offsets_;
  __monolog_base <uint16_t, 32> lengths_;
  __monolog_base <std::atomic<uint32_t>, 32> marker_;
};

}

#endif /* SLOG_KVMAP_H_ */
