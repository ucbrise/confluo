#ifndef PLANNER_QUERY_OPS_H_
#define PLANNER_QUERY_OPS_H_

#include "radix_tree.h"
#include "data_log.h"
#include "schema.h"
#include "record_stream.h"

namespace dialog {
namespace planner {

enum query_op_type {
  D_NO_OP = 0,
  D_SCAN_OP = 1,
  D_NO_VALID_INDEX_OP = 2,
  D_INDEX_OP = 3
};

class query_op {
 public:
  query_op(const query_op_type& op)
      : op_(op) {
  }

  virtual ~query_op() {
  }

  query_op_type op_type() {
    return op_;
  }

  virtual uint64_t cost() const = 0;

  virtual std::string to_string() const = 0;

 private:
  query_op_type op_;
};

class no_op : public query_op {
 public:
  no_op()
      : query_op(query_op_type::D_NO_OP) {
  }

  virtual std::string to_string() const override {
    return "no_op";
  }

  virtual uint64_t cost() const override {
    return UINT64_C(0);
  }
};

class no_valid_index_op : public query_op {
 public:
  no_valid_index_op()
      : query_op(query_op_type::D_NO_VALID_INDEX_OP) {
  }

  virtual std::string to_string() const override {
    return "no_valid_index_op";
  }

  virtual uint64_t cost() const override {
    return UINT64_MAX;
  }
};

class full_scan_op : public query_op {
 public:
  full_scan_op(const compiled_expression& expr)
      : query_op(query_op_type::D_SCAN_OP),
        expr_(expr) {
  }

  virtual std::string to_string() const override {
    return "full_scan(" + expr_.to_string() + ")";
  }

  virtual uint64_t cost() const override {
    return UINT64_MAX;
  }

 private:
  const compiled_expression& expr_;
};

class index_op : public query_op {
 public:
  typedef std::pair<byte_string, byte_string> key_range;

  typedef index::radix_index::rt_result offset_container;
  typedef record_stream<offset_container> record_stream_t;
  typedef filtered_record_stream<record_stream_t> filtered_record_stream_t;

  index_op(const data_log& dlog, const index::radix_index* index,
           const schema_t& schema, const key_range& range,
           const compiled_minterm& m)
      : query_op(query_op_type::D_INDEX_OP),
        dlog_(dlog),
        index_(index),
        schema_(schema),
        range_(range) {
    expr_.insert(m);
  }

  void set_range(const key_range& range) {
    range_ = range;
  }

  const index::radix_index* index() const {
    return index_;
  }

  byte_string const& kbegin() const {
    return range_.first;
  }

  byte_string const& kend() const {
    return range_.second;
  }

  immutable_byte_string kbegin_immutable() const {
    return range_.first.copy();
  }

  immutable_byte_string kend_immutable() const {
    return range_.second.copy();
  }

  compiled_expression const& minterm() const {
    return expr_;
  }

  virtual std::string to_string() const override {
    return "range(" + range_.first.to_string() + "," + range_.second.to_string()
        + ")" + " on index=" + index_->to_string() + " + filter("
        + expr_.to_string() + ")";
  }

  virtual uint64_t cost() const override {
    return index_->approx_count(range_.first, range_.second);
  }

  filtered_record_stream_t execute(uint64_t version) {
    auto mres = index_->range_lookup(range_.first, range_.second);
    record_stream_t rs(version, mres, schema_, dlog_);
    return filtered_record_stream_t(rs, expr_);
  }

 private:
  const data_log& dlog_;
  const index::radix_index* index_;
  const schema_t& schema_;
  key_range range_;
  compiled_expression expr_;
};

}
}

#endif /* PLANNER_QUERY_OPS_H_ */
