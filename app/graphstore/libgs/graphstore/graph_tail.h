#ifndef GRAPHSTORE_GRAPH_TAIL_H_
#define GRAPHSTORE_GRAPH_TAIL_H_

#include <cstdint>

namespace graphstore {

// Taken from https://github.com/preshing/cpp11-on-multicore
template<typename T, uint32_t offset, uint32_t bits>
struct bit_field_member {
  T value;

  static_assert(offset + bits <= sizeof(T) * 8, "Member exceeds bitfield boundaries");
  static_assert(bits < sizeof(T) * 8, "Cannot fill entire bitfield with one member");

  static const T MAX = (T(1) << bits) - 1;
  static const T MASK = MAX << offset;

  T maximum() const {
    return MAX;
  }

  T one() const {
    return T(1) << offset;
  }

  T n(T N) {
    return N << offset;
  }

  operator T() const {
    return (value >> offset) & MAX;
  }

  bit_field_member& operator=(T v) {
    assert(v <= MAX);
    value = (value & ~MASK) | (v << offset);
    return *this;
  }

  bit_field_member& operator+=(T v) {
    assert(T(*this) + v <= MAX);
    value += v << offset;
    return *this;
  }

  bit_field_member& operator-=(T v) {
    assert(T(*this) >= v);
    value -= v << offset;
    return *this;
  }

  bit_field_member& operator++() {
    return *this += 1;
  }

  bit_field_member operator++(int) {
    bit_field_member tmp(*this);
    operator++();
    return tmp;
  }

  bit_field_member& operator--() {
    return *this -= 1;
  }

  bit_field_member operator--(int) {
    bit_field_member tmp(*this);
    operator--();
    return tmp;
  }
};

union graph_tail {
  struct wrapper {
    uint64_t value;
  };
  wrapper w;

  graph_tail(const uint64_t v = 0) {
    w.value = v;
  }

  graph_tail& operator=(const uint64_t v) {
    w.value = v;
    return *this;
  }

  operator uint64_t&() {
    return w.value;
  }

  operator uint64_t() const {
    return w.value;
  }

  bit_field_member<uint64_t, 0, 32> node_tail;
  bit_field_member<uint64_t, 32, 32> link_tail;
};

class graph_tail_ops {
 public:
  static graph_tail increment_node_tail(std::atomic<uint64_t>& tail) {
    return tail.fetch_add(graph_tail().node_tail.one(),
                          std::memory_order_acquire);
  }

  static graph_tail increment_node_tail(std::atomic<uint64_t>& tail,
                                        uint64_t batch_size) {
    return tail.fetch_add(graph_tail().node_tail.n(batch_size),
                          std::memory_order_acquire);
  }

  static graph_tail increment_link_tail(std::atomic<uint64_t>& tail) {
    return tail.fetch_add(graph_tail().link_tail.one(),
                          std::memory_order_acquire);
  }

  static graph_tail increment_link_tail(std::atomic<uint64_t>& tail,
                                        uint64_t batch_size) {
    return tail.fetch_add(graph_tail().link_tail.n(batch_size),
                          std::memory_order_acquire);
  }

  graph_tail read_tail(const std::atomic<uint64_t>& tail) {
    return tail.load(std::memory_order_release);
  }

  static void update_tail(std::atomic<uint64_t>& tail,
                          const graph_tail expected, const graph_tail desired) {
    uint64_t old_tail;
    do {
      old_tail = expected;
    } while (tail.compare_exchange_weak(old_tail,
                                        static_cast<uint64_t>(desired),
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed));
  }
};

}

#endif /* GRAPHSTORE_GRAPH_TAIL_H_ */
