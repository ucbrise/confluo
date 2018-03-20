#ifndef CONFLUO_CONTAINER_SKETCH_COUNT_SKETCH_H_
#define CONFLUO_CONTAINER_SKETCH_COUNT_SKETCH_H_

#include <array>
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

  /**
   * Constructor.
   * @param num_estimates number of estimates to track per update (depth)
   * @param num_buckets number of buckets (width)
   * @param manager hash manager
   */
  count_sketch(size_t num_estimates, size_t num_buckets, const hash_manager& manager)
      : num_estimates_(num_estimates),
        num_buckets_(num_buckets),
        counters_(num_estimates_ * num_buckets_),
        hash_manager_(manager) {
    hash_manager_.guarantee_initialized(2 * num_estimates_);
  }

  count_sketch(const count_sketch& other)
      : num_estimates_(other.num_estimates_),
        num_buckets_(other.num_buckets_),
        counters_(num_estimates_ * num_buckets_),
        hash_manager_(other.hash_manager_) {
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], atomic::load(&other.counters_[i]));
    }
  }

  count_sketch& operator=(const count_sketch& other) {
    num_estimates_ = other.num_estimates_;
    num_buckets_ = other.num_buckets_;
    counters_ = std::vector<atomic_counter_t>(num_estimates_ * num_buckets_);
    hash_manager_ = other.hash_manager_;
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], atomic::load(&other.counters_[i]));
    }
    return *this;
  }

  /**
   * Update count estimates for element.
   * @param elem element
   */
  void update(T elem) {
    for (size_t i = 0; i < num_estimates_; i++) {
      int bucket_idx = hash_manager_.hash(i, elem) % num_buckets_;
      counter_t sign = to_sign(hash_manager_.hash(num_estimates_ + i, elem));
      atomic::faa<counter_t>(&counters_[num_buckets_ * i + bucket_idx], sign);
    }
  }

  /**
   * Estimate count of element.
   * @param elem element
   * @return estimated count
   */
  counter_t estimate(T elem) {
    std::vector<counter_t> median_buf(num_estimates_);
    for (size_t i = 0; i < num_estimates_; i++) {
      size_t bucket_idx = hash_manager_.hash(i, elem) % num_buckets_;
      counter_t sign = to_sign(hash_manager_.hash(num_estimates_ + i, elem));
      median_buf[i] = sign * atomic::load(&counters_[num_buckets_ * i + bucket_idx]);
    }
    return median<counter_t>(median_buf);
  }

  /**
   * Update counts and get the old estimate.
   * @param elem element
   * @return old estimated count
   */
  counter_t update_and_estimate(T elem) {
    std::vector<counter_t> median_buf(num_estimates_);
    for (size_t i = 0; i < num_estimates_; i++) {
      int bucket_idx = hash_manager_.hash(i, elem) % num_buckets_;
      counter_t sign = to_sign(hash_manager_.hash(num_estimates_ + i, elem));
      size_t old_count = atomic::faa<counter_t>(&counters_[num_buckets_ * i + bucket_idx], 1);
      median_buf[i] = sign * old_count;
    }
    return median<counter_t>(median_buf);
  }

  /**
   * Create a cont_min_sketch with desired accuracy guarantees. TODO describe
   * @param gamma desired probability of error (0 < gamma < 1)
   * @param epsilon desired margin of error (0 < epsilon < 1)
   * @param manager hash manager
   * @return count min sketch with accuracy guarantees
   */
  // TODO rename func
  static count_sketch create_parameterized(double gamma, double epsilon, hash_manager& manager) {
    return count_sketch(count_sketch<T>::perror_to_num_estimates(gamma),
                        count_sketch<T>::error_margin_to_num_buckets(epsilon),
                        manager);
  }

 private:
  /**
   * Number of estimates per update computed from probability of error
   * @param gamma desired probability of error
   * @return number of estimates
   */
  static size_t perror_to_num_estimates(double gamma) {
    // TODO assert
    double n = std::pow(2, sizeof(T) * 8) - 1;
    return std::ceil(std::log2(n / gamma));
  }

  /**
   * Number of buckets from error margin
   * @param epsilon desired error margin
   * @return number of buckets
   */
  static size_t error_margin_to_num_buckets(double epsilon) {
    // TODO assert
    return std::ceil(std::exp(1) / epsilon);
  }

  static counter_t to_sign(size_t num) {
    return num % 2 == 1 ? 1 : -1;
  }

  size_t num_estimates_; // depth
  size_t num_buckets_; // width

  std::vector<atomic_counter_t> counters_;
  hash_manager hash_manager_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_COUNT_SKETCH_H_ */
