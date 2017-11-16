#ifndef DIALOG_TRIGGER_H_
#define DIALOG_TRIGGER_H_

#include "aggregate.h"
#include "schema_snapshot.h"

namespace dialog {
namespace monitor {
struct trigger {
 public:
  /**
   * Constructor that initializes a trigger
   * @param trigger_name The name of the trigger
   * @param filter_name The name of the filter
   * @param trigger_expr The trigger expression
   * @param agg The aggregate
   * @param field_name The field name to act on
   * @param field_idx The field index
   * @param field_type The type of the field
   * @param op The operation to perform
   * @param threshold The threshold for the trigger
   */
  trigger(const std::string& trigger_name, const std::string& filter_name,
          const std::string& trigger_expr, aggregate_id agg,
          const std::string& field_name, size_t field_idx,
          const data_type& field_type, reational_op_id op,
          const numeric& threshold, const uint64_t periodicity_ms)
      : trigger_name_(trigger_name),
        filter_name_(filter_name),
        trigger_expr_(trigger_expr),
        agg_id_(agg),
        field_name_(field_name),
        field_idx_(field_idx),
        field_type_(field_type),
        op_(op),
        threshold_(threshold),
        periodicity_ms_(periodicity_ms),
        is_valid_(true) {
  }

  /**
   * Creates an aggregate for the trigger
   * @return The aggregate
   */
  aggregate create_aggregate() {
    return
        agg_id_ == aggregate_id::D_CNT ?
            aggregate(LONG_TYPE, agg_id_) : aggregate(field_type_, agg_id_);
  }

  /**
   * Gets the trigger name
   * @return The trigger name
   */
  std::string trigger_name() const {
    return trigger_name_;
  }

  /**
   * Gets the filter name
   * @return The filter name
   */
  std::string filter_name() const {
    return filter_name_;
  }

  /**
   * Gets the trigger expression
   * @return The trigger expression
   */
  std::string trigger_expr() const {
    return trigger_expr_;
  }

  /**
   * Gets the id of the aggregate
   * @return The aggregate id
   */
  aggregate_id agg_id() const {
    return agg_id_;
  }

  /**
   * Gets the field index
   * @return The field index
   */
  size_t field_idx() const {
    return field_idx_;
  }

  /**
   * Gets the type of the field
   * @return The field type
   */
  data_type field_type() const {
    return field_type_;
  }

  /**
   * Gets the operation
   * @return The operation
   */
  reational_op_id op() const {
    return op_;
  }

  /**
   * Gets the threshold of the trigger
   * @return The threshold
   */
  const numeric threshold() const {
    return threshold_;
  }

  uint64_t periodicity_ms() const {
    return periodicity_ms_;
  }

  /**
   * Gets the aggregate defined zero
   * @return The aggregate zero
   */
  numeric zero() {
    return aggregators[agg_id_].zero(field_type_);
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
    return aggregators[agg_id_].agg(
        a.type().is_none() ? zero() : a,
        agg_id_ == aggregate_id::D_CNT ? count_one : b);
  }

  /**
   * Invalidates the trigger
   * @return Whether trigger was successfully invalidated
   */
  bool invalidate() {
    bool expected = true;
    if (atomic::strong::cas(&is_valid_, &expected, false)) {
      return true;
    }
    return false;
  }

  /**
   * Determines whether the trigger is valid
   * @return Whether the trigger is valid
   */
  bool is_valid() const {
    return atomic::load(&is_valid_);
  }

  /**
   * Gets string representation of the trigger
   * @return The string representation
   */
  std::string to_string() const {
    return "Trigger Name: " + trigger_name_ + " Filter Name: " + filter_name_
        + " Trigger Expression: " + aggop_utils::agg_to_string(agg_id_) + "("
        + field_name_ + ") " + relop_utils::op_to_str(op_) + " "
        + threshold_.to_string();
  }

 private:
  std::string trigger_name_;
  std::string filter_name_;
  std::string trigger_expr_;
  aggregate_id agg_id_;
  std::string field_name_;
  uint32_t field_idx_;
  data_type field_type_;
  reational_op_id op_;
  numeric threshold_;
  uint64_t periodicity_ms_;
  atomic::type<bool> is_valid_;
};
}
}

#endif /* DIALOG_TRIGGER_H_ */
