#ifndef CONFLUO_CONTAINER_SKETCH_COUNT_SKETCH_H_
#define CONFLUO_CONTAINER_SKETCH_COUNT_SKETCH_H_

#include <array>
#include <algorithm>
#include <vector>

#include "atomic.h"
#include "hash_manager.h"
#include "types/primitive_types.h"
#include "sketch_utils.h"

namespace confluo {
namespace sketch {

/**
 * Thread-safe count-sketch.
 */
template<typename T, typename counter_t = int64_t>
class count_sketch {

 public:
  typedef atomic::type<counter_t> atomic_counter_t;

  // TODO defaults
  count_sketch() = default;

  /**
   * Constructor.
   * @param t number of estimates per update (depth)
   * @param b number of buckets (width)
   */
  count_sketch(size_t t, size_t b, hash_manager m1, hash_manager m2)
          : depth_(t),
            width_(b),
            counters_(depth_ * width_),
            bucket_hash_manager_(m1),
            sign_hash_manager_(m2) {
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], counter_t());
    }
  }

  /**
   * Constructor.
   * @param t number of estimates per update (depth)
   * @param b number of buckets (width)
   */
  count_sketch(size_t t, size_t b)
      : depth_(t),
        width_(b),
        counters_(depth_ * width_),
        bucket_hash_manager_(),
        sign_hash_manager_() {
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], counter_t());
    }
    bucket_hash_manager_.guarantee_initialized(depth_);
    sign_hash_manager_.guarantee_initialized(depth_);
  }

  count_sketch(const count_sketch& other)
      : depth_(other.depth_),
        width_(other.width_),
        counters_(depth_ * width_),
        bucket_hash_manager_(other.bucket_hash_manager_),
        sign_hash_manager_(other.sign_hash_manager_) {
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], atomic::load(&other.counters_[i]));
    }
  }

  count_sketch& operator=(const count_sketch& other) {
    depth_ = other.depth_;
    width_ = other.width_;
    counters_ = std::vector<atomic_counter_t>(depth_ * width_);
    bucket_hash_manager_ = other.bucket_hash_manager_;
    sign_hash_manager_ = other.sign_hash_manager_;
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], atomic::load(&other.counters_[i]));
    }
    return *this;
  }

  /**
   * Update count estimates for key.
   * @param key key
   */
  void update(T key, size_t incr = 1) {
    for (size_t i = 0; i < depth_; i++) {
      size_t bucket_idx = bucket_hash_manager_.hash(i, key) % width_;
      counter_t sign = to_sign(sign_hash_manager_.hash(i, key));
      atomic::faa<counter_t>(&counters_[width_ * i + bucket_idx], sign * incr);
    }
  }

  /**
   * Estimate count of a key.
   * @param key key
   * @return estimated count
   */
  counter_t estimate(T key) const {
    std::vector<counter_t> median_buf(depth_);
    for (size_t i = 0; i < depth_; i++) {
      size_t bucket_idx = bucket_hash_manager_.hash(i, key) % width_;
      counter_t sign = to_sign(sign_hash_manager_.hash(i, key));
      median_buf[i] = sign * atomic::load(&counters_[width_ * i + bucket_idx]);
    }
    return median(median_buf);
  }

  /**
   * Update counts and get the old estimate.
   * @param key key
   * @return old estimated count
   */
  counter_t update_and_estimate(T key, size_t incr = 1) {
    std::vector<counter_t> median_buf(depth_);
    for (size_t i = 0; i < depth_; i++) {
      size_t bucket_idx = bucket_hash_manager_.hash(i, key) % width_;
      counter_t sign = to_sign(sign_hash_manager_.hash(i, key));
      counter_t old_count = atomic::faa<counter_t>(&counters_[width_ * i + bucket_idx], sign * incr);
      median_buf[i] = sign * old_count;
    }
    return median(median_buf);
  }

  size_t depth() {
    return depth_;
  }

  size_t width() {
    return width_;
  }

  /**
   * @return storage size of data structure in bytes
   */
  size_t storage_size() {
   size_t counters_size_bytes = sizeof(atomic_counter_t) * (depth_ * width_);
   // TODO account for hashes (O(n) increase)
   return counters_size_bytes;
  }

  /**
   * Create a count_sketch with desired accuracy guarantees.
   * @param epsilon desired margin of error (0 < epsilon < 1)
   * @param gamma desired probability of error (0 < gamma < 1)
   * @param manager hash manager
   * @return count min sketch with accuracy guarantees
   */
  static count_sketch create_parameterized(double epsilon, double gamma) {
    return count_sketch(count_sketch<T>::perror_to_depth(gamma),
                        count_sketch<T>::error_margin_to_width(epsilon));
  }

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
    return size_t(std::ceil(std::exp(1) / (epsilon * epsilon)));
  }

 private:
  static counter_t to_sign(size_t num) {
    return num % 2 == 1 ? 1 : -1;
  }

  size_t depth_; // number of estimates
  size_t width_; // number of buckets

  std::vector<atomic_counter_t> counters_;
  hash_manager bucket_hash_manager_;
  hash_manager sign_hash_manager_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_COUNT_SKETCH_H_ */
