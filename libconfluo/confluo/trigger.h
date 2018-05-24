#ifndef CONFLUO_TRIGGER_H_
#define CONFLUO_TRIGGER_H_

#include "aggregate/aggregate.h"
#include "types/relational_ops.h"
#include "schema/schema_snapshot.h"

namespace confluo {
namespace monitor {

/**
 * A trigger structure. Contains trigger properties including the
 * periodicity, name, and aggregate
 */
struct trigger {
 public:
  /**
   * Constructor that initializes a trigger
   *
   * @param name The name of the trigger
   * @param aggregate_name The name of the aggregate on which the trigger
   * is defined
   * @param op Trigger's relational operator
   * @param threshold Trigger's threshold
   * @param periodicity_ms Trigger's periodicity in milliseconds
   */
  trigger(const std::string &name, const std::string &aggregate_name, reational_op_id op, const numeric &threshold,
          uint64_t periodicity_ms);

  /**
   * Gets the trigger name
   * @return The trigger name
   */
  std::string name() const;

  /**
   * Constructs the trigger expression
   * @return The trigger expression
   */
  std::string expr() const;

  /**
   * Gets the filter name
   * @return The filter name
   */
  std::string aggregate_name() const;

  /**
   * Gets the operation
   * @return The operation
   */
  reational_op_id op() const;

  /**
   * Gets the threshold of the trigger
   * @return The threshold
   */
  const numeric threshold() const;

  /**
   * Get the periodicity for the trigger in milliseconds.
   * @return The periodicity for the trigger in milliseconds.
   */
  uint64_t periodicity_ms() const;

  /**
   * Invalidates the trigger
   * @return Whether trigger was successfully invalidated
   */
  bool invalidate();

  /**
   * Determines whether the trigger is valid
   * @return Whether the trigger is valid
   */
  bool is_valid() const;

  /**
   * Gets string representation of the trigger
   * @return The string representation
   */
  std::string to_string() const;

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
