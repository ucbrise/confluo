#ifndef CONFLUO_CONTAINER_SKETCH_CONFLUO_UNIVERSAL_SKETCH_H
#define CONFLUO_CONTAINER_SKETCH_CONFLUO_UNIVERSAL_SKETCH_H

#include <unordered_map>
#include <utility>
#include <vector>

#include "atomic.h"
#include "count_sketch.h"
#include "frequency_functions.h"
#include "../data_log.h"
#include "schema/column.h"
#include "schema/record.h"
#include "hash_manager.h"

namespace confluo {
namespace sketch {

class universal_sketch {

 public:
  typedef size_t key_t;
  typedef int64_t counter_t;
  typedef frequency_functions<counter_t> fns;
  typedef count_sketch<key_t, counter_t> sketch_t;
  typedef std::vector<atomic::type<size_t>> heavy_hitters_t;
  typedef std::unordered_map<std::string, size_t> heavy_hitters_map_t;

  // TODO replace constructors with config-based constructor

  /**
   * Constructor
   * @param epsilon epsilon
   * @param gamma gamma
   * @param k number of heavy hitters to track per layer
   * @param log data log
   * @param column column of field to sketch
   */
  universal_sketch(double epsilon, double gamma, size_t k, data_log *log, column_t column);

  /**
   * Constructor
   * @param l number of layers
   * @param b count-sketch width (number of buckets)
   * @param t count-sketch depth (number of estimates)
   * @param k number of heavy hitters to track per layer
   * @param log data log
   * @param column column of field to sketch
   */
  universal_sketch(size_t l, size_t b, size_t t, size_t k, data_log *log, column_t colum);

  universal_sketch(const universal_sketch &other);

  universal_sketch &operator=(const universal_sketch &other);

  bool is_valid();

  /**
   * Invalidates sketch
   */
  bool invalidate();

  /**
   * Updates universal sketch with a record using the relevant field(s)
   * @param r record
   * @param incr increment
   */
  void update(const record_t &r, size_t incr = 1);

  /**
   * Estimates the frequency of the key
   * @return estimated key frequency
   */
  int64_t estimate_frequency(const std::string &key);

  /**
   * Gets heavy hitters and their estimated frequencies
   * @param num_layers number of layers to use
   * @return heavy hitters
   */
  heavy_hitters_map_t get_heavy_hitters(size_t num_layers = 1);

  /**
   * Evaluates a function over the universal sketch using all layers
   * @tparam ret_t return type
   * @param f function
   * @return estimate of summary function
   */
  template<typename ret_t = counter_t>
  ret_t evaluate(fns::fn<ret_t> f) {
    return evaluate(f, num_layers_);
  }

  /**
   * Evaluates a function over the universal sketch
   * @tparam ret_t return type
   * @param f frequency-domain function bounded by O(f^2) where f is the frequency
   * @param num_layers number of layers to use to compute G_SUM
   * @return estimate of summary function
   */
  template<typename ret_t = counter_t>
  ret_t evaluate(fns::fn<ret_t> f, size_t num_layers) {
    ret_t recursive_sum = 0;
    size_t substream_i = num_layers - 1;

    // Handle last substream (base case)
    for (auto &last_substream_hh : substream_heavy_hitters_[substream_i]) {
      key_t key = atomic::load(&last_substream_hh);
      // Make sure a key exists in the slot
      if (key != zero()) {
        counter_t count = substream_sketches_[substream_i].estimate(key);
        recursive_sum += f(count);
      }
    }

    // Handle rest (recursive case)
    while (substream_i-- > 0) {
      ret_t substream_sum = 0;
      for (auto &substream_hh : substream_heavy_hitters_[substream_i]) {
        key_t key = atomic::load(&substream_hh);
        // Make sure a key exists in the slot
        if (key != zero()) {
          counter_t count = substream_sketches_[substream_i].estimate(key);
          ret_t update = ((1 - 2 * to_bool(substream_hashes_.hash(substream_i, key))) * f(count));
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
   * Converts record's relevant field value to an indexable key
   * @param r record
   * @return field value as an indexable key
   */
  inline key_t get_key_hash(const record_t &r);

  /**
   * Converts record's relevant field value to an indexable key
   * @param ptr pointer into data log where record is stored
   * @return field value as an indexable key
   */
  inline key_t get_key_hash(const read_only_data_log_ptr &ptr);

  /**
   * Converts string to an indexable key
   * @param str string to convert
   * @return field value as an indexable key
   */
  inline key_t str_to_key_hash(const std::string &str);

  /**
   * Converts record's relevant field value to a string
   * @param ptr pointer into data log where record is stored
   * @return field value as a string
   */
  inline std::string record_key_to_string(const read_only_data_log_ptr &ptr);

  /**
   * Updates the heavy hitters of a substream
   * @param idx index of substream
   * @param key_hash key
   * @param offset offset of record holding key into data log
   * @param count count of key
   */
  void update_heavy_hitters(size_t idx, key_t key_hash, size_t offset, counter_t count);

  static inline key_t to_bool(key_t hashed_value) {
    return hashed_value % 2;
  }

  static inline key_t zero() {
   static key_t zero = hash_util::hash(immutable_value());
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
