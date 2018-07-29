#ifndef CONFLUO_CONTAINER_SKETCH_SUBSTREAM_SUMMARY_H
#define CONFLUO_CONTAINER_SKETCH_SUBSTREAM_SUMMARY_H

#include <vector>

#include "atomic.h"
#include "count_sketch.h"
#include "hash_manager.h"
#include "priority_queue.h"

namespace confluo {
namespace sketch {

template<typename T, typename counter_t = int64_t>
class substream_summary {

public:
  typedef atomic::type<counter_t> atomic_counter_t;
  typedef std::vector<atomic::type<T>> atomic_vector_t;
  typedef count_sketch<T, counter_t> sketch_t;

  substream_summary() = default;

  /**
   * Constructor
   * @param t depth (number of estimates)
   * @param b width (number of buckets)
   * @param k number of heavy hitters to track
   * @param a heavy hitter threshold
   * @param precise track exact heavy hitters
   */
  substream_summary(size_t t, size_t b, size_t k, double a, hash_manager m1, hash_manager m2, pairwise_indep_hash pwih)
          : hh_threshold_(a),
            num_hh_(k),
            l2_squared_(),
            sketch_(t, b, m1, m2),
            heavy_hitters_(k),
            hhs_precise_(),
            hh_hash_(pwih),
            use_precise_hh_(true) {
  }

  /**
   * Constructor
   * @param t depth (number of estimates)
   * @param b width (number of buckets)
   * @param k number of heavy hitters to track
   * @param a heavy hitter threshold
   * @param precise track exact heavy hitters
   */
  substream_summary(size_t t, size_t b, size_t k, double a, bool precise = true)
          : hh_threshold_(a),
            num_hh_(k),
            l2_squared_(),
            sketch_(t, b),
            heavy_hitters_(k),
            hhs_precise_(),
            hh_hash_(pairwise_indep_hash::generate_random()),
            use_precise_hh_(precise) {
  }

  substream_summary(const substream_summary& other)
          : hh_threshold_(other.hh_threshold_),
            num_hh_(other.num_hh_),
            l2_squared_(atomic::load(&other.l2_squared_)),
            sketch_(other.sketch_),
            heavy_hitters_(other.heavy_hitters_.size()),
            hhs_precise_(other.hhs_precise_),
            hh_hash_(other.hh_hash_),
            use_precise_hh_(other.use_precise_hh_) {
    for (size_t i = 0; i < other.heavy_hitters_.size(); i++) {
      atomic::store(&heavy_hitters_[i], atomic::load(&other.heavy_hitters_[i]));
    }
  }

  substream_summary& operator=(const substream_summary& other) {
    hh_threshold_ = other.hh_threshold_;
    num_hh_ = other.num_hh_;
    l2_squared_ = atomic::load(&other.l2_squared_);
    sketch_ = other.sketch_;
    heavy_hitters_ = atomic_vector_t(other.heavy_hitters_.size());
    hhs_precise_ = other.hhs_precise_;
    hh_hash_ = other.hh_hash_;
    use_precise_hh_ = other.use_precise_hh_;
    for (size_t i = 0; i < other.heavy_hitters_.size(); i++) {
      atomic::store(&heavy_hitters_[i], atomic::load(&other.heavy_hitters_[i]));
    }
    return *this;
  }

  void update(T key, size_t incr = 1) {
    counter_t old_count = sketch_.update_and_estimate(key, incr);
    counter_t update = l2_squared_update(old_count, incr);
    counter_t old_l2_sq = atomic::faa(&l2_squared_, update);
    double new_l2 = std::sqrt(old_l2_sq + update);

    if (use_precise_hh_) {
      this->update_hh_pq(key, old_count + incr, new_l2);
    } else {
      this->update_hh_approx(key, old_count + incr, new_l2);
    }
  }

  /**
   * Estimate count
   * @param key key
   * @return estimated count
   */
  counter_t estimate(T key) {
    return sketch_.estimate(key);
  }

  /**
   * @return sketch
   */
  sketch_t& get_sketch() {
    return sketch_;
  }

  atomic_vector_t& get_heavy_hitters() {
    return heavy_hitters_;
  }

  heavy_hitter_set<T, counter_t>& get_pq() {
    return hhs_precise_;
  }

  /**
   * @return size of data structure in bytes
   */
  size_t storage_size() {
    size_t total_size = 0;
    total_size += sketch_.storage_size();
    if (use_precise_hh_)
      total_size += hhs_precise_.storage_size();
    else
      total_size += heavy_hitters_.size();
    return total_size;
  }

private:
  /**
   * Update heavy hitters priority queue
   * @param key key
   * @param count frequency count
   * @param l2 current l2 norm
   */
  void update_hh_pq(T key, counter_t count, double l2) {
    if (count < hh_threshold_ * l2) {
      return;
    }
    if (hhs_precise_.size() < num_hh_) {
      hhs_precise_.update(key, count);
    }
    else {
      T head = hhs_precise_.top().key_;
      if (sketch_.estimate(head) < count) {
        hhs_precise_.pop();
        hhs_precise_.update(key, count);
      }
    }
  }

  /**
   * Update heavy hitters approximate DS
   * @param key key
   * @param count frequency count
   * @param l2 current l2 norm
   */
  void update_hh_approx(T key, counter_t count, double l2) {
    if (count < hh_threshold_ * l2) {
      return;
    }
    bool done = false;
    while (!done) {
      size_t idx = hh_hash_.apply<T>(key) % heavy_hitters_.size();
      T prev = atomic::load(&heavy_hitters_[idx]);
      if (prev == key)
        return;
      counter_t prev_count = sketch_.estimate(prev);
      done = (prev_count > count) ? true : atomic::strong::cas(&heavy_hitters_[idx], &prev, key);
    }
  }

  /**
   * L_2^2 += (c_i + 1)^2 - (c_i)^2
   * L_2^2 += (c_i + incr)^2 - (c_i)^2
   * @param old_count estimate of a count before an update
   */
  static inline counter_t l2_squared_update(counter_t old_count, size_t incr) {
    return (2 * old_count * incr + incr) * incr;
  }

  double hh_threshold_; // heavy hitter threshold
  size_t num_hh_; // number of heavy hitters to track (k)

  atomic_counter_t l2_squared_; // L2 norm squared
  sketch_t sketch_;
  atomic_vector_t heavy_hitters_;
  heavy_hitter_set<T, counter_t> hhs_precise_;
  pairwise_indep_hash hh_hash_;

  bool use_precise_hh_;

};

}
}

#endif // CONFLUO_CONTAINER_SKETCH_SUBSTREAM_SUMMARY_H
