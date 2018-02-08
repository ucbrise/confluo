#ifndef CONFLUO_AGGREGATED_REFLOG_H_
#define CONFLUO_AGGREGATED_REFLOG_H_

#include "aggregate/aggregate.h"
#include "aggregate/aggregate_info.h"
#include "container/reflog.h"

namespace confluo {

/**
 * Stores all of the aggregates
 */
class aggregated_reflog : public reflog {
 public:
  /** The type of size of the reflog */
  typedef reflog::size_type size_type;
  /** The type of position of the reflog */;
  typedef reflog::pos_type pos_type;
  /** The type of value contained in the reflog */
  typedef reflog::value_type value_type;
  /** The type of difference in the reflog */
  typedef reflog::difference_type difference_type;
  /** The type of pointer in the reflog */
  typedef reflog::pointer pointer;
  /** The type of reference in the reflog */
  typedef reflog::reference reference;
  /** The type of iterator for the reflog */
  typedef reflog::iterator iterator;
  /** The type of constant iterator for the reflog */
  typedef reflog::const_iterator const_iterator;

  /**
   * Constructs a reflog of aggregates from a log of aggregates
   *
   * @param aggregates The specified aggregates
   */
  aggregated_reflog(const aggregate_log& aggregates)
      : reflog(),
        num_aggregates_(aggregates.size()),
        aggregates_(new aggregate[aggregates.size()]) {
    for (size_t i = 0; i < num_aggregates_; i++) {
      aggregates_[i] = aggregates.at(i)->create_aggregate();
    }
  }

  inline void seq_update_aggregate(int thread_id, size_t aid,
                                   const numeric& value, uint64_t version) {
    aggregates_[aid].seq_update(thread_id, value, version);
  }

  inline void comb_update_aggregate(int thread_id, size_t aid,
                                   const numeric& value, uint64_t version) {
    aggregates_[aid].comb_update(thread_id, value, version);
  }

  /**
   * Gets the specified aggregate
   *
   * @param aid The identifier for the desired aggregate
   * @param version The version of the aggregate to get
   *
   * @return A numeric that contains the aggregate value
   */
  inline numeric get_aggregate(size_t aid, uint64_t version) const {
    return aggregates_[aid].get(version);
  }

  /**
   * Gets the number of aggregates
   *
   * @return The number of aggregates
   */
  inline size_t num_aggregates() const {
    return num_aggregates_;
  }

 private:
  size_t num_aggregates_;
  aggregate* aggregates_;
};

}

#endif /* CONFLUO_AGGREGATED_REFLOG_H_ */
