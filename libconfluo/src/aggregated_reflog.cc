#include "aggregated_reflog.h"

namespace confluo {

aggregated_reflog::aggregated_reflog()
    : reflog(),
      num_aggregates_(0),
      aggregates_() {
}

aggregated_reflog::aggregated_reflog(const aggregate_log &aggregates)
    : reflog() {
  size_t alloc_size = sizeof(aggregate) * aggregates.size();
  aggregate *aggs = static_cast<aggregate *>(allocator::instance().alloc(alloc_size));
  storage::lifecycle_util<aggregate>::construct(aggs);
  for (size_t i = 0; i < aggregates.size(); i++) {
    aggs[i] = aggregates.at(i)->create_aggregate();
  }
  init_aggregates(aggregates.size(), aggs);
}

void aggregated_reflog::init_aggregates(size_t num_aggregates, aggregate *aggregates) {
  num_aggregates_ = num_aggregates;
  aggregates_ = storage::swappable_ptr<aggregate>(aggregates);
}

numeric aggregated_reflog::get_aggregate(size_t aid, uint64_t version) const {
  storage::read_only_ptr<aggregate> copy;
  aggregates_.atomic_copy(copy);
  return copy.get()[aid].get(version);
}

void aggregated_reflog::seq_update_aggregate(int thread_id, size_t aid, const numeric &value, uint64_t version) {
  aggregates_.atomic_load()[aid].seq_update(thread_id, value, version);
}

void aggregated_reflog::comb_update_aggregate(int thread_id, size_t aid, const numeric &value, uint64_t version) {
  aggregates_.atomic_load()[aid].comb_update(thread_id, value, version);
}

size_t aggregated_reflog::num_aggregates() const {
  return num_aggregates_;
}

storage::swappable_ptr<aggregate> &aggregated_reflog::aggregates() {
  return aggregates_;
}

}