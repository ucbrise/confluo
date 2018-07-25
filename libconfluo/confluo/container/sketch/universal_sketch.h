#ifndef CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_
#define CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_

#include <vector>

#include "atomic.h"
#include "count_sketch.h"
#include "hash_manager.h"
#include "container/sketch/substream_summary.h"

namespace confluo {
namespace sketch {

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
   * @param precise track exact heavy hitters
   */
  universal_sketch(size_t t, size_t b, size_t k, double a)
      : universal_sketch(8 * sizeof(T), t, b, k, a) {
  }

  /**
   * Constructor
   * @param l number of layers
   * @param t count-sketch depth (number of estimates)
   * @param b count-sketch width (number of buckets)
   * @param k number of heavy hitters to track per layer
   * @param a heavy hitter threshold
   * @param precise track exact heavy hitters
   */
  universal_sketch(size_t l, size_t t, size_t b, size_t k, double a, bool precise = true)
      : substream_summaries_(l),
        layer_hashes_(l - 1),
        precise_hh_(precise) {
    layer_hashes_.guarantee_initialized(l - 1);
    for (size_t i = 0; i < l; i++) {
      substream_summaries_[i] = substream_summary<T, counter_t>(t, b, k, a, precise);
    }
  }

  universal_sketch(const universal_sketch& other)
      : substream_summaries_(other.substream_summaries_),
        layer_hashes_(other.layer_hashes_),
        precise_hh_(other.precise_hh_) {
  }

  universal_sketch& operator=(const universal_sketch& other) {
    substream_summaries_ = other.substream_summaries_;
    layer_hashes_ = other.layer_hashes_;
    precise_hh_ = other.precise_hh_;
    return *this;
  }

  /**
   * Update universal sketch with an element.
   * @param key key
   */
  void update(T key, size_t count = 1) {
    substream_summaries_[0].update(key, count);
    for (size_t i = 1; i < substream_summaries_.size() && to_bool(layer_hashes_.hash(i - 1, key)); i++) {
      substream_summaries_[i].update(key, count);
    }
  }

  /**
   * Estimate count of an individual key.
   * @param key key
   * @return estimated count from most accurate layer
   */
  counter_t estimate_count(T key) {
    counter_t est = substream_summaries_[0].estimate(key);
    // Refine count using lower layers.
    for (size_t i = 1; i < substream_summaries_.size() && to_bool(layer_hashes_.hash(i - 1, key)) == 1; i++) {
      est = substream_summaries_[i].estimate(key);
    }
    return est;
  }
  /**
   * Evaluate a G_SUM function using all layers
   * @tparam g_ret_t return type
   * @param g function
   * @return g sum estimate
   */
  template<typename g_ret_t = counter_t>
  g_ret_t evaluate(g_fn<g_ret_t> g) {
    return evaluate(g, substream_summaries_.size());
  }

  /**
   * Evaluate a G_SUM function
   * @tparam g_ret_t return type
   * @param g function
   * @param nlayers number of layers to use
   * @return g sum
   */
  template<typename g_ret_t = counter_t>
  g_ret_t evaluate(g_fn<g_ret_t> g, size_t nlayers) {
    g_ret_t recursive_sum = 0;

    // Handle last substream
    size_t substream_i = nlayers - 1;

    if (precise_hh_) {
      auto& last_substream_hhs = substream_summaries_[substream_i].get_pq();
      for (auto it = last_substream_hhs.begin(); it != last_substream_hhs.end(); ++it) {
        counter_t count = (*it).priority_;
        recursive_sum += g(count);
      }
    }
    else {
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
    }

    // Handle rest recursively

    while (substream_i-- > 0) {

      g_ret_t substream_sum = 0;

      if (precise_hh_) {
        auto& substream_hhs = substream_summaries_[substream_i].get_pq();
        for (auto it = substream_hhs.begin(); it != substream_hhs.end(); ++it) {
          T hh = (*it).key_;
          counter_t count = (*it).priority_;
          g_ret_t update = ((1 - 2 * to_bool(layer_hashes_.hash(substream_i, hh))) * g(count));
          substream_sum += update;
        }
      }
      else {
        auto &substream_hhs = substream_summaries_[substream_i].get_heavy_hitters();
        auto& substream_sketch = substream_summaries_[substream_i].get_sketch();
        for (size_t hh_i = 0; hh_i < substream_hhs.size(); hh_i++) {
          T hh = atomic::load(&substream_hhs[hh_i]);
          counter_t count = substream_sketch.estimate(hh);
          // TODO handle special case
          if (hh != T()) {
            g_ret_t update = ((1 - 2 * to_bool(layer_hashes_.hash(substream_i, hh))) * g(count));
            substream_sum += update;
          }
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

  static universal_sketch<T, counter_t> create_parameterized(double epsilon, double gamma, size_t k, double a) {
    return {
      count_sketch<T, counter_t>::error_margin_to_width(epsilon),
      count_sketch<T, counter_t>::perror_to_depth(gamma),
      k, a
    };
  }

 private:
  static inline counter_t to_bool(size_t hashed_value) {
    return hashed_value % 2;
  }

  std::vector<substream_summary<T, counter_t>> substream_summaries_;
  hash_manager layer_hashes_;

  bool precise_hh_;

};

}
}

#endif /* CONFLUO_SKETCH_UNIVERSAL_MONITOR_H_ */
