#ifndef DIALOG_ALERT_INDEX_H_
#define DIALOG_ALERT_INDEX_H_

#include "alert.h"
#include "monolog_exp2.h"
#include "radix_tree.h"
#include "trigger.h"
#include "types/byte_string.h"
#include "types/numeric.h"

namespace confluo {
namespace monitor {

class alert_index {
 public:
  typedef monolog::monolog_exp2<alert> alert_log;
  typedef index::radix_tree<alert_log> idx_t;
  typedef idx_t::rt_result alert_list;

  alert_index()
      : idx_(8, 256) {
  }

  // Note: single threaded
  void add_alert(uint64_t time_bucket, const std::string& trigger_name,
                 const std::string& trigger_expr, const numeric& value,
                 uint64_t version) {
    auto log = idx_.get_or_create(make_key(time_bucket));
    if (find_alert(log, trigger_name, value) == -1)
      log->push_back(
          alert(time_bucket, trigger_name, trigger_expr, value, version));
  }

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

#endif /* DIALOG_ALERT_INDEX_H_ */
