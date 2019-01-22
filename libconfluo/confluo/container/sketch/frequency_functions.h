#ifndef CONFLUO_CONTAINER_SKETCH_FREQUENCY_FUNCTIONS_H
#define CONFLUO_CONTAINER_SKETCH_FREQUENCY_FUNCTIONS_H

#include <functional>
#include <map>

namespace confluo {
namespace sketch {

template <typename arg_t, typename ret_t> using fn = std::function<ret_t(arg_t)>;

/**
 * Encoding type of data pointed to.
 */
typedef enum frequency_metric {
  CARDINALITY = 0,
  FREQUENCY = 1,
  ENTROPY = 2,
  L2_NORM = 3
} frequency_metric;

/**
 * These are monotonically increasing frequency-domain functions.
 * in f and O(f^2), where f is the frequency of the data point
 * @tparam counter_t type of input
 */
template <typename arg_t = int64_t>
class frequency_functions {
  // TODO address possible overflows

 public:
  typedef fn<arg_t, double> fn_t;

  static double cardinality(arg_t freq) {
    return 1;
  }

  static double frequency(arg_t freq) {
    return freq;
  }

  static double entropy(arg_t freq) {
    return freq * std::log(freq);
  }

  static double l2_norm(arg_t freq) {
    return freq * freq;
  }

  static fn_t get(const frequency_metric &metric) {
    static std::map<frequency_metric, fn_t> fn_map = {
        {CARDINALITY, cardinality}, {FREQUENCY, frequency}, {ENTROPY, entropy}, {L2_NORM, l2_norm}
    };
    return fn_map[metric];
  }
};


}
}

#endif // CONFLUO_CONTAINER_SKETCH_FREQUENCY_FUNCTIONS_H
