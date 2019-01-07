#ifndef CONFLUO_CONTAINER_SKETCH_CONFLUO_UNIVERSAL_SKETCH_H
#define CONFLUO_CONTAINER_SKETCH_CONFLUO_UNIVERSAL_SKETCH_H

#include <unordered_map>
#include <utility>
#include <vector>

#include "atomic.h"
#include "count_sketch.h"
#include "../data_log.h"
#include "../../schema/column.h"
#include "../../schema/record.h"
#include "hash_manager.h"

namespace confluo {
namespace sketch {

template<typename counter_t = int64_t>
class functions {

 public:
  // This is a frequency-domain function and must be monotonically increasing
  // in f and O(f^2), where f is the frequency of the data point
  template<typename g_ret_t> using g_fn = std::function<g_ret_t(counter_t)>;

  static counter_t l2_norm(counter_t freq) {
   return freq * freq;
  }

  static double entropy(counter_t freq) {
    return freq * std::log(freq);
  }

  static counter_t cardinality(counter_t freq) {
    return 1;
  }

};

class confluo_universal_sketch {

public:
  typedef size_t key_t;
  typedef int64_t counter_t;
  typedef functions<counter_t> fn_t;
  typedef count_sketch<key_t, counter_t> sketch_t;
  typedef std::vector<atomic::type<size_t>> heavy_hitters_t;

  /**
   * Constructor
   * @param epsilon epsilon
   * @param gamma gamma
   * @param k number of heavy hitters to track per layer
   * @param log data log
   * @param column column of field to sketch
   */
  confluo_universal_sketch(double epsilon, double gamma, size_t k, data_log *log, column_t column);

  /**
   * Constructor
   * @param l number of layers
   * @param b count-sketch width (number of buckets)
   * @param t count-sketch depth (number of estimates)
   * @param k number of heavy hitters to track per layer
   * @param log data log
   * @param column column of field to sketch
   */
  confluo_universal_sketch(size_t l, size_t b, size_t t, size_t k, data_log *log, column_t column);

  confluo_universal_sketch(const confluo_universal_sketch &other);

  confluo_universal_sketch &operator=(const confluo_universal_sketch &other);

  bool is_valid();

  /**
   * Invalidate sketch
   */
  bool invalidate();

  /**
   * Update universal sketch with a record.
   * @param r record
   */
  void update(const record_t &r);

  /**
   * Get heavy hitters and their estimated frequencies from the top layer
   * @param num_layers number of layers to use to refine count
   * @return heavy hitters
   */
  std::unordered_map<std::string, counter_t> get_heavy_hitters();

  /**
   * Evaluate a G_SUM function using all layers
   * @tparam g_ret_t return type
   * @param g function
   * @return g sum estimate
   */
  template<typename g_ret_t = counter_t>
  g_ret_t evaluate(fn_t::g_fn<g_ret_t> g) {
    return evaluate(g, num_layers_);
  }

  /**
   * Evaluate a G_SUM function
   * @tparam g_ret_t return type
   * @param g function bounded by O(f^2) where f is the frequency
   * @param num_layers number of layers to use to compute G_SUM
   * @return g sum
   */
  template<typename g_ret_t = counter_t>
  g_ret_t evaluate(fn_t::g_fn<g_ret_t> g, size_t num_layers) {
    g_ret_t recursive_sum = 0;
    size_t substream_i = num_layers - 1;

    // Handle last substream (base case)
    for (auto &last_substream_hh : substream_heavy_hitters_[substream_i]) {
      key_t key = atomic::load(&last_substream_hh);
      // Make sure a key exists in the slot
      if (key != zero()) {
        counter_t count = substream_sketches_[substream_i].estimate(key);
        recursive_sum += g(count);
      }
    }

    // Handle rest (recursive case)
    while (substream_i-- > 0) {
      g_ret_t substream_sum = 0;
      for (auto &substream_hh : substream_heavy_hitters_[substream_i]) {
        key_t key = atomic::load(&substream_hh);
        // Make sure a key exists in the slot
        if (key != zero()) {
          counter_t count = substream_sketches_[substream_i].estimate(key);
          g_ret_t update = ((1 - 2 * to_bool(substream_hashes_.hash(substream_i, key))) * g(count));
          substream_sum += update;
        }
      }
      recursive_sum = 2 * recursive_sum + substream_sum;
    }

    return recursive_sum;
  }

  /**
   * Size of data structure's first n layers
   * @param num_layers number of layers
   * @return size of data structure's first n layers in bytes
   */
  size_t storage_size();

private:
  /**
   * Convert record's relevant field value to an indexable key
   * @param r record
   * @return field value as an indexable key
   */
  inline key_t record_to_key(const record_t &r);

  /**
   * Convert record's relevant field value to an indexable key
   * @param ptr pointer into data log where record is stored
   * @return field value as an indexable key
   */
  inline key_t record_ptr_to_key(const read_only_data_log_ptr &ptr);

  /**
   * Convert record's relevant field value to a string
   * @param ptr pointer into data log where record is stored
   * @return field value as a string
   */
  inline std::string record_ptr_to_string(const read_only_data_log_ptr &ptr);

  /**
   * Update the heavy hitters of a substream
   * @param idx index of substream
   * @param key key
   * @param offset offset of record holding key into data log
   * @param count count of key
   */
  void update_heavy_hitters(size_t idx, key_t key, size_t offset, counter_t count);

  static inline key_t to_bool(key_t hashed_value) {
    return hashed_value % 2;
  }

  static inline key_t zero() {
   static key_t zero = byte_string().hash();
   return zero;
  }

  std::vector<sketch_t> substream_sketches_;
  std::vector<heavy_hitters_t> substream_heavy_hitters_;
  hash_manager substream_hashes_;
  pairwise_indep_hash hh_hash_;
  size_t num_layers_;

  data_log *data_log_;
  column_t column_;
  atomic::type<bool> is_valid_;

};

}
}

#endif //CONFLUO_CONTAINER_SKETCH_CONFLUO_UNIVERSAL_SKETCH_H
