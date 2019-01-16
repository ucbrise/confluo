#ifndef CONFLUO_FILTER_H_
#define CONFLUO_FILTER_H_

#include "aggregated_reflog.h"
#include "container/radix_tree.h"
#include "container/reflog.h"
#include "container/sketch/universal_sketch.h"
#include "parser/expression_compiler.h"
#include "schema/record_batch.h"
#include "schema/schema.h"
#include "trigger.h"
#include "trigger_log.h"
#include "univ_sketch_log.h"

// TODO: Update tests

using namespace ::utils;
using namespace ::confluo::parser;
using namespace ::confluo::sketch;

namespace confluo {
namespace monitor {

/**
 * Filter function type definition for filtering records.
 *
 * @param r The record
 * @return True if the record passes the filter, false otherwise.
 */
typedef bool (*filter_fn)(const record_t &r);

/**
 * Default filter function that allows all records to pass.
 *
 * @param r The record
 * @return True if the record passes the filter, false otherwise.
 */
inline bool default_filter(const record_t &r) {
  return true;
}

/**
 * Actual storage for the filter: stores references to data-points that pass
 * the filter in a time partitioned index.
 */
class filter {
 public:
  /** Type definition for time partitioned index; parameters ensure all
  nanosecond time-stamps can be handled by the index. */
  typedef index::radix_tree<aggregated_reflog> idx_t;
  /** The filter range result */
  typedef idx_t::rt_result range_result;
  typedef idx_t::rt_reflog_result reflog_result;

  /**
   * Constructor that initializes filter with provided compiled expression and
   * filter function.
   *
   * @param exp Compiled expression.
   * @param fn Filter function.
   */
  explicit filter(const compiled_expression &exp, filter_fn fn = default_filter);

  /**
   * Constructor that initializes the filter function with the provided one.
   *
   * @param fn Provided filter function.
   */
  explicit filter(filter_fn fn = default_filter);

  /**
   * Add an aggregate to the filter.
   *
   * @param a Reference to the aggregate.
   * @return Aggregate id.
   */
  size_t add_aggregate(aggregate_info *a);

  /**
   * Invalidate aggregate identified by id.
   *
   * @param id ID for the aggregate.
   * @return True if invalidation was successful, false otherwise.
   */
  bool remove_aggregate(size_t id);

  /**
   * Get the aggregate corresponding to id.
   *
   * @param id ID for the aggregate.
   * @return Reference to aggregate corresponding to id.
   */
  aggregate_info *get_aggregate_info(size_t id);

  /**
   * Get the number of aggregates associated with this filter.
   * This includes aggregates that have been invalidated.
   *
   * @return The number of aggregates associated with this filter.
   */
  size_t num_aggregates() const;

  /**
   * Adds a sketch to the filter.
   * @param sketch Pointer to the sketch to add.
   */
  size_t add_sketch(universal_sketch *sketch);

  /**
   * Removes a sketch from the filter.
   * @param id The ID of the sketch.
   * @return True if invalidation was successful, false otherwise.
   */
  bool remove_sketch(size_t id);

  /**
   * Gets the number of sketches associated with this filter.
   * This includes sketches that have been invalidated.
   *
   * @return The number of sketches associated with this filter.
   */
  size_t num_sketches() const;

  /**
   * Updates the filter index with a new data point. If the new data point
   * passes the filter, its reference is stored.
   *
   * @param r Record being tested.
   */
  void update(const record_t &r);

  /**
   * Updates the filter index with new data points. If data points
   * pass the filter, their references are stored.
   * @param log_offset The offset from the log
   * @param snap The snapshot of the schema
   * @param block The record block
   * @param record_size The size of the record
   */
  void update(size_t log_offset, const schema_snapshot &snap, record_block &block, size_t record_size);

  // TODO rename later
  /**
   * Get the RefLog corresponding to given time-block.
   *
   * @param ts_block Given time-block.
   * @return Corresponding RefLog.
   */
  aggregated_reflog *lookup_unsafe(uint64_t ts_block) const;

  /**
   * Get the RefLog corresponding to given time-block.
   *
   * @param ts_block Given time-block.
   * @return Corresponding RefLog.
   */
  aggregated_reflog const *lookup(uint64_t ts_block) const;

  /**
   * Get the range of offsets that lie in a given time-block.
   *
   * @param ts_block_begin Beginning time-block
   * @param ts_block_end End time-block.
   * @return An iterator over log offsets in the time range.
   */
  range_result lookup_range(uint64_t ts_block_begin, uint64_t ts_block_end) const;

  /**
   * Get the range of reflogs that lie between time-blocks.
   * @param ts_block_begin beginning time-block
   * @param ts_block_end end time-block
   * @return an iterator over reflogs in the time range
   */
  reflog_result lookup_range_reflogs(uint64_t ts_block_begin, uint64_t ts_block_end) const;

  /**
   * Estimates frequency of a key from a filter's sketch
   * @param id The ID of the sketch
   * @param key The string representation of the key to estimate
   * @return estimated frequency of key
   */
  int64_t estimate_frequency(size_t id, const std::string &key);

  /**
   * Gets the approximate heavy hitters associated with a filter's sketch
   * @param id The ID of the sketch
   * @return a map of heavy hitters to their estimated frequencies
   */
  universal_sketch::heavy_hitters_map_t get_heavy_hitters(size_t id);

  /**
   * Invalidates the filter expression
   *
   * @return True if the filter was succesfully validated, false otherwise
   */
  bool invalidate();

  /**
   * Gets whether the filter is valid
   *
   * @return True if the filter is valid, false otherwise
   */
  bool is_valid();

  /**
   * Note: It is dangerous to modify this data structure.
   * @return underlying radix tree
   */
  idx_t &data();

 private:
  compiled_expression exp_;         // The compiled filter expression
  filter_fn fn_;                    // Filter function
  idx_t idx_;                       // The filtered data index
  aggregate_log aggregates_;        // List of aggregates on this filter
  univ_sketch_log sketches_;        // List of universal sketches on this filter
  atomic::type<bool> is_valid_;     // Marks if the filter is valid or not
};

}
}

#endif /* CONFLUO_FILTER_H_ */
