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

  /**
   * Initializing an alert_index
   */
  alert_index();

  // Note: single threaded
  /**
   * Adds alert to the log
   * @param time_bucket the trigger time bucket
   * @param trigger_name the trigger name
   * @param trigger_expr expression for the trigger
   * @param value the trigger value
   * @param version marker for the trigger
   */
  void add_alert(uint64_t time_bucket,
                 const std::string &trigger_name,
                 const std::string &trigger_expr,
                 const numeric &value,
                 uint64_t version);

  /**
   * Fetches alerts from range between timestamps
   * @param t1 first timestamp
   * @param t2 second timestamp
   * @return list of alerts between timestamp range
   */
  alert_list get_alerts(uint64_t t1, uint64_t t2) const;

 private:
  /**
   * Make a key from time bucket
   *
   * @param time_bucket The time bucket
   * @return The corresponding key
   */
  byte_string make_key(uint64_t time_bucket) const;

  /**
   * Find an alert from the trigger name and value
   *
   * @param log The alert log
   * @param trigger_name The trigger name
   * @param value The trigger value
   * @return The alert index
   */
  int64_t find_alert(alert_log *log, const std::string &trigger_name, const numeric &value) const;

  idx_t idx_;
};

}
}

#endif /* CONFLUO_ALERT_INDEX_H_ */
