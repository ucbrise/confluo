#ifndef DATASTORE_TAIL_SCHEME_H_
#define DATASTORE_TAIL_SCHEME_H_

#include <atomic>

namespace datastore {

class read_stalled {
  read_stalled() {
    tail_.store(0ULL, std::memory_order_release);
  }

  uint64_t get_tail() {
    return tail_.load(std::memory_order_acquire);
  }

  uint64_t begin_write_op(const uint64_t tail_inc) {
    return tail_.fetch_add(tail_inc);
  }

  void end_write_op(const uint64_t old_tail, const uint64_t new_tail) {
    // Do nothing
  }

  std::atomic<uint64_t> tail_;
};

class write_stalled {
  write_stalled() {
    rtail_.store(0ULL, std::memory_order_release);
  }

  uint64_t get_tail() {
    return rtail_.load(std::memory_order_acquire);
  }

  uint64_t begin_write_op(const uint64_t tail_inc) {
    return wtail_.fetch_add(tail_inc);
  }

  void end_write_op(const uint64_t old_tail, const uint64_t new_tail) {
    uint64_t expected;
    do {
      expected = old_tail;
    } while (rtail_.compare_exchange_weak(expected, new_tail,
                                          std::memory_order_release,
                                          std::memory_order_acquire));
  }

  std::atomic<uint64_t> rtail_;
  std::atomic<uint64_t> wtail_;
};

}

#endif /* DATASTORE_TAIL_SCHEME_H_ */
