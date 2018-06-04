#include "alert_index.h"

namespace confluo {

monitor::alert_index::alert_index()
    : idx_(8, 256) {
}

void monitor::alert_index::add_alert(uint64_t time_bucket,
                                     const std::string &trigger_name,
                                     const std::string &trigger_expr,
                                     const numeric &value,
                                     uint64_t version) {
  auto log = idx_.get_or_create(make_key(time_bucket));
  if (find_alert(log, trigger_name, value) == -1)
    log->push_back(
        alert(time_bucket, trigger_name, trigger_expr, value, version));
}

monitor::alert_index::alert_list monitor::alert_index::get_alerts(uint64_t t1, uint64_t t2) const {
  return idx_.range_lookup(make_key(t1), make_key(t2));
}

byte_string monitor::alert_index::make_key(uint64_t time_bucket) const {
  return byte_string(time_bucket);
}

int64_t monitor::alert_index::find_alert(monitor::alert_index::alert_log *log,
                                         const std::string &trigger_name,
                                         const numeric &value) const {
  size_t n_alerts = log->size();
  for (size_t i = 0; i < n_alerts; i++) {
    const alert &a = log->at(i);
    if (a.trigger_name == trigger_name && a.value == value) {
      return static_cast<int64_t>(i);
    }
  }
  return -1;
}

}