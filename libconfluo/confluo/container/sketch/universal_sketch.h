#ifndef CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_
#define CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_

#include <vector>

#include "atomic.h"
#include "count_sketch.h"
#include "hash_manager.h"

namespace confluo {
namespace sketch {

template<typename T, typename counter_t = int64_t>
class substream_summary {

 public:
  typedef atomic::type<counter_t> atomic_counter_t;
  typedef std::vector<atomic::type<T>> heavy_hitters_set;
  typedef count_sketch<T, counter_t> sketch;

  substream_summary() = default;

  /**
   * Constructor
   * @param t depth (number of estimates)
   * @param b width (number of buckets)
   * @param k number of heavy hitters to track
   * @param a heavy hitter threshold
   */
  substream_summary(size_t t, size_t b, size_t k, double a)
      : hh_threshold_(a),
        num_hh_(k),
        l2_squared_(),
        sketch_(t, b),
        heavy_hitters_(k),
        hh_hash_(pairwise_indep_hash<T>::generate_random()) {
  }

  substream_summary(const substream_summary& other)
      : hh_threshold_(other.hh_threshold_),
        num_hh_(other.num_hh_),
        l2_squared_(atomic::load(&other.l2_squared_)),
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
    l2_squared_ = atomic::load(&other.l2_squared_);
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
    counter_t old_l2_sq = atomic::faa(&l2_squared_, update);
    double new_l2 = std::sqrt(old_l2_sq + update);
    this->update_heavy_hitters(key, old_count + 1, new_l2);
  }

  heavy_hitters_set& get_heavy_hitters() {
    return heavy_hitters_;
  }

  sketch& get_sketch() {
    return sketch_;
  }

  /**
   * @return size of data structure in bytes
   */
  size_t storage_size() {
    size_t total_size = 0;
    total_size += sketch_.storage_size();
    total_size += heavy_hitters_.size();
    return total_size;
  }

 private:

  void update_heavy_hitters(T key, counter_t count, double l2) {
    if (count < hh_threshold_ * l2) {
      return;
    }
    bool done = false;
    while (!done) {
      size_t idx = hh_hash_.apply(key) % heavy_hitters_.size();
      T prev = atomic::load(&heavy_hitters_[idx]);
      if (prev == key)
        return;
      counter_t prev_count = sketch_.estimate(prev);
      done = (prev_count > count) ? true : atomic::strong::cas(&heavy_hitters_[idx], &prev, key);
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

  atomic_counter_t l2_squared_; // L2 norm squared
  sketch sketch_;
  heavy_hitters_set heavy_hitters_;
  pairwise_indep_hash<T> hh_hash_;

};

template<typename T, typename counter_t = int64_t>
class universal_sketch {

 public:
  typedef std::vector<atomic::type<T>> heavy_hitters_set;
  template<typename g_ret_t> using g_fn = std::function<g_ret_t(counter_t)>;

  /**
   * Constructor
   * @param t count-sketch depth (number of estimates)
   * @param b count-sketch width (number of buckets)
   * @param k number of heavy hitters to track per layer
   * @param a heavy hitter threshold
   * @param layer_hashes hash manager for layers
   */
  universal_sketch(size_t t, size_t b, size_t k, double a, hash_manager<T>& layer_hashes)
      : universal_sketch(8 * sizeof(T), t, b, a, k, layer_hashes) {
  }

  /**
   * Constructor
   * @param l number of layers
   * @param t count-sketch depth (number of estimates)
   * @param b count-sketch width (number of buckets)
   * @param k number of heavy hitters to track per layer
   * @param a heavy hitter threshold
   * @param layer_hashes hash manager for layers
   */
  universal_sketch(size_t l, size_t t, size_t b, size_t k, double a, const hash_manager<T>& layer_hashes)
      : substream_summaries_(l),
        layer_hashes_(layer_hashes) {
    layer_hashes_.guarantee_initialized(l - 1);
    for (size_t i = 0; i < l; i++) {
      substream_summaries_[i] = substream_summary<T, counter_t>(t, b, k, a);
    }
  }

  universal_sketch(const universal_sketch& other)
      : substream_summaries_(other.substream_summaries_),
        layer_hashes_(other.layer_hashes_) {
  }

  universal_sketch& operator=(const universal_sketch& other) {
    substream_summaries_ = other.substream_summaries_;
    layer_hashes_ = other.layer_hashes_;
    return *this;
  }

  /**
   * Update universal sketch with an element.
   * @param elem element
   */
  void update(T key) {
    substream_summaries_[0].update(key);
    for (size_t i = 1; i < substream_summaries_.size() && to_bool(layer_hashes_.hash(i - 1, key)); i++) {
      substream_summaries_[i].update(key);
    }
  }

  counter_t estimate_count(T key) {
    counter_t est = substream_summaries_[0].estimate(key);
    // Refine count using lower layers.
    for (size_t i = 1; i < substream_summaries_.size() && to_bool(layer_hashes_.hash(i - 1, key)); i++) {
      est = substream_summaries_[i].estimate(key);
    }
    return est;
  }

  template<typename g_ret_t = counter_t>
  g_ret_t evaluate(g_fn<g_ret_t> g) {
    g_ret_t recursive_sum = 0;

    // Handle last substream
    size_t substream_i = substream_summaries_.size() - 1;

    auto& last_substream_hhs = substream_summaries_[substream_i].get_heavy_hitters();
    auto& last_substream_sketch = substream_summaries_[substream_i].get_sketch();
    for (size_t hh_i = 0; hh_i < last_substream_hhs.size(); hh_i++) {
      T hh = atomic::load(&last_substream_hhs[hh_i]);
      // TODO handle special case
      if (hh != T()) {
        counter_t count = last_substream_sketch.estimate(hh);
        recursive_sum += g(count);
      }
    }

    while (substream_i-- > 0) {

      g_ret_t substream_sum = 0;
      auto& substream_hhs = substream_summaries_[substream_i].get_heavy_hitters();
      auto& substream_sketch = substream_summaries_[substream_i].get_sketch();
      for (size_t hh_i = 0; hh_i < substream_hhs.size(); hh_i++) {
        T hh = atomic::load(&substream_hhs[hh_i]);
        // TODO handle special case
        if (hh != T()) {
          counter_t count = substream_sketch.estimate(hh);
          g_ret_t update = ((1 - 2 * (layer_hashes_.hash(substream_i, hh) % 2)) * g(count));
          substream_sum += update;
        }
      }

      recursive_sum = 2 * recursive_sum + substream_sum;
    }
    return recursive_sum;
  }

  /**
   * @return size of data structure in bytes
   */
  size_t storage_size() {
    size_t total_size = 0;
    for (size_t i = 0; i < substream_summaries_.size(); i++) {
     total_size += substream_summaries_[i].storage_size();
    }
    return total_size;
  }

  static universal_sketch<T, counter_t> create_parameterized(double gamma, double epsilon,
                                                              double hh_threshold, size_t num_heavy_hitters,
                                                              hash_manager<T>& layer_hashes) {
    return universal_sketch<T, counter_t>(count_sketch<T, counter_t>::perror_to_num_estimates(gamma),
                                           count_sketch<T, counter_t>::error_margin_to_num_buckets(epsilon),
                                           hh_threshold, num_heavy_hitters, layer_hashes);
  }

 private:
  static inline bool to_bool(size_t hashed_value) {
    return bool(hashed_value % 2);
  }

  std::vector<substream_summary<T, counter_t>> substream_summaries_;
  hash_manager<T> layer_hashes_;

};

}
}

#endif /* CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_ */