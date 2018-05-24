#include "aggregate/aggregate_info.h"

namespace confluo {

aggregate_info::aggregate_info(std::string name, aggregator agg, uint16_t field_idx)
    : name_(std::move(name)),
      agg_(std::move(agg)),
      field_idx_(field_idx),
      is_valid_(true) {
}

const std::string aggregate_info::name() const {
  return name_;
}

data_type aggregate_info::result_type() const {
  return agg_.result_type;
}

uint16_t aggregate_info::field_idx() const {
  return field_idx_;
}

numeric aggregate_info::value(const std::string &str) {
  return numeric::parse(str, result_type());
}

numeric aggregate_info::comb_op(const numeric &a, const schema_snapshot &s, void *data) {
  numeric b(s.get(data, field_idx_));
  return comb_op(a, b);
}

numeric aggregate_info::comb_op(const numeric &a, const numeric &b) {
  return agg_.comb_op(a.is_valid() ? a : zero(), b);
}

numeric aggregate_info::seq_op(const numeric &a, const schema_snapshot &s, void *data) {
  numeric b(s.get(data, field_idx_));
  return seq_op(a, b);
}

numeric aggregate_info::seq_op(const numeric &a, const numeric &b) {
  return agg_.seq_op(a.is_valid() ? a : zero(), b);
}

numeric aggregate_info::zero() {
  return agg_.zero;
}

aggregate aggregate_info::create_aggregate() const {
  return aggregate(result_type(), agg_);
}

bool aggregate_info::invalidate() {
  bool expected = true;
  return atomic::strong::cas(&is_valid_, &expected, false);
}

bool aggregate_info::is_valid() const {
  return atomic::load(&is_valid_);
}

size_t aggregate_info::add_trigger(trigger *t) {
  return triggers_.push_back(t);
}

bool aggregate_info::remove_trigger(size_t id) {
  return triggers_.at(id)->invalidate();
}

trigger *aggregate_info::get_trigger(size_t id) {
  return triggers_.at(id);
}

size_t aggregate_info::num_triggers() const {
  return triggers_.size();
}

}