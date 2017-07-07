#ifndef LIBDIALOG_DIALOG_RELATIONAL_OPS_H_
#define LIBDIALOG_DIALOG_RELATIONAL_OPS_H_

#include <array>

#include "exceptions.h"

namespace dialog {

/**
 * Relational operator
 */
enum relop_id
  : uint8_t {
    LT = 0,  //!< LT
  LE = 1,  //!< LE
  GT = 2,  //!< GT
  GE = 3,  //!< GE
  EQ = 4,  //!< EQ
  NEQ = 5  //!< NEQ
};

typedef bool (*relational_fn)(const void* v1, const void* v2);

typedef std::array<relational_fn, 6> rel_ops_t;

template<typename T>
inline bool less_than(const void* v1, const void* v2) {
  return *(reinterpret_cast<const T*>(v1)) < *(reinterpret_cast<const T*>(v2));
}

template<>
inline bool less_than<std::string>(const void* v1, const void* v2) {
  throw unsupported_exception("< not supported for string type");
}

template<>
inline bool less_than<void>(const void* v1, const void* v2) {
  throw unsupported_exception("< not supported for none type");
}

template<typename T>
inline bool less_than_equals(const void* v1, const void* v2) {
  return *(reinterpret_cast<const T*>(v1)) <= *(reinterpret_cast<const T*>(v2));
}

template<>
inline bool less_than_equals<std::string>(const void* v1, const void* v2) {
  throw unsupported_exception("<= not supported for string type");
}

template<>
inline bool less_than_equals<void>(const void* v1, const void* v2) {
  throw unsupported_exception("<= not supported for none type");
}

template<typename T>
inline bool greater_than(const void* v1, const void* v2) {
  return *(reinterpret_cast<const T*>(v1)) > *(reinterpret_cast<const T*>(v2));
}

template<>
inline bool greater_than<std::string>(const void* v1, const void* v2) {
  throw unsupported_exception("> not supported for string type");
}

template<>
inline bool greater_than<void>(const void* v1, const void* v2) {
  throw unsupported_exception("> not supported for none type");
}

template<typename T>
inline bool greater_than_equals(const void* v1, const void* v2) {
  return *(reinterpret_cast<const T*>(v1)) >= *(reinterpret_cast<const T*>(v2));
}

template<>
inline bool greater_than_equals<std::string>(const void* v1, const void* v2) {
  throw unsupported_exception(">= not supported for string type");
}

template<>
inline bool greater_than_equals<void>(const void* v1, const void* v2) {
  throw unsupported_exception(">= not supported for none type");
}

template<typename T>
inline bool equals(const void* v1, const void* v2) {
  return *(reinterpret_cast<const T*>(v1)) == *(reinterpret_cast<const T*>(v2));
}

template<>
inline bool equals<std::string>(const void* v1, const void* v2) {
  throw unsupported_exception("== not supported for string type");
}

template<>
inline bool equals<void>(const void* v1, const void* v2) {
  throw unsupported_exception("== not supported for none type");
}

template<typename T>
inline bool not_equals(const void* v1, const void* v2) {
  return *(reinterpret_cast<const T*>(v1)) != *(reinterpret_cast<const T*>(v2));
}

template<>
inline bool not_equals<std::string>(const void* v1, const void* v2) {
  throw unsupported_exception("!= not supported for string type");
}

template<>
inline bool not_equals<void>(const void* v1, const void* v2) {
  throw unsupported_exception("!= not supported for none type");
}

template<typename T>
static rel_ops_t init_relops() {
  return rel_ops_t { { less_than<T>, less_than_equals<T>, greater_than<T>,
      greater_than_equals<T>, equals<T>, not_equals<T> } };
}

class relop_utils {
 public:
  /**
   * Converts a string operator to relop_id enum
   *
   * @param op String operator
   * @return relop_id enum
   */
  static relop_id str_to_op(const std::string& op) {
    if (op == "==") {
      return relop_id::EQ;
    } else if (op == "!=") {
      return relop_id::NEQ;
    } else if (op == "<") {
      return relop_id::LT;
    } else if (op == ">") {
      return relop_id::GT;
    } else if (op == "<=") {
      return relop_id::LE;
    } else if (op == ">=") {
      return relop_id::GE;
    } else {
      throw parse_exception("Invalid operator " + op);
    }
  }

  /**
   * Converts a relop_id enum to string
   *
   * @param op relop_id enum
   * @return String representation of operator
   */
  static std::string op_to_str(const relop_id& op) {
    switch (op) {
      case relop_id::EQ:
        return "==";
      case relop_id::NEQ:
        return "!=";
      case relop_id::LT:
        return "<";
      case relop_id::GT:
        return ">";
      case relop_id::LE:
        return "<=";
      case relop_id::GE:
        return ">=";
    }
    return "INVALID";
  }
};

}

#endif /* LIBDIALOG_DIALOG_RELATIONAL_OPS_H_ */
