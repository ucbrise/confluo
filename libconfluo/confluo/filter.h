#ifndef CONFLUO_FILTER_H_
#define CONFLUO_FILTER_H_

#include "aggregated_reflog.h"
#include "container/radix_tree.h"
#include "container/reflog.h"
#include "trigger.h"
#include "trigger_log.h"
#include "parser/expression_compiler.h"
#include "schema/record_batch.h"
#include "schema/schema.h"

// TODO: Update tests

using namespace ::utils;
using namespace ::confluo::parser;

namespace confluo {
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
   * Add an aggregate to the filter.
   *
   * @param a Reference to the aggregate.
   * @return Aggregate id.
   */
  size_t add_aggregate(aggregate_info* a) {
    return aggregates_.push_back(a);
  }

  /**
   * Invalidate aggregate identified by id.
   *
   * @param id for the aggregate.
   * @return True if invalidation was successful, false otherwise.
   */
  bool remove_aggregate(size_t id) {
    return aggregates_.at(id)->invalidate();
  }

  /**
   * Get the aggregate corresponding to id.
   *
   * @param id for the aggregate.
   * @return Reference to aggregate corresponding to id.
   */
  aggregate_info* get_aggregate(size_t id) {
    return aggregates_.at(id);
  }

  /**
   * Get the number of aggregates associated with this filter.
   *
   * @return The number of aggregates associated with this filter.
   */
  size_t num_aggregates() const {
    return aggregates_.size();
  }

  /**
   * Updates the filter index with a new data point. If the new data point
   * passes the filter, its reference is stored.
   *
   * @param r Record being tested.
   */
  void update(const record_t& r) {
    if (exp_.test(r) && fn_(r)) {
      aggregated_reflog* refs = idx_.insert(
          byte_string(r.timestamp() / configuration_params::TIME_RESOLUTION_NS),
          r.log_offset(), aggregates_);
      int tid = thread_manager::get_id();
      for (size_t i = 0; i < refs->num_aggregates(); i++) {
        if (aggregates_.at(i)->is_valid()) {
          size_t field_idx = aggregates_.at(i)->field_idx();
          aggregate_type agg = aggregates_.at(i)->agg_type();
          numeric val =
              agg == aggregate_type::D_CNT ?
                  numeric(INT64_C(1)) : numeric(r[field_idx].value());
          refs->update_aggregate(tid, i, val, r.version());
        }
      }
    }
  }

  /**
   * Updates the filter index with new data points. If data points
   * pass the filter, their references are stored.
   */
  void update(size_t log_offset, const schema_snapshot& snap,
              record_block& block, size_t record_size) {
    int tid = thread_manager::get_id();
    aggregated_reflog* refs = nullptr;
    std::vector<numeric> local_aggs;

    for (size_t i = 0; i < block.nrecords; i++) {
      void* cur_rec = reinterpret_cast<uint8_t*>(&block.data[i * record_size]);
      uint64_t rec_off = log_offset + i * record_size;
      if (exp_.test(snap, cur_rec)) {
        if (refs == nullptr) {
          refs = idx_.get_or_create(
              byte_string(static_cast<uint64_t>(block.time_block)),
              aggregates_);
          local_aggs.resize(refs->num_aggregates());
        }
        refs->push_back(rec_off);
        for (size_t j = 0; j < local_aggs.size(); j++)
          if (aggregates_.at(j)->is_valid())
            local_aggs[j] = aggregates_.at(j)->agg(local_aggs[j], snap,
                                                   cur_rec);
      }
    }

    size_t version = log_offset + block.nrecords * record_size;
    for (size_t j = 0; j < local_aggs.size(); j++)
      if (aggregates_.at(j)->is_valid() && !local_aggs[j].type().is_none())
        refs->update_aggregate(tid, j, local_aggs[j], version);
  }

  /**
   * Get the RefLog corresponding to given time-block.
   *
   * @param ts_block Given time-block.
   * @return Corresponding RefLog.
   */
  aggregated_reflog const* lookup(uint64_t ts_block) const {
    return idx_.get(byte_string(ts_block));
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
  aggregate_log aggregates_;        // List of aggregates on this filter
  atomic::type<bool> is_valid_;     // Marks if the filter is valid or not
};

}
}

#endif /* CONFLUO_FILTER_H_ */
