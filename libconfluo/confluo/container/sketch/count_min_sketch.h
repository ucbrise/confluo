#ifndef CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_
#define CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_

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

  // TODO defaults

  /**
   * Constructor.
   * @param num_estimates number of estimates to track per update (depth)
   * @param num_buckets number of buckets (width)
   * @param manager hash manager
   */
  count_min_sketch(size_t num_estimates, size_t num_buckets, const hash_manager& manager)
      : num_estimates_(num_estimates),
        num_buckets_(num_buckets),
        counters_(num_estimates_ * num_buckets_),
        hash_manager_(manager) {
    hash_manager_.guarantee_initialized(num_estimates_);
  }

  count_min_sketch(const count_min_sketch& other)
      : num_estimates_(other.num_estimates_),
        num_buckets_(other.num_buckets_),
        counters_(num_estimates_ * num_buckets_),
        hash_manager_(other.hash_manager_) {
    for (size_t i = 0; i < counters_.size(); i++) {
      atomic::store(&counters_[i], atomic::load(&other.counters_[i]));
    }
  }

  count_min_sketch& operator=(const count_min_sketch& other) {
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
      atomic::faa<counter_t>(&counters_[num_buckets_ * i + bucket_idx], 1);
    }
  }

  /**
   * Estimate count of element.
   * @param elem element
   * @return estimated count
   */
  counter_t estimate(T elem) {
    counter_t min_estimate = std::numeric_limits<counter_t>::max();
    for (size_t i = 0; i < num_estimates_; i++) {
      size_t bucket_idx = hash_manager_.hash(i, elem) % num_buckets_;
      min_estimate = std::min(min_estimate, atomic::load(&counters_[num_buckets_ * i + bucket_idx]));
    }
    return min_estimate;
  }

  /**
   * Update counts and get the old estimate.
   * @param elem element
   * @return old estimated count
   */
  counter_t update_and_estimate(T elem) {
    counter_t old_min_estimate = std::numeric_limits<counter_t>::max();
    for (size_t i = 0; i < num_estimates_; i++) {
      int bucket_idx = hash_manager_.hash(i, elem) % num_buckets_;
      size_t old_count = atomic::faa<counter_t>(&counters_[num_buckets_ * i + bucket_idx], 1);
      old_min_estimate = std::min(old_min_estimate, old_count);
    }
    return old_min_estimate;
  }

  /**
   * Create a cont_min_sketch with desired accuracy guarantees. TODO describe
   * @param gamma desired probability of error (0 < gamma < 1)
   * @param epsilon desired margin of error (0 < epsilon < 1)
   * @param manager hash manager
   * @return count min sketch with accuracy guarantees
   */
  // TODO rename func
  static count_min_sketch create_parameterized(double gamma, double epsilon, hash_manager& manager) {
    return count_min_sketch(count_min_sketch<T>::perror_to_num_estimates(gamma),
                            count_min_sketch<T>::error_margin_to_num_buckets(epsilon),
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

  size_t num_estimates_; // depth
  size_t num_buckets_; // width

  std::vector<atomic_counter_t> counters_;
  hash_manager hash_manager_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_ */