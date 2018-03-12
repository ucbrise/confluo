#ifndef CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_
#define CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_

#include <vector>
#include "hash_manager.h"
#include "types/primitive_types.h"

namespace confluo {
namespace sketch {

template<typename T>
class count_min_sketch {

 public:
  /**
   * Constructor.
   * @param manager hash manager
   */
  count_min_sketch(const hash_manager& manager)
      : count_min_sketch(count_min_sketch<T>::perror_to_num_estimates(0.01),
                         count_min_sketch<T>::error_margin_to_num_buckets(0.01),
                         manager) {
  }

  /**
   * Constructor.
   * @param num_estimates number of estimates to track per update (depth)
   * @param num_buckets number of buckets (width)
   * @param manager hash manager
   */
  count_min_sketch(size_t num_estimates, size_t num_buckets, hash_manager& manager)
      : num_estimates_(num_estimates),
        num_buckets_(num_buckets),
        counters_(new size_t[num_estimates_ * num_buckets_]()),
        hash_manager_(&manager) {
  }

  ~count_min_sketch() {
    delete[] counters_;
  }

  /**
   * Update count estimates for element
   * @param elem element
   */
  void update(T elem) {
    for (size_t i = 0; i < num_estimates_; i++) {
      int bucket_idx = hash_manager_->hash(elem, i) % num_buckets_;
      counters_[num_buckets_ * i + bucket_idx]++;
    }
  }

  /**
   * Estimate count of element
   * @param elem element
   * @return estimated count
   */
  size_t estimate(T elem) {
    size_t min_estimate = limits::ulong_max;
    for (size_t i = 0; i < num_estimates_; i++) {
      size_t bucket_idx = hash_manager_->hash(elem, i) % num_buckets_;
      min_estimate = std::min(min_estimate, counters_[num_buckets_ * i + bucket_idx]);
    }
    return min_estimate;
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
    return count_min_sketch(this->perror_to_num_estimates(gamma),
                            this->error_margin_to_num_buckets(epsilon),
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
    size_t n = std::pow(2, sizeof(T)) - 1;
    return std::ceil(std::log(n / gamma));
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

  size_t num_estimates_; // t
  size_t num_buckets_; // b

  size_t* counters_;
  hash_manager* hash_manager_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_COUNT_MIN_SKETCH_H_ */
