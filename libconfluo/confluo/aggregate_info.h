#ifndef CONFLUO_AGGREGATE_INFO_H_
#define CONFLUO_AGGREGATE_INFO_H_

#include "types/aggregate_types.h"
#include "types/data_type.h"
#include "parser/aggregate_parser.h"
#include "trigger_log.h"

using namespace confluo::monitor;

namespace confluo {

class aggregate_info {
 public:
  /**
   * Constructor to initialize aggregate_info
   *
   * @param name The name of the aggregate.
   * @param atype Type of the aggregate.
   * @param field_type Type of field.
   * @param field_idx Index of field in schema.
   */
  aggregate_info(const std::string& name, aggregate_type atype,
                 data_type field_type, uint16_t field_idx)
      : name_(name),
        agg_type_(atype),
        field_type_(field_type),
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
   * Gets the aggregate type
   * @return The aggregate type
   */
  aggregate_type agg_type() const {
    return agg_type_;
  }

  /**
   * Gets the data type for the aggregate.
   * @return The data type for the aggregate.
   */
  data_type agg_data_type() const {
    return agg_type_ == aggregate_type::D_CNT ? LONG_TYPE : field_type_;
  }

  /**
   * Gets the data type of the field
   * @return The field data type
   */
  data_type field_type() const {
    return field_type_;
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
    return numeric::parse(str, agg_data_type());
  }

  /**
   * Aggregates the data
   * @param a The numeric
   * @param s The snapshot of the schema
   * @param data The data
   * @return An aggregate of the data based on the numeric
   */
  numeric agg(const numeric& a, const schema_snapshot& s, void* data) {
    numeric b(s.get(data, field_idx_));
    return aggregators[agg_type_].agg(
        a.type().is_none() ? aggregators[agg_type_].zero(field_type_) : a,
        agg_type_ == aggregate_type::D_CNT ? count_one : b);
  }

  /**
   * Aggregates the data
   * @param a A numeric
   * @param b Another numeric
   * @return An aggregate of the two numerics
   */
  numeric agg(const numeric& a, const numeric& b) {
    return aggregators[agg_type_].agg(
        a.type().is_none() ? aggregators[agg_type_].zero(field_type_) : a,
        agg_type_ == aggregate_type::D_CNT ? count_one : b);
  }

  /**
   * Creates an aggregate using the parsed aggregate information
   *
   * @return The aggregate
   */
  aggregate create_aggregate() const {
    return aggregate(agg_data_type(), agg_type_);
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
  aggregate_type agg_type_;
  data_type field_type_;
  uint16_t field_idx_;

  // Other metadata
  trigger_log triggers_;
  atomic::type<bool> is_valid_;
};

typedef monolog::monolog_exp2<aggregate_info*> aggregate_log;

}

#endif /* CONFLUO_AGGREGATE_INFO_H_ */
