#ifndef LIBDIALOG_DIALOG_RELATIONAL_OPS_H_
#define LIBDIALOG_DIALOG_RELATIONAL_OPS_H_

#include <vector>

#include "exceptions.h"
#include "raw_data.h"

namespace confluo {

/**
 * Relational operator
 */
enum reational_op_id
  : uint8_t {
    LT = 0,  //!< LT
  LE = 1,  //!< LE
  GT = 2,  //!< GT
  GE = 3,  //!< GE
  EQ = 4,  //!< EQ
  NEQ = 5  //!< NEQ
};

typedef bool (*relational_op_t)(const immutable_raw_data& v1, const immutable_raw_data& v2);

typedef std::vector<relational_op_t> rel_ops_t;

template<typename T>
inline bool less_than(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() < v2.as<T>();
}

template<>
inline bool less_than<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "< not supported for none type");
}

template<typename T>
inline bool less_than_equals(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() <= v2.as<T>();
}

template<>
inline bool less_than_equals<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "<= not supported for none type");
}

template<typename T>
inline bool greater_than(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() > v2.as<T>();
}

template<>
inline bool greater_than<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "> not supported for none type");
}

template<typename T>
inline bool greater_than_equals(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() >= v2.as<T>();
}

template<>
inline bool greater_than_equals<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, ">= not supported for none type");
}

template<typename T>
inline bool equals(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() == v2.as<T>();
}

template<>
inline bool equals<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "== not supported for none type");
}

template<typename T>
inline bool not_equals(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() != v2.as<T>();
}

template<>
inline bool not_equals<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "!= not supported for none type");
}

template<typename T>
static rel_ops_t init_relops() {
  return {less_than<T>, less_than_equals<T>, greater_than<T>,
    greater_than_equals<T>, equals<T>, not_equals<T>};
}

class relop_utils {
 public:
  /**
   * Converts a string operator to relop_id enum
   *
   * @param op String operator
   * @return relop_id enum
   */
  static reational_op_id str_to_op(const std::string& op) {
    if (op == "==") {
      return reational_op_id::EQ;
    } else if (op == "!=") {
      return reational_op_id::NEQ;
    } else if (op == "<") {
      return reational_op_id::LT;
    } else if (op == ">") {
      return reational_op_id::GT;
    } else if (op == "<=") {
      return reational_op_id::LE;
    } else if (op == ">=") {
      return reational_op_id::GE;
    } else {
      THROW(parse_exception, "Invalid relational operator " + op);
    }
  }

  /**
   * Converts a relop_id enum to string
   *
   * @param op relop_id enum
   * @return String representation of operator
   */
  static std::string op_to_str(const reational_op_id& op) {
    switch (op) {
      case reational_op_id::EQ:
        return "==";
      case reational_op_id::NEQ:
        return "!=";
      case reational_op_id::LT:
        return "<";
      case reational_op_id::GT:
        return ">";
      case reational_op_id::LE:
        return "<=";
      case reational_op_id::GE:
        return ">=";
    }
    return "INVALID";
  }
};

}

#endif /* LIBDIALOG_DIALOG_RELATIONAL_OPS_H_ */
