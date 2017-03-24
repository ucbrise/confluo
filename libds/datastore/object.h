#ifndef DATASTORE_OBJECT_H_
#define DATASTORE_OBJECT_H_

#include <cstdio>
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

  object_state(const object_state& other) {
    atomic::init(&state_, atomic::load(&other.state_));
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
  atomic::type<uint64_t> state_;
};

struct stateful {
  stateful() = default;
  stateful(const stateful& other)
      : state(other.state) {
  }
  object_state state;
};

struct object_ptr_t : public stateful {
  size_t offset :40;
  size_t length :24;
  uint64_t version;

  object_ptr_t()
      : offset(0UL),
        length(0UL),
        version(UINT64_MAX) {
  }

  object_ptr_t(const object_ptr_t& other)
      : stateful(other),
        offset(other.offset),
        length(other.length),
        version(other.version) {
  }
};

}

#endif /* DATASTORE_OBJECT_H_ */
