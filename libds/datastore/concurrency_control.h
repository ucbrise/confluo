#ifndef DATASTORE_TAIL_SCHEME_H_
#define DATASTORE_TAIL_SCHEME_H_

#include "atomic.h"
#include <cassert>
#include <cstdio>

#include "object.h"

namespace datastore {

class write_stalled {
 public:
  static const uint64_t HI_BIT = UINT64_C(1) << ((sizeof(uint64_t) * 8) - 1);
  static const uint64_t RT_MASK = ~(UINT64_C(1) << ((sizeof(uint64_t) * 8) - 1));

  write_stalled()
      : read_tail_(UINT64_C(0)),
        write_tail_(UINT64_C(0)) {
  }

  static uint64_t get_state(const stateful& o) {
    return o.state.get();
  }

  static bool is_valid(uint64_t state) {
    return state == object_state::uninitialized
        || state == object_state::updating;
  }

  uint64_t start_write_op() {
    return atomic::faa(&write_tail_, UINT64_C(1));
  }

  void init_object(stateful& obj) {
  }

  void end_write_op(uint64_t tail) {
    uint64_t old_tail;
    do {
      old_tail = tail;
    } while (!atomic::weak::cas(&read_tail_, &old_tail, tail + 1));
  }

  static bool start_update_op(stateful& obj) {
    return obj.state.mark_updating(object_state::uninitialized);
  }

  static void end_update_op(stateful& obj, uint64_t new_id) {
    obj.state.update(new_id);
  }

  uint64_t get_tail() const {
    return atomic::load(&read_tail_) & RT_MASK;
  }

  uint64_t start_snapshot_op() {
    uint64_t tail = get_tail();
    while (!atomic::weak::cas(&read_tail_, &tail, tail | HI_BIT))
      ;
    return tail;
  }

  bool end_snapshot_op(uint64_t tail) {
    uint64_t expected = tail | HI_BIT;
    return atomic::strong::cas(&read_tail_, &expected, tail);
  }

 private:
  atomic::type<uint64_t> read_tail_;
  atomic::type<uint64_t> write_tail_;
};

class read_stalled {
 public:
  read_stalled()
      : tail_(UINT64_C(0)),
        snapshot_(false) {
  }

  static uint64_t get_state(stateful& o) {
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

  void init_object(stateful& obj) {
    while (atomic::load(&snapshot_) == true)
      ;
    obj.state.initalize();
  }

  void end_write_op(uint64_t tail) {
  }

  static bool start_update_op(stateful& obj) {
    return obj.state.mark_updating();
  }

  static void end_update_op(stateful& obj, uint64_t new_id) {
    obj.state.update(new_id);
  }

  uint64_t get_tail() const {
    return atomic::load(&tail_);
  }

  uint64_t start_snapshot_op() {
    atomic::store(&snapshot_, true);
    return get_tail();
  }

  bool end_snapshot_op(uint64_t tail) {
    atomic::store(&snapshot_, false);
    return true;
  }

 private:
  atomic::type<uint64_t> tail_;
  atomic::type<bool> snapshot_;
};

}

#endif /* DATASTORE_TAIL_SCHEME_H_ */
