#ifndef CONFLUO_AGGREGATED_REFLOG_H_
#define CONFLUO_AGGREGATED_REFLOG_H_

#include "aggregate.h"
#include "aggregate_info.h"
#include "container/reflog.h"

namespace confluo {

class aggregated_reflog : public reflog {
 public:
  typedef reflog::size_type size_type;
  typedef reflog::pos_type pos_type;
  typedef reflog::value_type value_type;
  typedef reflog::difference_type difference_type;
  typedef reflog::pointer pointer;
  typedef reflog::reference reference;
  typedef reflog::iterator iterator;
  typedef reflog::const_iterator const_iterator;

  aggregated_reflog(const aggregate_log& aggregates)
      : reflog(),
        num_aggregates_(aggregates.size()),
        aggregates_(new aggregate[aggregates.size()]) {
    for (size_t i = 0; i < num_aggregates_; i++) {
      aggregates_[i] = aggregates.at(i)->create_aggregate();
    }
  }

  inline void update_aggregate(int thread_id, size_t aid, const numeric& value,
                               uint64_t version) {
    aggregates_[aid].update(thread_id, value, version);
  }

  inline numeric get_aggregate(size_t aid, uint64_t version) const {
    return aggregates_[aid].get(version);
  }

  inline size_t num_aggregates() const {
    return num_aggregates_;
  }

 private:
  size_t num_aggregates_;
  aggregate* aggregates_;
};

}

#endif /* CONFLUO_AGGREGATED_REFLOG_H_ */
