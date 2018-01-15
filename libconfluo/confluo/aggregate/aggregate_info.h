#ifndef CONFLUO_AGGREGATE_INFO_H_
#define CONFLUO_AGGREGATE_INFO_H_

#include "types/data_type.h"
#include "parser/aggregate_parser.h"
#include "trigger_log.h"

using namespace confluo::monitor;

namespace confluo {

/**
* @brief Information about the aggregate
*/
class aggregate_info {
 public:
  /**
   * Constructor to initialize aggregate_info
   *
   * @param name The name of the aggregate.
   * @param agg The aggregator.
   * @param field_type Type of field.
   * @param field_idx Index of field in schema.
   */
  aggregate_info(const std::string& name, aggregator agg, uint16_t field_idx)
      : name_(name),
        agg_(agg),
        field_idx_(field_idx),
        is_valid_(true) {
  }

  /**
   * Get the aggregate name
   * @return Aggregate name.
   */
  const std::string name() const {
    return name_;
  }

  /**
   * Gets the result data type for the aggregate.
   * @return The result data type for the aggregate.
   */
  data_type result_type() const {
    return agg_.result_type;
  }

  /**
   * Gets the field index
   * @return The field index
   */
  uint16_t field_idx() const {
    return field_idx_;
  }

  /**
   * Parses a value with the aggregate data type.
   * @param str String to parse from.
   * @return Numeric value with the aggregate's data type.
   */
  numeric value(const std::string& str) {
    return numeric::parse(str, result_type());
  }

  /**
   * Aggregates the data using the combine operator.
   * @param a The numeric
   * @param s The snapshot of the schema
   * @param data The data
   * @return An aggregate of the data based on the numeric
   */
  numeric comb_op(const numeric& a, const schema_snapshot& s, void* data) {
    numeric b(s.get(data, field_idx_));
    return comb_op(a, b);
  }

  /**
   * Aggregates the data using the combine operator.
   * @param a A numeric
   * @param b Another numeric
   * @return An aggregate of the two numerics
   */
  numeric comb_op(const numeric& a, const numeric& b) {
    return agg_.comb_op(a.is_valid() ? a : zero(), b);
  }

  /**
   * Aggregates the data using the combine operator.
   * @param a The numeric
   * @param s The snapshot of the schema
   * @param data The data
   * @return An aggregate of the data based on the numeric
   */
  numeric seq_op(const numeric& a, const schema_snapshot& s, void* data) {
    numeric b(s.get(data, field_idx_));
    return seq_op(a, b);
  }

  /**
   * Aggregates the data using the combine operator.
   * @param a A numeric
   * @param b Another numeric
   * @return An aggregate of the two numerics
   */
  numeric seq_op(const numeric& a, const numeric& b) {
    return agg_.seq_op(a.is_valid() ? a : zero(), b);
  }

  /**
   * Returns the zero value for the aggregate
   * @return Zero value for the aggregate
   */
  numeric zero() {
    return agg_.zero;
  }

  /**
   * Creates an aggregate using the parsed aggregate information
   *
   * @return The aggregate
   */
  aggregate create_aggregate() const {
    return aggregate(result_type(), agg_);
  }

  /**
   * Invalidates the aggregate
   * @return Whether aggregate was successfully invalidated
   */
  bool invalidate() {
    bool expected = true;
    if (atomic::strong::cas(&is_valid_, &expected, false)) {
      return true;
    }
    return false;
  }

  /**
   * Determines whether the aggregate is valid
   * @return Whether the aggregate is valid
   */
  bool is_valid() const {
    return atomic::load(&is_valid_);
  }

  /**
   * Add a trigger on the aggregate.
   * @param t The trigger to be added.
   * @return Trigger ID.
   */
  size_t add_trigger(trigger* t) {
    return triggers_.push_back(t);
  }

  /**
   * Remove a trigger from the aggregate.
   * @param id The trigger ID.
   * @return True if the trigger was successfully removed, false otherwise.
   */
  bool remove_trigger(size_t id) {
    return triggers_.at(id)->invalidate();
  }

  /**
   * Get the trigger corresponding to a given id.
   * @param id The trigger ID.
   * @return The trigger corresponding to the id.
   */
  trigger* get_trigger(size_t id) {
    return triggers_.at(id);
  }

  /**
   * Get the number of triggers on this aggregate (includes invalid triggers)
   * @return The number of triggers on this aggregate.
   */
  size_t num_triggers() const {
    return triggers_.size();
  }

 private:
  std::string name_;

  // Parsed information
  aggregator agg_;
  uint16_t field_idx_;

  // Other metadata
  trigger_log triggers_;
  atomic::type<bool> is_valid_;
};

/**
 * @brief Log of aggregate_info pointers
 */
typedef monolog::monolog_exp2<aggregate_info*> aggregate_log;

}

#endif /* CONFLUO_AGGREGATE_INFO_H_ */
