#ifndef DATASTORE_OBJECT_H_
#define DATASTORE_OBJECT_H_

#include <cstdint>
#include <cassert>
#include "atomic.h"

namespace datastore {

// Object states:
//   UINT64_MAX           uninitialized
//   UINT64_MAX - 1       initialized
//   UINT64_MAX - 2       updating
//   i < UINT64_MAX - 2   updated to id 'i'

class object_state {
 public:
  static const uint64_t uninitialized = UINT64_MAX;
  static const uint64_t initialized = UINT64_MAX - 1;
  static const uint64_t updating = UINT64_MAX - 2;

  object_state()
      : state_(uninitialized) {
  }

  void initalize() {
    atomic::store(&state_, initialized);
  }

  bool mark_updating(uint64_t expected = initialized) {
    return atomic::strong::cas(&state_, &expected, updating);
  }

  void update(uint64_t new_id) {
    atomic::store(&state_, new_id);
  }

  uint64_t get() const {
    return atomic::load(&state_);
  }

 private:
  std::atomic<uint64_t> state_;
};

struct object {
  object_state state;
};

}

#endif /* DATASTORE_OBJECT_H_ */
