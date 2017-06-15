#ifndef DIALOG_CONCURRENCY_CONTROL_H_
#define DIALOG_CONCURRENCY_CONTROL_H_

#include <cassert>
#include <cstdio>
#include <thread>

#include "atomic.h"
#include "data_point.h"
#include "monolog.h"

namespace dialog {

class write_stalled {
 public:
  write_stalled()
      : read_tail_(UINT64_C(0)),
        write_tail_(UINT64_C(0)) {
  }

  static uint64_t get_state(const stateful& o) {
    return o.state.get();
  }

  static bool is_valid(uint64_t state) {
    return state == data_state::uninitialized
        || state == data_state::updating;
  }

  uint64_t start_write_op() {
    return atomic::faa(&write_tail_, UINT64_C(1));
  }

  // Batch op
  uint64_t start_write_op(uint64_t cnt) {
    return atomic::faa(&write_tail_, cnt);
  }

  void init_object(stateful& obj, uint64_t tail) {
    // Do nothing
  }

  void end_write_op(uint64_t tail) {
    uint64_t old_tail = tail;
    while (!atomic::weak::cas(&read_tail_, &old_tail, tail + 1)) {
      old_tail = tail;
      std::this_thread::yield();
    }
  }

  void end_write_op(uint64_t tail, uint64_t cnt) {
    uint64_t old_tail = tail;
    while (!atomic::weak::cas(&read_tail_, &old_tail, tail + cnt)) {
      old_tail = tail;
      std::this_thread::yield();
    }
  }

  static bool start_update_op(stateful& obj) {
    return obj.state.mark_updating(data_state::uninitialized);
  }

  static void end_update_op(stateful& obj, uint64_t new_id) {
    obj.state.complete_updating(new_id);
  }

  uint64_t get_tail() const {
    return atomic::load(&read_tail_);
  }

 private:
  atomic::type<uint64_t> read_tail_;
  atomic::type<uint64_t> write_tail_;
};

class read_stalled {
 public:
  read_stalled()
      : tail_(UINT64_C(0)),
        snapshot_(UINT64_MAX) {
  }

  static uint64_t get_state(stateful& o) {
    uint64_t state;
    do {
      state = o.state.get();
    } while (state == dialog::data_state::uninitialized
        || state == dialog::data_state::updating);
    return state;
  }

  static bool is_valid(uint64_t state) {
    return state == data_state::initialized;
  }

  uint64_t start_write_op() {
    return atomic::faa(&tail_, UINT64_C(1));
  }

  // Batch op
  uint64_t start_write_op(uint64_t cnt) {
    return atomic::faa(&tail_, cnt);
  }

  void init_object(stateful& obj, uint64_t tail) {
    while (tail >= atomic::load(&snapshot_)) {
      std::this_thread::yield();
    }
    obj.state.initalize();
  }

  void end_write_op(uint64_t tail) {
    // Do nothing
  }

  void end_write_op(uint64_t tail, uint64_t cnt) {
    // Do nothing
  }

  static bool start_update_op(stateful& obj) {
    return obj.state.mark_updating();
  }

  static void end_update_op(stateful& obj, uint64_t new_id) {
    obj.state.complete_updating(new_id);
  }

  uint64_t get_tail() const {
    return atomic::load(&tail_);
  }

 private:
  atomic::type<uint64_t> tail_;
  atomic::type<uint64_t> snapshot_;
};

namespace append_only {

class write_stalled {
 public:
  static const uint64_t HI_BIT = UINT64_C(1) << ((sizeof(uint64_t) * 8) - 1);
  static const uint64_t RT_MASK = ~(UINT64_C(1) << ((sizeof(uint64_t) * 8) - 1));

  write_stalled()
      : read_tail_(UINT64_C(0)),
        write_tail_(UINT64_C(0)) {
  }

  uint64_t get_tail() const {
    return atomic::load(&read_tail_) & RT_MASK;
  }

  bool is_valid(uint64_t id, uint64_t tail) const {
    return id < tail;
  }

  uint64_t start_write_op(uint64_t cnt) {
    return atomic::faa(&write_tail_, cnt);
  }

  void end_write_op(uint64_t tail, uint64_t cnt) {
    uint64_t old_tail = tail;
    while (!atomic::weak::cas(&read_tail_, &old_tail, tail + cnt)) {
      old_tail = tail;
      std::this_thread::yield();
    }
  }

 private:
  atomic::type<uint64_t> read_tail_;
  atomic::type<uint64_t> write_tail_;
};

class read_stalled {
 public:
  read_stalled()
      : tail_(UINT64_C(0)),
        snapshot_(UINT64_MAX) {
  }

  uint64_t get_tail() const {
    return atomic::load(&tail_);
  }

  bool is_valid(uint64_t id, uint64_t tail) const {
    if (id < tail) {
      while (!valid_.get_bit(id)) {
        std::this_thread::yield();
      }
      return true;
    }
    return false;
  }

  uint64_t start_write_op(uint64_t cnt) {
    return atomic::faa(&tail_, cnt);
  }

  void end_write_op(uint64_t tail, uint64_t cnt) {
    while (tail >= atomic::load(&snapshot_)) {
      std::this_thread::yield();
    }
    valid_.set_bits(tail, cnt);
  }

 private:
  typedef monolog::monolog_linear_base<atomic::type<uint64_t>, 1024> bits;
  monolog::monolog_bitvector<bits> valid_;
  atomic::type<uint64_t> tail_;
  atomic::type<uint64_t> snapshot_;
};

}

}

#endif /* DIALOG_CONCURRENCY_CONTROL_H_ */
