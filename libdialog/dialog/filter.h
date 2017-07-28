#ifndef DIALOG_FILTER_H_
#define DIALOG_FILTER_H_

#include "schema.h"
#include "reflog.h"
#include "tiered_index.h"
#include "radix_tree.h"
#include "aggregated_reflog.h"
#include "compiled_expression.h"
#include "trigger.h"
#include "trigger_log.h"

// TODO: Update tests

using namespace ::utils;

namespace dialog {
namespace monitor {

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
  typedef index::radix_tree<aggregated_reflog> idx_t;
  typedef idx_t::rt_result range_result;

  // Default leaf time-granularity (1ms)
  static const size_t LEAF_RANGE_NS = 1e6;

  /**
   * Constructor that initializes filter with provided compiled expression and
   * filter function.
   *
   * @param exp Compiled expression.
   * @param fn Filter function.
   * @param monitor_granularity_ms Time-granularity (milliseconds) for monitor.
   */
  filter(const compiled_expression& exp, filter_fn fn = default_filter)
      : exp_(exp),
        fn_(fn),
        idx_(8, 256),
        is_valid_(true) {
  }

  /**
   * Constructor that initializes the filter function with the provided one.
   *
   * @param fn Provided filter function.
   * @param monitor_granularity_ms Time-granularity (milliseconds) for monitor.
   */
  filter(filter_fn fn = default_filter)
      : exp_(),
        fn_(fn),
        idx_(8, 256),
        is_valid_(true) {
  }

  /**
   * Add a trigger to the filter
   *
   * @param t Pointer to trigger
   * @return Trigger id.
   */
  size_t add_trigger(trigger* t) {
    return triggers_.push_back(t);
  }

  /**
   * Invalidate trigger identified by id.
   *
   * @param id ID for the trigger.
   * @return True if invalidation was successful, false otherwise.
   */
  bool remove_trigger(size_t id) {
    return triggers_.at(id)->invalidate();
  }

  trigger* get_trigger(size_t id) {
    return triggers_.at(id);
  }

  /**
   *
   *
   * @return
   */
  size_t num_triggers() {
    return triggers_.size();
  }

  /**
   * Convert given timestamp into time block (finest time granularity for the
   * filter)
   *
   * @param ts Given timestamp.
   * @return Corresponding time block.
   */
  static uint64_t get_ts_block(uint64_t ts) {
    return ts / LEAF_RANGE_NS;
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
    if (exp_.test(r) && fn_(r)) {
      aggregated_reflog* refs = idx_.insert(
          byte_string(get_ts_block(r.timestamp())), r.log_offset(), triggers_);
      int tid = thread_manager::get_id();
      for (size_t i = 0; i < refs->num_aggregates(); i++) {
        if (triggers_.at(i)->is_valid()) {
          size_t field_idx = triggers_.at(i)->field_idx();
          aggregate_id agg = triggers_.at(i)->agg_id();
          refs->update_aggregate(
              tid,
              i,
              agg == aggregate_id::D_CNT ?
                  numeric(static_cast<long>(1)) : r[field_idx].value(),
              r.version());
        }
      }
    }
  }

  /**
   * Get the RefLog corresponding to given time-block.
   *
   * @param ts_block Given time-block.
   * @return Corresponding RefLog.
   */
  aggregated_reflog const* lookup(uint64_t ts_block) const {
    return idx_.at(byte_string(ts_block));
  }

  /**
   * Get the range of offsets that lie in a given time-block.
   *
   * @param ts_block_begin Beginning time-block
   * @param ts_block_end End time-block.
   * @return An iterator over log offsets in the time range.
   */
  range_result lookup_range(uint64_t ts_block_begin,
                            uint64_t ts_block_end) const {
    return idx_.range_lookup(byte_string(ts_block_begin),
                             byte_string(ts_block_end));
  }

  bool invalidate() {
    bool expected = true;
    if (atomic::strong::cas(&is_valid_, &expected, false)) {
      return true;
    }
    return false;
  }

  bool is_valid() {
    return atomic::load(&is_valid_);
  }

 private:
  compiled_expression exp_;         // The compiled filter expression
  filter_fn fn_;                    // Filter function
  idx_t idx_;                       // The filtered data index
  trigger_log triggers_;
  atomic::type<bool> is_valid_;     // Is valid
};

}
}

#endif /* DIALOG_FILTER_H_ */
