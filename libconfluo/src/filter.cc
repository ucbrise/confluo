#include "filter.h"

namespace confluo {

filter::filter(const compiled_expression &exp, filter_fn fn)
    : exp_(exp),
      fn_(fn),
      idx_(8, 256),
      is_valid_(true) {
}

filter::filter(filter_fn fn)
    : exp_(),
      fn_(fn),
      idx_(8, 256),
      is_valid_(true) {
}

size_t filter::add_aggregate(aggregate_info *a) {
  return aggregates_.push_back(a);
}

bool filter::remove_aggregate(size_t id) {
  return aggregates_.at(id)->invalidate();
}

aggregate_info *filter::get_aggregate_info(size_t id) {
  return aggregates_.at(id);
}

size_t filter::num_aggregates() const {
  return aggregates_.size();
}

void filter::update(const record_t &r) {
  if (exp_.test(r) && fn_(r)) {
    aggregated_reflog *refs = idx_.insert(
        byte_string(r.timestamp() / configuration_params::TIME_RESOLUTION_NS()),
        r.log_offset(), aggregates_);
    int tid = thread_manager::get_id();
    if (tid < 0) {
      throw std::runtime_error("Thread is not registered");
    }
    for (size_t i = 0; i < refs->num_aggregates(); i++) {
      if (aggregates_.at(i)->is_valid()) {
        size_t field_idx = aggregates_.at(i)->field_idx();
        numeric val(r[field_idx].value());
        refs->seq_update_aggregate(tid, i, val, r.version());
      }
    }
  }
}

void filter::update(size_t log_offset, const schema_snapshot &snap, record_block &block, size_t record_size) {
  int tid = thread_manager::get_id();
  if (tid < 0) {
    throw std::runtime_error("Thread is not registered");
  }
  aggregated_reflog *refs = nullptr;
  std::vector<numeric> local_aggs;

  for (size_t i = 0; i < block.nrecords; i++) {
    void *cur_rec = reinterpret_cast<uint8_t *>(&block.data[i * record_size]);
    uint64_t rec_off = log_offset + i * record_size;
    if (exp_.test(snap, cur_rec)) {
      if (refs == nullptr) {
        refs = idx_.get_or_create(
            byte_string(static_cast<uint64_t>(block.time_block)),
            aggregates_);
        local_aggs.resize(refs->num_aggregates());
      }
      refs->push_back(rec_off);
      for (size_t j = 0; j < local_aggs.size(); j++)
        if (aggregates_.at(j)->is_valid())
          local_aggs[j] = aggregates_.at(j)->seq_op(local_aggs[j], snap,
                                                    cur_rec);
    }
  }

  size_t version = log_offset + block.nrecords * record_size;
  for (size_t j = 0; j < local_aggs.size(); j++)
    if (aggregates_.at(j)->is_valid() && !local_aggs[j].type().is_none())
      refs->comb_update_aggregate(tid, j, local_aggs[j], version);
}

aggregated_reflog *filter::lookup_unsafe(uint64_t ts_block) const {
  return idx_.get_unsafe(byte_string(ts_block));
}

aggregated_reflog const *filter::lookup(uint64_t ts_block) const {
  return idx_.get(byte_string(ts_block));
}

filter::range_result filter::lookup_range(uint64_t ts_block_begin, uint64_t ts_block_end) const {
  return idx_.range_lookup(byte_string(ts_block_begin),
                           byte_string(ts_block_end));
}

filter::reflog_result filter::lookup_range_reflogs(uint64_t ts_block_begin, uint64_t ts_block_end) const {
  return idx_.range_lookup_reflogs(byte_string(ts_block_begin),
                                   byte_string(ts_block_end));
}

bool filter::invalidate() {
  bool expected = true;
  return atomic::strong::cas(&is_valid_, &expected, false);
}

bool filter::is_valid() {
  return atomic::load(&is_valid_);
}

filter::idx_t &filter::data() {
  return idx_;
}

}