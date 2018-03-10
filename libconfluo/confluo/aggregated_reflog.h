#ifndef CONFLUO_AGGREGATED_REFLOG_H_
#define CONFLUO_AGGREGATED_REFLOG_H_

#include "aggregate/aggregate.h"
#include "aggregate/aggregate_info.h"
#include "storage/allocator.h"
#include "container/reflog.h"
#include "storage/swappable_ptr.h"

namespace confluo {

/**
 * Stores all of the aggregates
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

  aggregated_reflog()
      : reflog(),
        num_aggregates_(0),
        aggregates_() {
  }

  /**
   * Constructs a reflog of aggregates from a log of aggregates
   *
   * @param aggregates The specified aggregates
   */
  aggregated_reflog(const aggregate_log& aggregates)
      : reflog() {
    storage::ptr_aux_block aux(storage::state_type::D_IN_MEMORY, storage::encoding_type::D_UNENCODED);
    size_t alloc_size = sizeof(aggregate) * aggregates.size();
    aggregate* aggs = static_cast<aggregate*>(ALLOCATOR.alloc(alloc_size, aux));
    storage::lifecycle_util<aggregate>::construct(aggs);
    for (size_t i = 0; i < aggregates.size(); i++) {
      aggs[i] = aggregates.at(i)->create_aggregate();
    }
    init_aggregates(aggregates.size(), aggs);
  }

  /**
   * Initialize aggregates.
   * @param num_aggregates number of aggregates
   * @param aggregates aggregates
   */
  void init_aggregates(size_t num_aggregates, aggregate* aggregates) {
    num_aggregates_ = num_aggregates;
    aggregates_ = storage::swappable_ptr<aggregate>(aggregates);
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
    storage::read_only_ptr<aggregate> copy;
    aggregates_.atomic_copy(copy);
    return copy.get()[aid].get(version);
  }

  /**
   * Updates an aggregate. Assumes no contention with archiver calling swap_aggregates.
   * Note: this assumption allows for update without performing a pointer copy.
   * @param thread_id thread id
   * @param aid aggregate id
   * @param value value to update with
   * @param version data log version
   */
  inline void seq_update_aggregate(int thread_id, size_t aid, const numeric& value, uint64_t version) {
    aggregates_.atomic_load()[aid].seq_update(thread_id, value, version);
  }

  /**
   * Updates an aggregate. Assumes no contention with archiver calling swap_aggregates.
   * Note: this assumption allows for update without performing a pointer copy.
   * @param thread_id thread id
   * @param aid aggregate id
   * @param value value to update with
   * @param version data log version
   */
  inline void comb_update_aggregate(int thread_id, size_t aid, const numeric& value, uint64_t version) {
    aggregates_.atomic_load()[aid].comb_update(thread_id, value, version);
  }

  /**
   * Gets the number of aggregates.
   * @return number of aggregates
   */
  inline size_t num_aggregates() const {
    return num_aggregates_;
  }

  /**
   * Returns aggregates of the reflog.
   * Note: it is unsafe to modify this data structure.
   *
   * @return aggregates
   */
  inline storage::swappable_ptr<aggregate>& aggregates() {
    return aggregates_;
  }

private:
  size_t num_aggregates_;
  storage::swappable_ptr<aggregate> aggregates_; // TODO or array of swappable_ptrs for less contention
};

}

#endif /* CONFLUO_AGGREGATED_REFLOG_H_ */
