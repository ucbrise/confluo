#ifndef DIALOG_FILTER_H_
#define DIALOG_FILTER_H_

#include "tiered_index.h"
#include "attributes.h"

// TODO: Update tests

using namespace ::utils;

namespace dialog {

namespace filter {

struct filter_info {
  uint32_t filter_id;
  const char* filter_expr;
};

/**
 * Type-definition for RefLog type -- a MonoLog of type uint64_t and
 * bucket size of 24.
 */
typedef monolog::monolog_exp2<uint64_t, 24> reflog;

/**
 * Filter function type definition for filtering data points.
 *
 * @param ts Timestamp associated with data point.
 * @param buf Binary data for the data point.
 * @param len Length of data in buf.
 * @param attrs Attributes associated with the data point.
 * @return True if the data point passes the filter, false otherwise.
 */
typedef bool (*filter_fn)(uint64_t ts, uint8_t* buf, size_t len,
                          attribute_list& list);

/**
 * Default filter function that allows all data points to pass.
 *
 * @param ts Timestamp associated with data point.
 * @param buf Binary data for the data point.
 * @param len Length of data in buf.
 * @param attrs Attributes associated with the data point.
 * @return True if the data point passes the filter, false otherwise.
 */
bool default_filter(uint64_t ts, uint8_t* buf, size_t len,
                    attribute_list& list) {
  return true;
}

/**
 * Actual storage for the filter: stores references to data-points that pass
 * the filter in a time partitioned index.
 */
class filter {
 public:
  // Type definition for time partitioned index; parameters ensure all
  // nanosecond time-stamps can be handled by the index.
  typedef index::tiered_index<reflog, 1000, 5> idx_t;

  // Default leaf time-granularity (1ms)
  static const size_t DEFAULT_LEAF_RANGE_NS = 1e6;

  /**
   * Default constructor.
   */
  filter()
      : filter_(default_filter),
        leaf_range_ns_(DEFAULT_LEAF_RANGE_NS) {
  }

  /**
   * Constructor that initializes the filter functor with the provided one.
   * @param f Provided filter functor.
   * @param monitor_granularity_ms Time-granularity (milliseconds) for monitor.
   */
  filter(filter_fn f, size_t monitor_granularity_ms)
      : filter_(f),
        leaf_range_ns_(monitor_granularity_ms * 1e6) {
  }

  /**
   * Convert given timestamp into time block (finest time granularity for the
   * filter)
   *
   * @param ts Given timestamp.
   * @return Corresponding time block.
   */
  uint64_t get_ts_block(uint64_t ts) {
    return ts / leaf_range_ns_;
  }

  /**
   * Updates the filter index with a new data point. If the new data point
   * passes the filter, its reference will be stored.
   *
   * @param ts Timestamp associated with the data point.
   * @param buf Binary data for the data point.
   * @param len Length of data in buf.
   * @param attrs List of attributes associated with the data point.
   */
  void update(uint64_t offset, uint64_t ts, uint8_t* buf, size_t len,
              attribute_list& attrs) {
    if (filter_(ts, buf, len, attrs))
      idx_[get_ts_block(ts)]->push_back(offset);
  }

  /**
   * Get the RefLog corresponding to given time-block.
   *
   * @param ts_block Given time-block.
   * @return Corresponding RefLog.
   */
  reflog* get(uint64_t ts_block) const {
    return idx_.at(ts_block);
  }

 private:
  idx_t idx_;               // The filtered data index
  filter_fn filter_;  // The filter functor
  size_t leaf_range_ns_;    // Time-range (nanoseconds) covered by leaf node
};

}
}

#endif /* DIALOG_FILTER_H_ */
