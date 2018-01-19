#ifndef CONFLUO_TRIGGER_H_
#define CONFLUO_TRIGGER_H_

#include "aggregate/aggregate.h"
#include "types/relational_ops.h"
#include "schema/schema_snapshot.h"

namespace confluo {
namespace monitor {

/**
 * A trigger for Confluo
 */
struct trigger {
 public:
  /**
   * Constructor that initializes a trigger
   *
   * @param name The name of the trigger
   * @param aggregate_name The name of the aggregate on which the trigger
   *        is defined
   * @param op Trigger's relational operator
   * @param threshold Trigger's threshold
   * @param periodicity Trigger's periodicity
   */
  trigger(const std::string& name, const std::string& aggregate_name,
          reational_op_id op, const numeric& threshold,
          const uint64_t periodicity_ms)
      : name_(name),
        aggregate_name_(aggregate_name),
        op_(op),
        threshold_(threshold),
        periodicity_ms_(periodicity_ms),
        is_valid_(true) {
  }

  /**
   * Gets the trigger name
   * @return The trigger name
   */
  std::string name() const {
    return name_;
  }

  /**
   * Constructs the trigger expression
   * @return The trigger expression
   */
  std::string expr() const {
    return aggregate_name_ + " " + relop_utils::op_to_str(op_) + " "
        + threshold_.to_string();
  }

  /**
   * Gets the filter name
   * @return The filter name
   */
  std::string aggregate_name() const {
    return aggregate_name_;
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

  /**
   * Get the periodicity for the trigger in milliseconds.
   * @return The periodicity for the trigger in milliseconds.
   */
  uint64_t periodicity_ms() const {
    return periodicity_ms_;
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
    return "{ name: \"" + name() + "\", expression: \"" + expr() + "\"}";
  }

 private:
  std::string name_;
  std::string aggregate_name_;
  reational_op_id op_;
  numeric threshold_;

  uint64_t periodicity_ms_;
  atomic::type<bool> is_valid_;
};

}
}

#endif /* CONFLUO_TRIGGER_H_ */
