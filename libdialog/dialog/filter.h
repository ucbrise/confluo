#ifndef DIALOG_FILTER_H_
#define DIALOG_FILTER_H_

#include "schema.h"
#include "tiered_index.h"
#include "compiled_expression.h"

// TODO: Update tests

using namespace ::utils;

namespace dialog {
namespace monitor {

/**
 * Type-definition for RefLog type -- a MonoLog of type uint64_t and
 * bucket size of 24.
 */
typedef monolog::monolog_exp2<uint64_t, 24> reflog;

/**
 * Filter function type definition for filtering records.
 *
 * @param r The record
 * @return True if the record passes the filter, false otherwise.
 */
typedef bool (*filter_fn)(const record_t& r);

/**
 * Default filter function that allows all records to pass.
 *
 * @param r The record
 * @return True if the record passes the filter, false otherwise.
 */
inline bool default_filter(const record_t& r) {
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
   * Constructor that initializes the filter functor with the provided one.
   * @param f Provided filter functor.
   * @param monitor_granularity_ms Time-granularity (milliseconds) for monitor.
   */
  filter(const compiled_expression& exp, filter_fn fn = default_filter,
         size_t monitor_granularity_ms = 1)
      : exp_(exp),
        fn_(fn),
        leaf_range_ns_(monitor_granularity_ms * 1e6) {
  }

  filter(filter_fn fn = default_filter, size_t monitor_granularity_ms = 1)
      : exp_(),
        fn_(fn),
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
  void update(const record_t& r) {
    if (exp_.test(r) && fn_(r))
      idx_[get_ts_block(r.timestamp())]->push_back(r.log_offset());
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
  idx_t idx_;                       // The filtered data index
  compiled_expression exp_;         // The compiled filter expression
  filter_fn fn_;                    // Filter function
  size_t leaf_range_ns_;            // Time-range (ns) covered by leaf node
};

}
}

#endif /* DIALOG_FILTER_H_ */
