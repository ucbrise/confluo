#ifndef DATASTORE_TAIL_SCHEME_H_
#define DATASTORE_TAIL_SCHEME_H_

#include "atomic.h"
#include "object.h"
#include <cassert>

namespace datastore {

class write_stalled_tail {
 public:
  write_stalled_tail()
      : read_tail_(UINT64_C(0)),
        write_tail_(UINT64_C(0)) {
  }

  static uint64_t get_state(object& o) {
    return o.state.get();
  }

  static bool is_valid(uint64_t state) {
    return state == object_state::uninitialized;
  }

  uint64_t start_write_op() {
    return atomic::faa(&write_tail_, UINT64_C(1));
  }

  void end_write_op(object& obj, uint64_t tail) {
    uint64_t old_tail;
    do {
      old_tail = tail;
    } while (!atomic::weak::cas(&read_tail_, &old_tail, tail + 1));
  }

  bool start_update_op(object& obj) {
    return obj.state.mark_updating(object_state::uninitialized);
  }

  void end_update_op(object& obj, uint64_t new_id) {
    obj.state.update(new_id);
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
      : tail_(UINT64_C(0)) {
  }

  static uint64_t get_state(object& o) {
    uint64_t state;
    do {
      state = o.state.get();
    } while (state == datastore::object_state::uninitialized
        || state == datastore::object_state::updating);
    return state;
  }

  static bool is_valid(uint64_t state) {
    return state == object_state::initialized;
  }

  uint64_t start_write_op() {
    return atomic::faa(&tail_, UINT64_C(1));
  }

  void end_write_op(object& obj, uint64_t tail) {
    obj.state.initalize();
  }

  bool start_update_op(object& obj) {
    return obj.state.mark_updating();
  }

  void end_update_op(object& obj, uint64_t new_id) {
    obj.state.update(new_id);
  }

  uint64_t get_tail() const {
    return atomic::load(&tail_);
  }

 private:
  std::atomic<uint64_t> tail_;
};

}

#endif /* DATASTORE_TAIL_SCHEME_H_ */
