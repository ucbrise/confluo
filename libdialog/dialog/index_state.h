#ifndef DIALOG_INDEX_STATE_H_
#define DIALOG_INDEX_STATE_H_

#include <cstdint>

#include "atomic.h"

namespace dialog {

struct index_state_t {
  static const uint8_t UNINDEXED = 0;
  static const uint8_t INDEXING = 1;
  static const uint8_t INDEXED = 2;

  atomic::type<uint8_t> state;
  uint16_t id;

  index_state_t()
      : state(UNINDEXED),
        id(UINT16_MAX) {
  }

  index_state_t(const index_state_t& other)
      : state(atomic::load(&other.state)),
        id(other.id) {
  }

  index_state_t& operator=(const index_state_t& other) {
    atomic::init(&state, atomic::load(&other.state));
    id = other.id;
    return *this;
  }

  bool is_indexed() {
    return atomic::load(&state) == INDEXED;
  }

  bool set_indexing() {
    uint8_t expected = UNINDEXED;
    return atomic::strong::cas(&state, &expected, INDEXING);
  }

  void set_indexed(uint16_t index_id) {
    id = index_id;
    atomic::store(&state, INDEXED);
  }

  void set_unindexed() {
    atomic::store(&state, UNINDEXED);
  }

  bool disable_indexing() {
    uint8_t expected = INDEXED;
    return atomic::strong::cas(&state, &expected, UNINDEXED);
  }
};

}

#endif /* DIALOG_INDEX_STATE_H_ */
