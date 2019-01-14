#ifndef CONFLUO_CONTAINER_SKETCH_FREQUENCY_FUNCTIONS_H
#define CONFLUO_CONTAINER_SKETCH_FREQUENCY_FUNCTIONS_H

namespace confluo {
namespace sketch {

/**
 * These are monotonically increasing frequency-domain functions.
 * in f and O(f^2), where f is the frequency of the data point
 * @tparam counter_t type of input
 */
template<typename arg_t = int64_t>
class frequency_functions {

 public:
  template<typename ret_t> using fn = std::function<ret_t(arg_t)>;

  static arg_t l2_norm(arg_t freq) {
    return freq * freq;
  }

  static double entropy(arg_t freq) {
    return freq * std::log(freq);
  }

  static arg_t cardinality(arg_t freq) {
    return 1;
  }

};

}
}

#endif // CONFLUO_CONTAINER_SKETCH_FREQUENCY_FUNCTIONS_H
