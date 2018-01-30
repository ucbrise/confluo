#ifndef CONFLUO_TYPES_RELATIONAL_OPS_H_
#define CONFLUO_TYPES_RELATIONAL_OPS_H_

#include <vector>

#include "exceptions.h"
#include "raw_data.h"

namespace confluo {

/**
 * Relational operators
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

/**
 * Performs a less than comparison between two immutable raw data
 *
 * @tparam The type of the raw immutable data
 * @param v1 The first immutable raw data used for comparison
 * @param v2 The second immutable raw data used for comparison
 *
 * @return True if the first immutable raw data is less than the second,
 * false otherwise
 */
template<typename T>
inline bool less_than(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() < v2.as<T>();
}

/**
 * Performs a less than comparison between two immutable raw data of
 * void type
 *
 * @tparam The type of the raw immutable data, which is void in this case
 * @param v1 The first immutable raw data used for comparison
 * @param v2 The second immutable raw data used for comparison
 *
 * @throws unsupported_exception Less than comparison is not defined for
 * void types
 * @return True if the first immutable raw data is less than the second,
 * false otherwise
 */
template<>
inline bool less_than<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "< not supported for none type");
}

/**
 * Performs a less than or equal to 
 * comparison between two immutable raw data
 *
 * @tparam The type of the raw immutable data, which is void in this case
 * @param v1 The first immutable raw data used for comparison
 * @param v2 The second immutable raw data used for comparison
 *
 * @throws unsupported_exception Less than comparison is not defined for
 * void types
 * @return True if the first immutable raw data is less than the second,
 * false otherwise
 */
template<typename T>
inline bool less_than_equals(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() <= v2.as<T>();
}

/**
 * Peraluforms a less than or equal to comparison between two immutable
 * raw data values interpreted as being of the void type
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @return True if the first raw immutable data value is less than the
 * second, false otherwise
 *
 * @throw unsupported_exception This operation is not defined for void types
 */
template<>
inline bool less_than_equals<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "<= not supported for none type");
}

/**
 * Peraluforms a greater than comparison between two immutable
 * raw data values 
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @return True if the first raw immutable data value is greater than the
 * second, false otherwise
 */
template<typename T>
inline bool greater_than(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() > v2.as<T>();
}

/**
 * Performs a greater than comparison between two immutable
 * raw data values interpreted as being of the void type
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @return True if the first raw immutable data value is greater than the
 * second, false otherwise
 *
 * @throw unsupported_exception This operation is not supported for the
 * void type
 */
template<>
inline bool greater_than<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "> not supported for none type");
}

/**
 * Performs a greater than or equal to comparison between two immutable
 * raw data values
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @return True if the first raw immutable data value is greater than or
 * equal to the second, false otherwise
 */
template<typename T>
inline bool greater_than_equals(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() >= v2.as<T>();
}

/**
 * Performs a greater than or equal to comparison between two immutable
 * raw data values interpreted as being of the void type
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @return True if the first raw immutable data value is greater than the
 * second, false otherwise
 *
 * @throw unsupported_exception This operation is not supported for the
 * void type
 */
template<>
inline bool greater_than_equals<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, ">= not supported for none type");
}

/**
 * Performs an equality comparison between two immutable
 * raw data values
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @return True if the first raw immutable data value is equal to the 
 * second, false otherwise
 */
template<typename T>
inline bool equals(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() == v2.as<T>();
}

/**
 * Performs an equality comparison between two immutable
 * raw data values interpreted as being of the void type
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @throw unsupported_exception This operation is not supported for the void
 * type
 *
 * @return True if the first raw immutable data value is equal to the
 * second, false otherwise
 */
template<>
inline bool equals<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "== not supported for none type");
}

/**
 * Performs a not equal comparison between two immutable
 * raw data values
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @return True if the first raw immutable data value is not equal to the 
 * second, false otherwise
 */
template<typename T>
inline bool not_equals(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  return v1.as<T>() != v2.as<T>();
}

/**
 * Performs a not equal comparison between two immutable
 * raw data values interpreted as being of the void type
 *
 * @param v1 The first immutable raw data value used
 * @param v2 The second immutable raw data value used
 *
 * @throw unsupported_exception This operation is not supported for the void
 * type
 *
 * @return True if the first raw immutable data value is not equal to the
 * second, false otherwise
 */
template<>
inline bool not_equals<void>(const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "!= not supported for none type");
}

/**
 * Initializes a list of the relational operators a type needs to define
 *
 * @tparam T The type of data the operators work on
 *
 * @return A vector of relation operator function pointers
 */
template<typename T>
static rel_ops_t init_relops() {
  return {less_than<T>, less_than_equals<T>, greater_than<T>,
    greater_than_equals<T>, equals<T>, not_equals<T>};
}

/**
 * Utility functions for relational operators
 */
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

#endif /* CONFLUO_TYPES_RELATIONAL_OPS_H_ */
