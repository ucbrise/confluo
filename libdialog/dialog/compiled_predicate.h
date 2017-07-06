#ifndef DIALOG_COMPILED_PREDICATE_H_
#define DIALOG_COMPILED_PREDICATE_H_

#include <string>

#include "column.h"
#include "value.h"
#include "expression.h"
#include "relational_ops.h"

namespace dialog {

struct compiled_predicate {
  column_t col;
  relop_id op;
  value_t val;
  std::string str;

  compiled_predicate()
      : op() {
  }

  compiled_predicate(const compiled_predicate& other) {
    col = other.col;
    op = other.op;
    val = other.val;
    str = other.str;
  }

  template<typename schema_t>
  compiled_predicate(const predicate_t& p, const schema_t& schema)
      : op(p.op) {
    try {
      col = schema.lookup(p.attr);
    } catch (std::exception& e) {
      throw parse_exception("No such attribute: " + p.attr);
    }
    try {
      val = value_t::from_string(p.value, col.type);
    } catch (std::exception& e) {
      throw parse_exception(
          "Could not parse attribute " + p.attr + " value " + p.value
              + " to type " + col.type.to_string());
    }
    str = p.to_string();
  }

  inline bool test(const record_t& r) const {
    const field_t& f = r.get(col.idx);
    return f.value.relop(op, val);
  }

  std::string to_string() const {
    return str;
  }
};

}

#endif /* DIALOG_COMPILED_PREDICATE_H_ */
