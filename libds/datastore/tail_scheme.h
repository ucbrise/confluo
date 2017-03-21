#ifndef DATASTORE_TAIL_SCHEME_H_
#define DATASTORE_TAIL_SCHEME_H_

#include "atomic.h"
#include <cassert>

namespace datastore {

class write_stalled_tail {
 public:
  write_stalled_tail()
      : read_tail_(0ULL),
        write_tail_(0ULL) {
  }

  uint64_t start_write_op() {
    return atomic::faa(&write_tail_, 1ULL);
  }

  void end_write_op(uint64_t tail) {
    uint64_t old_tail;
    do {
      old_tail = tail;
    } while (!atomic::weak::cas(&read_tail_, &old_tail, tail + 1));
  }

  uint64_t get_tail() const {
    return atomic::load(&read_tail_);
  }

 private:
  std::atomic<uint64_t> read_tail_;
  std::atomic<uint64_t> write_tail_;
};

class read_stalled_tail {
 public:
  read_stalled_tail()
      : tail_(0ULL) {
  }

  uint64_t start_write_op() {
    return atomic::faa(&tail_, 1ULL);
  }

  void end_write_op(uint64_t tail) {
  }

  uint64_t get_tail() const {
    return atomic::load(&tail_);
  }

 private:
  std::atomic<uint64_t> tail_;
};

}

#endif /* DATASTORE_TAIL_SCHEME_H_ */
