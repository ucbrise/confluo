#include "schema/column.h"

namespace confluo {

column_t::column_t()
    : idx_(UINT16_MAX),
      type_(NONE_TYPE),
      offset_(UINT16_MAX) {
}

column_t::column_t(uint16_t idx,
                   uint16_t offset,
                   const data_type &type,
                   const std::string &name,
                   const mutable_value &min,
                   const mutable_value &max)
    : idx_(idx),
      type_(type),
      offset_(offset),
      name_(string_utils::to_upper(name)),
      min_(min),
      max_(max) {
}

std::string column_t::name() const {
  return name_;
}

const data_type &column_t::type() const {
  return type_;
}

uint16_t column_t::offset() const {
  return offset_;
}

uint16_t column_t::idx() const {
  return idx_;
}

mutable_value column_t::min() const {
  return min_;
}

mutable_value column_t::max() const {
  return max_;
}

uint16_t column_t::index_id() const {
  return idx_state_.id();
}

double column_t::index_bucket_size() const {
  return idx_state_.bucket_size();
}

bool column_t::is_indexed() const {
  return idx_state_.is_indexed();
}

bool column_t::set_indexing() {
  return idx_state_.set_indexing();
}

void column_t::set_indexed(uint16_t index_id, double bucket_size) {
  idx_state_.set_indexed(index_id, bucket_size);
}

void column_t::set_unindexed() {
  idx_state_.set_unindexed();
}

bool column_t::disable_indexing() {
  return idx_state_.disable_indexing();
}

field_t column_t::apply(void *data) const {
  return field_t(idx_, type_,
                 reinterpret_cast<unsigned char*>(data) + offset_,
                 is_indexed(), idx_state_.id(), idx_state_.bucket_size());
}

column_snapshot column_t::snapshot() const {
  return {type_, offset_, is_indexed(), index_id(), index_bucket_size()};
}
}