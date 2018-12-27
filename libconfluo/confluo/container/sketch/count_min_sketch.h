#ifndef CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_
#define CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_

#include <utility>
#include <vector>

#include "atomic.h"
#include "hash_manager.h"
#include "types/primitive_types.h"

namespace confluo {
namespace sketch {

/**
 * Thread-safe count-min-sketch.
 */
template<typename T, typename counter_t = size_t>
class count_min_sketch {

 public:
  typedef atomic::type<counter_t> atomic_counter_t;

  /**
   * Constructor.
   * @param num_estimates number of estimates to track per update (depth)
   * @param num_buckets number of buckets (width)
   * @param manager hash manager
   */
  count_min_sketch(size_t b, size_t t, hash_manager manager)
      : w_(b),
        d_(t),
        counters_(d_ * w_),
        hash_manager_(std::move(manager)) {
    hash_manager_.guarantee_initialized(d_);
  }

  count_min_sketch(const count_min_sketch& other)
      : w_(other.w_),
        d_(other.d_),
        counters_(d_ * w_),
        hash_manager_(other.hash_manager_) {
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], atomic::load(&other.counters_[i]));
    }
  }

  count_min_sketch& operator=(const count_min_sketch& other) {
    w_ = other.w_;
    d_ = other.d_;
    counters_ = std::vector<atomic_counter_t>(d_ * w_);
    hash_manager_ = other.hash_manager_;
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], atomic::load(&other.counters_[i]));
    }
    return *this;
  }

  /**
   * Update count estimates for key
   * @param key key
   */
  void update(T key, size_t incr = 1) {
    for (size_t i = 0; i < d_; i++) {
      size_t bucket_idx = hash_manager_.hash(i, key) % w_;
      atomic::faa<counter_t>(&counters_[w_ * i + bucket_idx], incr);
    }
  }

  /**
   * Estimate count of key
   * @param key key
   * @param incr increment
   * @return estimated count
   */
  counter_t estimate(T key) {
    counter_t min_estimate = std::numeric_limits<counter_t>::max();
    for (size_t i = 0; i < d_; i++) {
      size_t bucket_idx = hash_manager_.hash(i, key) % w_;
      min_estimate = std::min(min_estimate, atomic::load(&counters_[w_ * i + bucket_idx]));
    }
    return min_estimate;
  }

  /**
   * Update counts and get the old estimate
   * @param key key
   * @param incr increment
   * @return old estimated count
   */
  counter_t update_and_estimate(T key, size_t incr = 1) {
    counter_t old_min_estimate = std::numeric_limits<counter_t>::max();
    for (size_t i = 0; i < d_; i++) {
      size_t bucket_idx = hash_manager_.hash(i, key) % w_;
      size_t old_count = atomic::faa<counter_t>(&counters_[w_ * i + bucket_idx], incr);
      old_min_estimate = std::min(old_min_estimate, old_count);
    }
    return old_min_estimate;
  }

  /**
   * Create a count_min_sketch with desired accuracy guarantees
   * @param epsilon desired margin of error (0 < epsilon < 1)
   * @param gamma desired probability of error (0 < gamma < 1)
   * @return count-min sketch with desired accuracy guarantees
   */
  static count_min_sketch create_parameterized(double epsilon, double gamma) {
    return count_min_sketch(count_min_sketch<T>::error_margin_to_width(epsilon),
                            count_min_sketch<T>::perror_to_depth(gamma),
                            hash_manager());
  }

 private:
  /**
   * Number of estimates per update computed from probability of error
   * @param gamma desired probability of error
   * @return number of estimates
   */
  static size_t perror_to_depth(double gamma) {
    assert(gamma > 0.0 && gamma < 1.0);
    return size_t(std::ceil(sizeof(T) * 8 - std::log2(gamma))); // log2(N/gamma)
  }

  /**
   * Number of buckets from error margin
   * @param epsilon desired error margin
   * @return number of buckets
   */
  static size_t error_margin_to_width(double epsilon) {
    assert(epsilon > 0.0 && epsilon < 1.0);
    return size_t(std::ceil(std::exp(1) / epsilon));
  }

  size_t w_; // number of buckets (width)
  size_t d_; // number of estimates (depth)

  std::vector<atomic_counter_t> counters_;
  hash_manager hash_manager_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_ */
