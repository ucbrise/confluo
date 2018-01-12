#ifndef CONFLUO_ALERT_INDEX_H_
#define CONFLUO_ALERT_INDEX_H_

#include "trigger.h"
#include "types/byte_string.h"
#include "types/numeric.h"
#include "alert.h"
#include "container/monolog/monolog.h"
#include "container/radix_tree.h"

namespace confluo {
namespace monitor {

/**
 * Efficient lookup and insertions of alerts
 */
class alert_index {
 public:
  /** log containing alerts  */
  typedef monolog::monolog_exp2<alert> alert_log;
  /** index data structure containing the log */
  typedef index::radix_tree<alert_log> idx_t;
  /** list of alerts returned from range lookup */
  typedef idx_t::rt_result alert_list;

  alert_index()
      : idx_(8, 256) {
  }

  // Note: single threaded
  /**
   * Adds alert to the log
   * @param time_bucket the trigger time bucket
   * @param trigger_name the trigger name
   * @param trigger_expr expression for the trigger
   * @param value the trigger value
   * @param version marker for the trigger
   */
  void add_alert(uint64_t time_bucket, const std::string& trigger_name,
                 const std::string& trigger_expr, const numeric& value,
                 uint64_t version) {
    auto log = idx_.get_or_create(make_key(time_bucket));
    if (find_alert(log, trigger_name, value) == -1)
      log->push_back(
          alert(time_bucket, trigger_name, trigger_expr, value, version));
  }

  /**
   * Fetches alerts from range between timestamps
   * @param t1 first timestamp
   * @param t2 second timestamp
   * @return list of alerts between timestamp range
   */
  alert_list get_alerts(uint64_t t1, uint64_t t2) const {
    return idx_.range_lookup(make_key(t1), make_key(t2));
  }

 private:
  byte_string make_key(uint64_t time_bucket) const {
    return byte_string(time_bucket);
  }

  int64_t find_alert(alert_log* log, const std::string& trigger_name,
                     const numeric& value) const {
    size_t nalerts = log->size();
    for (size_t i = 0; i < nalerts; i++) {
      const alert& a = log->at(i);
      if (a.trigger_name == trigger_name && a.value == value) {
        return i;
      }
    }
    return -1;
  }

  idx_t idx_;
};

}
}

#endif /* CONFLUO_ALERT_INDEX_H_ */
