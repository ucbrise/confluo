#ifndef DIALOG_COMPILED_PREDICATE_H_
#define DIALOG_COMPILED_PREDICATE_H_

#include <string>

#include "record.h"
#include "column.h"
#include "value.h"
#include "expression.h"
#include "relational_ops.h"

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
      val_ = value_t::from_string(p.value, col_.type());
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

  inline value_t value() const {
    return val_;
  }

  inline bool test(const record_t& r) const {
    return r[col_.idx()].value().relop(op_, val_);
  }

  inline std::string to_string() const {
    return str_;
  }

 private:
  column_t col_;
  relop_id op_;
  value_t val_;
  std::string str_;
};

}

#endif /* DIALOG_COMPILED_PREDICATE_H_ */
