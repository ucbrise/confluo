#include "planner/query_ops.h"

namespace confluo {
namespace planner {

query_op::query_op(const query_op_type &op)
    : op_(op) {
}

query_op::~query_op() {}

query_op_type query_op::op_type() {
  return op_;
}

no_op::no_op()
    : query_op(query_op_type::D_NO_OP) {
}

std::string no_op::to_string() const {
  return "no_op";
}

uint64_t no_op::cost() const {
  return UINT64_C(0);
}

no_valid_index_op::no_valid_index_op()
    : query_op(query_op_type::D_NO_VALID_INDEX_OP) {
}

std::string no_valid_index_op::to_string() const {
  return "no_valid_index_op";
}

uint64_t no_valid_index_op::cost() const {
  return UINT64_MAX;
}

full_scan_op::full_scan_op()
    : query_op(query_op_type::D_SCAN_OP) {
}

std::string full_scan_op::to_string() const {
  return "full_scan";
}

uint64_t full_scan_op::cost() const {
  return UINT64_MAX;
}

index_op::index_op(const index::radix_index *index, const index_op::key_range &range)
    : query_op(query_op_type::D_INDEX_OP),
      index_(index),
      range_(range) {
}

std::string index_op::to_string() const {
  return "range(" + range_.first.to_string() + "," + range_.second.to_string()
      + ")" + " on index=" + index_->to_string();
}

uint64_t index_op::cost() const {
  return index_->approx_count(range_.first, range_.second);
}

index::radix_index::rt_result index_op::query_index() {
  return index_->range_lookup(range_.first, range_.second);
}

}
}