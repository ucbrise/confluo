#ifndef DATASTORE_OBJECT_H_
#define DATASTORE_OBJECT_H_

#include <cstdint>
#include <atomic>

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

  object_state() {
    state_.store(uninitialized, std::memory_order_release);
  }

  void initalize() {
    state_.store(initialized, std::memory_order_release);
  }

  bool mark_updating() {
    uint64_t expected = initialized;
    return state_.compare_exchange_strong(expected, updating,
                                          std::memory_order_acquire,
                                          std::memory_order_release);
  }

  void update(uint64_t new_id) {
    state_.store(new_id, std::memory_order_acquire);
  }

  uint64_t get() {
    return state_.load(std::memory_order_release);
  }

 private:
  std::atomic<uint64_t> state_;
};

struct object {
  object_state state;
};

}

#endif /* DATASTORE_OBJECT_H_ */
