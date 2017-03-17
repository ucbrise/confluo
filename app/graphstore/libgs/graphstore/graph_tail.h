#ifndef GRAPHSTORE_GRAPH_TAIL_H_
#define GRAPHSTORE_GRAPH_TAIL_H_

namespace graphstore {

class write_stalled_tail {
 public:
  write_stalled_tail() {
    write_tail_.store(0ULL, std::memory_order_release);
    read_tail_.store(0ULL, std::memory_order_release);
  }

  uint64_t start_write_op() {
    return write_tail_.fetch_add(1ULL, std::memory_order_release);
  }

  void end_write_op(uint64_t tail) {
    uint64_t old_tail;
    do {
      old_tail = tail;
    } while (!read_tail_.compare_exchange_weak(old_tail, tail + 1,
                                               std::memory_order_release,
                                               std::memory_order_acquire));
  }

  uint64_t get_tail() const {
    return read_tail_.load(std::memory_order_acquire);
  }

 private:
  std::atomic<uint64_t> read_tail_;
  std::atomic<uint64_t> write_tail_;
};

class read_stalled_tail {
 public:
  read_stalled_tail() {
    tail_.store(0ULL, std::memory_order_release);
  }

  uint64_t start_write_op() {
    return tail_.fetch_add(1ULL, std::memory_order_release);
  }

  void end_write_op(uint64_t tail) {}

  uint64_t get_tail() const {
    return tail_.load(std::memory_order_acquire);
  }

 private:
  std::atomic<uint64_t> tail_;
};

}

#endif /* GRAPHSTORE_GRAPH_TAIL_H_ */
