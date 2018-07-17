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
   * @param num_estimates number of estimates to track per update (depth)
   * @param num_buckets number of buckets (width)
   */
  count_sketch(size_t num_estimates, size_t num_buckets)
      : num_estimates_(num_estimates),
        num_buckets_(num_buckets),
        counters_(num_estimates_ * num_buckets_),
        bucket_hash_manager_(),
        sign_hash_manager_() {
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], counter_t());
    }
    bucket_hash_manager_.guarantee_initialized(num_estimates_);
    sign_hash_manager_.guarantee_initialized(num_estimates_);
  }

  count_sketch(const count_sketch& other)
      : num_estimates_(other.num_estimates_),
        num_buckets_(other.num_buckets_),
        counters_(num_estimates_ * num_buckets_),
        bucket_hash_manager_(other.bucket_hash_manager_),
        sign_hash_manager_(other.sign_hash_manager_) {
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], atomic::load(&other.counters_[i]));
    }
  }

  count_sketch& operator=(const count_sketch& other) {
    num_estimates_ = other.num_estimates_;
    num_buckets_ = other.num_buckets_;
    counters_ = std::vector<atomic_counter_t>(num_estimates_ * num_buckets_);
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
  void update(T key) {
    for (size_t i = 0; i < num_estimates_; i++) {
      size_t bucket_idx = bucket_hash_manager_.hash(i, key) % num_buckets_;
      counter_t sign = to_sign(sign_hash_manager_.hash(i, key));
      atomic::faa<counter_t>(&counters_[num_buckets_ * i + bucket_idx], sign);
    }
  }

  /**
   * Estimate count of a key.
   * @param key key
   * @return estimated count
   */
  counter_t estimate(T key) {
    std::vector<counter_t> median_buf(num_estimates_);
    for (size_t i = 0; i < num_estimates_; i++) {
      size_t bucket_idx = bucket_hash_manager_.hash(i, key) % num_buckets_;
      counter_t sign = to_sign(sign_hash_manager_.hash(i, key));
      median_buf[i] = sign * atomic::load(&counters_[num_buckets_ * i + bucket_idx]);
    }
    return median(median_buf);
  }

  /**
   * Update counts and get the old estimate.
   * @param key key
   * @return old estimated count
   */
  counter_t update_and_estimate(T key) {
    std::vector<counter_t> median_buf(num_estimates_);
    for (size_t i = 0; i < num_estimates_; i++) {
      size_t bucket_idx = bucket_hash_manager_.hash(i, key) % num_buckets_;
      counter_t sign = to_sign(sign_hash_manager_.hash(i, key));
      counter_t old_count = atomic::faa<counter_t>(&counters_[num_buckets_ * i + bucket_idx], sign);
      median_buf[i] = sign * old_count;
    }
    return median(median_buf);
  }

  /**
   * @return storage size of data structure in bytes
   */
  size_t storage_size() {
   size_t counters_size_bytes = sizeof(atomic_counter_t) * (num_estimates_ * num_buckets_);
   // TODO account for hashes (O(n) increase)
   return counters_size_bytes;
  }

  /**
   * Create a cont_min_sketch with desired accuracy guarantees. TODO describe
   * @param gamma desired probability of error (0 < gamma < 1)
   * @param epsilon desired margin of error (0 < epsilon < 1)
   * @param manager hash manager
   * @return count min sketch with accuracy guarantees
   */
  // TODO rename func
  static count_sketch create_parameterized(double gamma, double epsilon) {
    return count_sketch(count_sketch<T>::perror_to_num_estimates(gamma),
                        count_sketch<T>::error_margin_to_num_buckets(epsilon));
  }

  // TODO move
  /**
   * Number of estimates per update computed from probability of error
   * @param gamma desired probability of error
   * @return number of estimates
   */
  static size_t perror_to_num_estimates(double gamma) {
    // TODO assert
    double n = std::pow(2.0, sizeof(T) * 8) - 1;
    return std::ceil(std::log2(n / gamma));
  }

  /**
   * Number of buckets from error margin
   * @param epsilon desired error margin
   * @return number of buckets
   */
  static size_t error_margin_to_num_buckets(double epsilon) {
    // TODO assert
    return std::ceil(std::exp(1) / (epsilon * epsilon));
  }

 private:
  static counter_t to_sign(size_t num) {
    return num % 2 == 1 ? 1 : -1;
  }

  size_t num_estimates_; // depth
  size_t num_buckets_; // width

  std::vector<atomic_counter_t> counters_;
  hash_manager<T> bucket_hash_manager_;
  hash_manager<T> sign_hash_manager_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_COUNT_SKETCH_H_ */
