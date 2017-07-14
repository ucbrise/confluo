#ifndef DIALOG_COMPILED_PREDICATE_H_
#define DIALOG_COMPILED_PREDICATE_H_

#include <string>

#include "record.h"
#include "column.h"
#include "expression.h"
#include "immutable_value.h"
#include "relational_ops.h"
#include "index_filter.h"

namespace dialog {

struct compiled_predicate {
  compiled_predicate(const compiled_predicate& other)
      : col_(other.col_),
        op_(other.op_),
        val_(other.val_),
        str_(other.str_) {
  }

  template<typename schema_t>
  compiled_predicate(const predicate_t& p, const schema_t& s)
      : op_(p.op) {
    try {
      col_ = s[p.attr];
    } catch (std::exception& e) {
      THROW(parse_exception, "No such attribute " + p.attr + ": " + e.what());
    }
    try {
      val_ = immutable_value_t::parse(p.value, col_.type());
    } catch (std::exception& e) {
      THROW(
          parse_exception,
          "Could not parse attribute " + p.attr + " value " + p.value
              + " to type " + col_.type().to_string());
    }
    str_ = p.to_string();
  }

  inline column_t column() const {
    return col_;
  }

  inline relop_id op() const {
    return op_;
  }

  inline immutable_value_t value() const {
    return val_;
  }

  inline bool is_indexed() const {
    return col_.is_indexed();
  }

  inline std::vector<index_filter> idx_filters() const {
    uint32_t id = col_.index_id();
    double bucket_size = col_.index_bucket_size();
    switch (op_) {
      case relop_id::EQ:
        return {index_filter(id, val_.to_key(bucket_size),
              val_.to_key(bucket_size))};
      case relop_id::NEQ:
        return {index_filter(id, col_.min().to_key(bucket_size),
              --(val_.to_key(bucket_size))), index_filter(id, ++(val_.to_key(bucket_size)),
              col_.max().to_key(bucket_size))};
      case relop_id::GE:
        return {index_filter(id, val_.to_key(bucket_size),
              col_.max().to_key(bucket_size))};
      case relop_id::LE:
        return {index_filter(id, col_.min().to_key(bucket_size),
              val_.to_key(bucket_size))};
      case relop_id::GT:
        return {index_filter(id, ++(val_.to_key(bucket_size)),
              col_.max().to_key(bucket_size))};
      case relop_id::LT:
        return {index_filter(id, col_.min().to_key(bucket_size),
              --(val_.to_key(bucket_size)))};
      default:
        return {};
    }
  }

  inline bool test(const record_t& r) const {
    return relop(op_, r[col_.idx()].value(), val_);
  }

  inline std::string to_string() const {
    return str_;
  }

  inline bool operator<(const compiled_predicate& other) const {
    return to_string() < other.to_string();
  }

 private:

  column_t col_;
  relop_id op_;
  immutable_value_t val_;
  std::string str_;
};

}

#endif /* DIALOG_COMPILED_PREDICATE_H_ */
