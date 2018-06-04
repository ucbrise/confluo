#ifndef CONFLUO_CONTAINER_CURSOR_ALERT_CURSOR_H_
#define CONFLUO_CONTAINER_CURSOR_ALERT_CURSOR_H_

#include "batched_cursor.h"
#include "alert.h"
#include "alert_index.h"

namespace confluo {

typedef batched_cursor<monitor::alert> alert_cursor;

/**
 * A cursor for triggers and alerts 
 */
class trigger_alert_cursor : public alert_cursor {
 public:
  /** The list of alerts */
  typedef monitor::alert_index::alert_list alert_list;
  /** The iterator for alerts */
  typedef monitor::alert_index::alert_list::const_iterator alert_iterator;

  /**
   * Initializes a trigger alert cursor
   *
   * @param alerts The list of alerts
   * @param trigger_name The name of the trigger
   * @param batch_size The number of records in the batch
   */
  trigger_alert_cursor(const alert_list &alerts, const std::string &trigger_name, size_t batch_size = 64);

  /**
   * Loads the next record batch
   *
   * @return The size of the batch
   */
  virtual size_t load_next_batch() override;

 private:
  alert_iterator cur_;
  alert_iterator end_;
  std::string trigger_name_;
};

}

#endif /* CONFLUO_CONTAINER_CURSOR_ALERT_CURSOR_H_ */
