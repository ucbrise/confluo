#ifndef CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_
#define CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_

#include <vector>

#include "count_sketch.h"
#include "hash_manager.h"
#include "container/monolog/monolog_exp2_linear.h"

namespace confluo {
namespace sketch {

template<typename T, typename counter_t = int64_t>
class substream_summary {

 public:
  typedef atomic::type<counter_t> atomic_counter_t;
  typedef std::vector<atomic::type<T>> heavy_hitters_set;
  typedef count_sketch<T, counter_t> sketch;

  substream_summary() = default;

  substream_summary(size_t num_estimates, size_t num_buckets, size_t num_heavy_hitters,
                       double hh_threshold, atomic_counter_t* l2_squared)
      : hh_threshold_(hh_threshold),
        num_hh_(num_heavy_hitters),
        l2_squared_(l2_squared),
        sketch_(num_estimates, num_buckets),
        heavy_hitters_(num_heavy_hitters),
        hh_hash_(pairwise_indep_hash<T>::generate_random()) {
  }

  substream_summary(const substream_summary& other)
      : hh_threshold_(other.hh_threshold_),
        num_hh_(other.num_hh_),
        l2_squared_(other.l2_squared_),
        sketch_(other.sketch_),
        heavy_hitters_(other.heavy_hitters_.size()),
        hh_hash_(other.hh_hash_) {
    for (size_t i = 0; i < other.heavy_hitters_.size(); i++) {
      atomic::store(&heavy_hitters_[i], atomic::load(&other.heavy_hitters_[i]));
    }
  }

  substream_summary& operator=(const substream_summary& other) {
    hh_threshold_ = other.hh_threshold_;
    num_hh_ = other.num_hh_;
    l2_squared_ = other.l2_squared_; // atomic load??
    sketch_ = other.sketch_;
    heavy_hitters_ = heavy_hitters_set(other.heavy_hitters_.size());
    hh_hash_ = other.hh_hash_;
    for (size_t i = 0; i < other.heavy_hitters_.size(); i++) {
      atomic::store(&heavy_hitters_[i], atomic::load(&other.heavy_hitters_[i]));
    }
    return *this;
  }

  void update(T key) {
    counter_t old_count = sketch_.update_and_estimate(key);
    counter_t update = l2_squared_update(old_count);
    counter_t old_l2_sq = atomic::faa(l2_squared_, update);
    double new_l2 = std::sqrt(old_l2_sq + update);
    this->update_heavy_hitters(key, old_count + 1, new_l2);
  }

  heavy_hitters_set& get_heavy_hitters() {
    return heavy_hitters_;
  }

  sketch& get_sketch() {
    return sketch_;
  }

 private:
  void update_heavy_hitters(T key, counter_t count, double l2) {
    if (count < hh_threshold_ * l2) {
      return;
    }
    bool updated = false;
    while (!updated) {
      size_t idx = hh_hash_.apply(key) % heavy_hitters_.size();
      T prev = atomic::load(&heavy_hitters_[idx]);
      if (prev == key)
        return;
      counter_t prev_count = sketch_.estimate(prev);
      if (prev_count <= count) {
        updated = atomic::strong::cas(&heavy_hitters_[idx], &prev, key);
      }
      else
        updated = true;
    }
  }

  /**
   * L_2^2 += (c_i + 1)^2 - (c_i)^2
   * @param old_count estimate of a count before an update
   */
  static inline counter_t l2_squared_update(counter_t old_count) {
    return 2 * old_count + 1;
  }

  double hh_threshold_; // heavy hitter threshold
  size_t num_hh_; // number of heavy hitters to track

  atomic_counter_t* l2_squared_; // L2 norm squared
  sketch sketch_;
  heavy_hitters_set heavy_hitters_;
  pairwise_indep_hash<T> hh_hash_;

};

template<typename T, typename counter_t = int64_t>
class universal_monitor {

 public:
  typedef atomic::type<counter_t> atomic_counter_t;
  typedef std::vector<atomic::type<T>> heavy_hitters_set;
  template<typename g_ret_t> using g_fn = std::function<g_ret_t(counter_t)>;


  universal_monitor(size_t num_estimates, size_t num_buckets, double hh_threshold, size_t num_heavy_hitters,
                    hash_manager<T>& monitor_hash_manager)
      : universal_monitor(8 * sizeof(T), num_estimates, num_buckets, hh_threshold, num_heavy_hitters,
                          monitor_hash_manager) {
  }

  universal_monitor(size_t num_substreams, size_t num_estimates, size_t num_buckets,
                    double hh_threshold, size_t num_heavy_hitters,
                    hash_manager<T>& monitor_hash_manager)
      : l2_squared_(0),
        substream_summaries_(num_substreams),
        hash_manager_(monitor_hash_manager) {
    hash_manager_.guarantee_initialized(num_substreams);
    for (size_t i = 0; i < num_substreams - 1; i++) {
      substream_summaries_[i] = substream_summary<T, counter_t>(num_estimates, num_buckets, num_heavy_hitters,
                                                               hh_threshold, &l2_squared_);
    }
  }

  universal_monitor(const universal_monitor& other)
      : l2_squared_(atomic::load(&other.l2_squared_)),
        substream_summaries_(other.substream_summaries_),
        hash_manager_(other.hash_manager_) {
  }

  universal_monitor& operator=(const universal_monitor& other) {
    l2_squared_ = atomic::load(&other.l2_squared_);
    substream_summaries_ = other.substream_summaries_;
    hash_manager_ = other.hash_manager_;
    return *this;
  }

  /**
   * Update universal sketch with an element.
   * @param elem element
   */
  void update(T key) {
    substream_summaries_[0].update(key);
    for (size_t i = 1; i < substream_summaries_.size() && to_bool(hash_manager_.hash(i - 1, key)); i++) {
      substream_summaries_[i].update(key);
    }
  }

  heavy_hitters_set& get_heavy_hitters() {
    return substream_summaries_[0].get_heavy_hitters();
  }

  template<typename g_ret_t = counter_t>
  g_ret_t process_heavy_hitters(g_fn<g_ret_t> g) {
    g_ret_t recursive_sum = 0;

    // Handle last substream
    int substream_i = substream_summaries_.size() - 1;
    auto substream_hhs = substream_summaries_[substream_i].get_heavy_hitters();
    auto substream_sketch = substream_summaries_[substream_i].get_sketch();
    for (size_t hh_i = 0; hh_i < substream_hhs.size(); hh_i++) {
      T hh = atomic::load(&substream_hhs[hh_i]);
      if (hh != T()) {
        counter_t count = substream_sketch.estimate(hh);
        recursive_sum += g(count);
      }
    }

    substream_i--;
    while (substream_i >= 0) {

      g_ret_t substream_sum = 0;
      substream_hhs = substream_summaries_[substream_i].get_heavy_hitters();
      substream_sketch = substream_summaries_[substream_i].get_sketch();
      for (size_t hh_i = 0; hh_i < substream_hhs.size(); hh_i++) {
        T hh = atomic::load(&substream_hhs[hh_i]);
        if (hh != T()) {
          counter_t count = substream_sketch.estimate(hh);
          g_ret_t update = ((1 - 2 * hash_manager_.hash(substream_i, hh)) * g(count));
          substream_sum += update;
        }
      }

      recursive_sum = 2 * recursive_sum + substream_sum;
    }
    return 0;
  }

  /**
   * Get the number of substreams.
   * @return number of substreams
   */
  size_t num_substreams() {
    return substream_summaries_.size();
  }

  static universal_monitor<T, counter_t> create_parameterized(double gamma, double epsilon,
                                                              double hh_threshold, size_t num_heavy_hitters,
                                                              hash_manager<T>& monitor_hash_manager) {
    return universal_monitor<T, counter_t>(count_sketch<T, counter_t>::perror_to_num_estimates(gamma),
                                           count_sketch<T, counter_t>::error_margin_to_num_buckets(epsilon),
                                           hh_threshold, num_heavy_hitters, monitor_hash_manager);
  }

 private:
  static inline bool to_bool(size_t hashed_value) {
    return bool(hashed_value % 2);
  }

  atomic_counter_t l2_squared_; // L2 norm squared
  std::vector<substream_summary<T, counter_t>> substream_summaries_;
  hash_manager<T> hash_manager_;

};

}
}

#endif /* CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_ */
