#ifndef CONFLUO_ALERT_H_
#define CONFLUO_ALERT_H_

#include "trigger.h"
#include "types/numeric.h"
#include "time_utils.h"

namespace confluo {
namespace monitor {

/**
 * A message that stores information about schema triggers
 */
struct alert {
  /**
   * Constructs an alert with no associated trigger info
   */
  alert();

  /**
   * Constructs an alert that contains information about the specified trigger
   *
   * @param _time_bucket the time associated with trigger creation, used for comparison against other alerts
   * @param trigger_name identifier for trigger, no error checking for invalid/incorrect name
   * @param trigger_expr the executed instruction, no error checking for invalid expressions
   * @param _value result of the trigger, used for comparing alerts
   * @param _version the associated marker used for the trigger
   */
  alert(uint64_t _time_bucket, const std::string &trigger_name, const std::string &trigger_expr, const numeric &_value,
        uint64_t _version);

  /**
   * Combines information about the associated trigger into one string
   * @return the string representation of the alert
   */
  std::string to_string() const;

  /**
   * Compares two alerts (used for sorting)
   * @param left First alert
   * @param right Second alert
   */
  friend bool operator<(const alert &left, const alert &right);

  /** name used for identifying the trigger */
  std::string trigger_name;
  /** expression that is executed for the trigger */
  std::string trigger_expr;
  /** item used for comparing alerts */
  numeric value;
  /** marker associated with the trigger */
  uint64_t version;
  /** time associated with the trigger creation */
  uint64_t time_bucket;
};

}
}

#endif /* CONFLUO_ALERT_H_ */
