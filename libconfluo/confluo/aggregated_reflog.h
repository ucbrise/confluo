#ifndef CONFLUO_AGGREGATED_REFLOG_H_
#define CONFLUO_AGGREGATED_REFLOG_H_

#include "aggregate/aggregate.h"
#include "aggregate/aggregate_info.h"
#include "storage/allocator.h"
#include "container/reflog.h"
#include "storage/swappable_ptr.h"

namespace confluo {

/**
 * Reflog with aggregates
 */
class aggregated_reflog : public reflog {
 public:
  /** The type of size of the reflog */
  typedef reflog::size_type size_type;
  /** The type of position of the reflog */
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

  aggregated_reflog();

  /**
   * Constructor. Initializes aggregates from an aggregate info log.
   * @param aggregates The specified aggregates
   */
  explicit aggregated_reflog(const aggregate_log &aggregates);

  /**
   * Initialize aggregates.
   * @param num_aggregates number of aggregates
   * @param aggregates aggregates
   */
  void init_aggregates(size_t num_aggregates, aggregate *aggregates);

  /**
   * Gets the specified aggregate
   *
   * @param aid The identifier for the desired aggregate
   * @param version The version of the aggregate to get
   *
   * @return A numeric that contains the aggregate value
   */
  numeric get_aggregate(size_t aid, uint64_t version) const;

  /**
   * Updates an aggregate. Assumes no contention with archiver calling swap_aggregates.
   * Note: this assumption allows for update without performing a pointer copy.
   * @param thread_id thread id
   * @param aid aggregate id
   * @param value value to update with
   * @param version data log version
   */
  void seq_update_aggregate(int thread_id, size_t aid, const numeric &value, uint64_t version);

  /**
   * Updates an aggregate. Assumes no contention with archiver calling swap_aggregates.
   * Note: this assumption allows for update without performing a pointer copy.
   * @param thread_id thread id
   * @param aid aggregate id
   * @param value value to update with
   * @param version data log version
   */
  void comb_update_aggregate(int thread_id, size_t aid, const numeric &value, uint64_t version);

  /**
   * Gets the number of aggregates.
   * @return number of aggregates
   */
  size_t num_aggregates() const;

  /**
   * Returns aggregates of the reflog.
   * Note: it is unsafe to modify this data structure.
   *
   * @return aggregates
   */
  storage::swappable_ptr<aggregate> &aggregates();

 private:
  size_t num_aggregates_;
  storage::swappable_ptr<aggregate> aggregates_; // TODO or array of swappable_ptrs for less contention
};

}

#endif /* CONFLUO_AGGREGATED_REFLOG_H_ */
