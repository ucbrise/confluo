#ifndef DIALOG_AGGREGATED_REFLOG_H_
#define DIALOG_AGGREGATED_REFLOG_H_

#include "aggregate.h"
#include "reflog.h"
#include "trigger_log.h"

namespace dialog {

class aggregated_reflog : public reflog {
 public:
  typedef reflog::iterator iterator;
  typedef reflog::const_iterator const_iterator;

  aggregated_reflog(const trigger_log& triggers)
      : reflog(),
        num_aggregates_(triggers.size()),
        aggregates_(new aggregate[triggers.size()]) {
    for (size_t i = 0; i < num_aggregates_; i++) {
      aggregates_[i] = triggers.at(i)->create_aggregate();
    }
  }

  void update_aggregate(int thread_id, size_t trigger_id, const numeric& value,
                        uint64_t version) {
    aggregates_[trigger_id].update(thread_id, value, version);
  }

  numeric get_aggregate(size_t trigger_id, uint64_t version) const {
    return aggregates_[trigger_id].get(version);
  }

  size_t num_aggregates() const {
    return num_aggregates_;
  }

 private:
  size_t num_aggregates_;
  aggregate* aggregates_;
};

}

#endif /* DIALOG_AGGREGATED_REFLOG_H_ */
