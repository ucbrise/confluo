#ifndef DIALOG_ALERT_H_
#define DIALOG_ALERT_H_

#include "numeric.h"
#include "trigger.h"
#include "time_utils.h"

namespace dialog {
namespace monitor {

struct alert {
  alert()
      : trig(nullptr),
        version(0),
        time_bucket(0) {
  }

  alert(uint64_t _time_bucket, trigger* _trig, const numeric& _value,
        uint64_t _version)
      : trig(_trig),
        value(_value),
        version(_version),
        time_bucket(_time_bucket) {
  }

  std::string to_string() const {
    return "Timestamp: " + std::to_string(time_bucket) + " " + trig->to_string()
        + " Actual Value: " + value.to_string() + " Version: "
        + std::to_string(version);
  }

  friend bool operator<(const alert& left, const alert& right) {
    if (left.time_bucket == right.time_bucket
        && left.trig->trigger_name() == right.trig->trigger_name()) {
      return left.value < right.value;
    } else if (left.time_bucket == right.time_bucket) {
      return left.trig->trigger_name() < right.trig->trigger_name();
    }
    return left.time_bucket < right.time_bucket;
  }

  trigger* trig;
  numeric value;
  uint32_t version;
  uint64_t time_bucket;
};

}
}

#endif /* DIALOG_ALERT_H_ */
