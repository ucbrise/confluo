#include "container/cursor/alert_cursor.h"

namespace confluo {

trigger_alert_cursor::trigger_alert_cursor(const trigger_alert_cursor::alert_list &alerts,
                                           const std::string &trigger_name,
                                           size_t batch_size)
    : alert_cursor(batch_size),
      cur_(alerts.begin()),
      end_(alerts.end()),
      trigger_name_(trigger_name) {
  init();
}

size_t trigger_alert_cursor::load_next_batch() {
  size_t i = 0;
  for (; i < current_batch_.size() && cur_ != end_; ++i, ++cur_) {
    current_batch_[i] = *cur_;
    if (trigger_name_ != "" && trigger_name_ != current_batch_[i].trigger_name) {
      --i;
    }
  }
  return i;
}

}