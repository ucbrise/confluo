#ifndef DATASTORE_LOG_STREAM_H_
#define DATASTORE_LOG_STREAM_H_

#include "monolog.h"

namespace dialog {
namespace filter {

/**
 * Type-definition for RefLog type -- a MonoLog of type uint64_t and
 * bucket size of 24.
 */
typedef monolog::monolog_relaxed<uint64_t, 24> reflog_t;

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
class reflog {
 public:
  // Default filter used to initialize filter functor if one is not provided
  // in the constructor.
  static filter DEFAULT_FILTER;

  /**
   * Default constructor.
   */
  reflog()
      : filter_(DEFAULT_FILTER) {
  }

  /**
   * Constructor that initializes the filter functor with the provided one.
   * @param f Provided filter functor.
   */
  reflog(filter& f)
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
  const reflog_t& cstream() const {
    return rlog_;
  }

  /**
   * Returns a reference to the underlying MonoLog.
   *
   * @return A reference to the underlying MonoLog.
   */
  reflog_t& stream() {
    return rlog_;
  }

 private:
  reflog_t rlog_;   // The underlying MonoLog
  filter& filter_;  // The filter functor
};

// Initializes the default filter functor.
template<typename filter>
filter reflog<filter>::DEFAULT_FILTER = filter();

}
}

#endif /* DATASTORE_LOG_STREAM_H_ */
