#ifndef DIALOG_MONITOR_H_
#define DIALOG_MONITOR_H_

#include "tiered_index.h"

namespace dialog {

namespace monitor {

/**
 * Type-definition for RefLog type -- a MonoLog of type uint64_t and
 * bucket size of 24.
 */
typedef monolog::monolog_exp2<uint64_t, 24> reflog;

/**
 * The default filter functor: allows all data to pass.
 */
struct default_filter {
  /**
   * Functor operator to filter data points. Default functor always returns
   * true.
   *
   * @param id Identifier for data point.
   * @param buf Binary data for the data point.
   * @param len Length of data in buf.
   * @return True if the data point passes the filter, false otherwise.
   */
  bool operator()(uint64_t id, uint8_t* buf, size_t len) {
    return true;
  }

  /**
   * Templated functor operator to filter data points. Default functor always
   * returns true.
   *
   * @param id Identifier for data point.
   * @param data The actual data point.
   * @return
   */
  template<typename T>
  bool operator()(uint64_t id, const T& data) {
    return true;
  }
};

/**
 * Actual storage for the RefLog: stores references to data-points that pass
 * the filter specific to the RefLog.
 */
template<typename filter = default_filter>
class filter_log {
 public:
  // Default filter used to initialize filter functor if one is not provided
  // in the constructor.
  static filter DEFAULT_FILTER;

  /**
   * Default constructor.
   */
  filter_log()
      : filter_(DEFAULT_FILTER) {
  }

  /**
   * Constructor that initializes the filter functor with the provided one.
   * @param f Provided filter functor.
   */
  filter_log(filter& f)
      : filter_(f) {
  }

  /**
   * Updates the RefLog with a new data point. If the new data point passes
   * the filter, its reference will be stored.
   *
   * @param id Identifier of the data point.
   * @param buf Binary data for the data point.
   * @param len Length of data in buf.
   */
  void update(uint64_t id, uint8_t* buf, size_t len) {
    if (filter_(id, buf, len))
      rlog_.push_back(id);
  }

  /**
   * Updates the RefLog with a new data point (with templated type). If the new
   * data point passes the filter, its reference will be stored.
   *
   * @param id Identifier for data point.
   * @param data The actual data point.
   */
  template<typename T>
  void update(uint64_t id, const T& data) {
    if (filter_(id, data))
      rlog_.push_back(id);
  }

  /**
   * Returns a constant reference to the underlying MonoLog.
   *
   * @return A constant reference to the underlying MonoLog.
   */
  const reflog& cstream() const {
    return rlog_;
  }

  /**
   * Returns a reference to the underlying MonoLog.
   *
   * @return A reference to the underlying MonoLog.
   */
  reflog& stream() {
    return rlog_;
  }

 private:
  reflog rlog_;   // The underlying RefLog
  filter& filter_;  // The filter functor
};

// Initializes the default filter functor.
template<typename filter>
filter filter_log<filter>::DEFAULT_FILTER = filter();

}
}

#endif /* DIALOG_MONITOR_H_ */
