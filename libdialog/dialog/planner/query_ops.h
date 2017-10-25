#ifndef PLANNER_QUERY_OPS_H_
#define PLANNER_QUERY_OPS_H_

#include "radix_tree.h"
#include "data_log.h"
#include "schema.h"
#include "record_offset_range.h"
#include "lazy/stream.h"

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
  full_scan_op(const data_log* dlog, const schema_t* schema,
               const compiled_expression& expr)
      : query_op(query_op_type::D_SCAN_OP),
        dlog_(dlog),
        schema_(schema),
        expr_(expr) {
  }

  virtual std::string to_string() const override {
    return "full_scan(" + expr_.to_string() + ")";
  }

  virtual uint64_t cost() const override {
    return UINT64_MAX;
  }

  lazy::stream<record_t> execute(uint64_t version) const {
    static auto to_record = [this](uint64_t offset) -> record_t {
      return this->schema_->apply(offset, this->dlog_->cptr(offset));
    };
    auto expr_check = [this](const record_t& r) -> bool {
      return this->expr_.test(r);
    };
    auto r = record_offset_range(version, schema_->record_size());
    return lazy::container_to_stream(r).map(to_record).filter(expr_check);
  }

 private:
  const data_log* dlog_;
  const schema_t* schema_;
  const compiled_expression& expr_;
};

class index_op : public query_op {
 public:
  typedef std::pair<byte_string, byte_string> key_range;

  index_op(const data_log* dlog, const index::radix_index* index,
           const schema_t* schema, const key_range& range,
           const compiled_minterm& m)
      : query_op(query_op_type::D_INDEX_OP),
        dlog_(dlog),
        index_(index),
        schema_(schema),
        range_(range) {
    expr_.insert(m);
  }

  virtual std::string to_string() const override {
    return "range(" + range_.first.to_string() + "," + range_.second.to_string()
        + ")" + " on index=" + index_->to_string() + " + filter("
        + expr_.to_string() + ")";
  }

  virtual uint64_t cost() const override {
    return index_->approx_count(range_.first, range_.second);
  }

  lazy::stream<record_t> execute(uint64_t version) const {
    const data_log* d = dlog_;
    const schema_t* s = schema_;
    compiled_expression e = expr_;

    auto version_check = [version](uint64_t offset) -> bool {
      return offset < version;
    };

    auto to_record = [d, s](uint64_t offset) -> record_t {
      return s->apply(offset, d->cptr(offset));
    };

    auto expr_check = [e](const record_t& r) -> bool {
      return e.test(r);
    };

    auto r = index_->range_lookup(range_.first, range_.second);
    return lazy::container_to_stream(r).filter(version_check).map(to_record)
        .filter(expr_check);
  }

 private:
  const data_log* dlog_;
  const index::radix_index* index_;
  const schema_t* schema_;
  key_range range_;
  compiled_expression expr_;
};

}
}

#endif /* PLANNER_QUERY_OPS_H_ */
