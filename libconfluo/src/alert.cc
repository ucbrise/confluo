#include "alert.h"

namespace confluo {

monitor::alert::alert()
    : version(0),
      time_bucket(0) {
}

monitor::alert::alert(uint64_t _time_bucket,
                      const std::string &trigger_name,
                      const std::string &trigger_expr,
                      const numeric &_value,
                      uint64_t _version)
    : trigger_name(trigger_name),
      trigger_expr(trigger_expr),
      value(_value),
      version(_version),
      time_bucket(_time_bucket) {
}

std::string monitor::alert::to_string() const {
  return "{ timestamp: " + std::to_string(time_bucket) + ", trigger-name: \""
      + trigger_name + "\", trigger-expression: \"" + trigger_expr
      + "\", trigger-value: \"" + value.to_string() + "\", version: "
      + std::to_string(version) + " }";
}

bool monitor::operator<(const monitor::alert &left, const monitor::alert &right) {
  if (left.time_bucket == right.time_bucket
      && left.trigger_name == right.trigger_name) {
    return left.value < right.value;
  } else if (left.time_bucket == right.time_bucket) {
    return left.trigger_name < right.trigger_name;
  }
  return left.time_bucket < right.time_bucket;
}

}