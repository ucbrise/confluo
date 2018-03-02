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
  alert()
      : version(0),
        time_bucket(0) {
  }

  /**
   * Constructs an alert that contains information about the specified
   * trigger
   * @param _time_bucket the time associated with trigger creation, used
   * for comparison against other alerts
   * @param trigger_name identifier for trigger, no error checking for 
   * invalid/incorrect name
   * @param trigger_expr the executed instruction, no error checking for
   * invalid expressions
   * @param _value result of the trigger, used for comparing alerts
   * @param _version the associated marker used for the trigger
   */
  alert(uint64_t _time_bucket, const std::string& trigger_name,
        const std::string& trigger_expr, const numeric& _value,
        uint64_t _version)
      : trigger_name(trigger_name),
        trigger_expr(trigger_expr),
        value(_value),
        version(_version),
        time_bucket(_time_bucket) {
  }

  /**
   * Combines information about the associated trigger into one string
   * @return the string representation that contains the timestamp, name,
   * expression, value, and version of the trigger
   */
  std::string to_string() const {
    return "{ timestamp: " + std::to_string(time_bucket) + ", trigger-name: \""
        + trigger_name + "\", trigger-expression: \"" + trigger_expr
        + "\", trigger-value: \"" + value.to_string() + "\", version: "
        + std::to_string(version) + " }";
  }

  /**
   * Compares two alerts (used for sorting)
   * @param left one of the alerts for comparison, assumes time_bucket, 
   * trigger_name, and value fields are set correctly
   * @param right the other alert used for comparison, assumes the 
   * time_bucket, trigger_name, and value fields are initialized
   */
  friend bool operator<(const alert& left, const alert& right) {
    if (left.time_bucket == right.time_bucket
        && left.trigger_name == right.trigger_name) {
      return left.value < right.value;
    } else if (left.time_bucket == right.time_bucket) {
      return left.trigger_name < right.trigger_name;
    }
    return left.time_bucket < right.time_bucket;
  }

  /** name used for identifying the trigger */
  std::string trigger_name;
  /** expression that is executed for the trigger */
  std::string trigger_expr;
  /** item used for comparing alerts */
  numeric value;
  /** marker associated with the trigger */
  uint32_t version;
  /** time associated with the trigger creation */
  uint64_t time_bucket;
};

}
}

#endif /* CONFLUO_ALERT_H_ */
