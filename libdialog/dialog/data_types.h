#ifndef DIALOG_DATA_TYPES_H_
#define DIALOG_DATA_TYPES_H_

#include <limits>
#include <string>
#include <array>
#include <cstdint>

#include "relational_ops.h"
#include "arithmetic_ops.h"
#include "exceptions.h"

namespace dialog {

enum type_id
  : uint16_t {
    D_NONE = UINT16_MAX,
  D_BOOL = 0,
  D_CHAR = 1,
  D_SHORT = 2,
  D_INT = 3,
  D_LONG = 4,
  D_FLOAT = 5,
  D_DOUBLE = 6,
  D_STRING = 7
};

namespace limits {

static bool bool_min = std::numeric_limits<bool>::lowest();
static bool bool_zero = false;
static bool bool_one = true;
static bool bool_max = std::numeric_limits<bool>::max();

static char char_min = std::numeric_limits<char>::lowest();
static char char_zero = static_cast<char>(0);
static char char_one = static_cast<char>(1);
static char char_max = std::numeric_limits<char>::max();

static short short_min = std::numeric_limits<short>::lowest();
static short short_zero = static_cast<short>(0);
static short short_one = static_cast<short>(1);
static short short_max = std::numeric_limits<short>::max();

static int int_min = std::numeric_limits<int>::lowest();
static int int_zero = static_cast<int>(0);
static int int_one = static_cast<int>(1);
static int int_max = std::numeric_limits<int>::max();

static long long_min = std::numeric_limits<long>::lowest();
static long long_zero = static_cast<long>(0);
static long long_one = static_cast<long>(1);
static long long_max = std::numeric_limits<long>::max();

static float float_min = std::numeric_limits<float>::lowest();
static float float_zero = static_cast<float>(0.0);
static float float_one = static_cast<float>(1.0);
static float float_max = std::numeric_limits<float>::max();

static double double_min = std::numeric_limits<double>::lowest();
static double double_zero = static_cast<double>(0.0);
static double double_one = static_cast<double>(1.0);
static double double_max = std::numeric_limits<double>::max();

}

struct data_type {
 public:
  type_id id;
  size_t size;
  void* min;
  void* zero;
  void* one;
  void* max;

  data_type(size_t _size)
      : id(type_id::D_STRING),
        size(_size),
        min(nullptr),
        zero(nullptr),
        one(nullptr),
        max(nullptr) {
    relops_ = init_relops<std::string>();
    unaryops_ = init_unaryops<std::string>();
    binaryops_ = init_binaryops<std::string>();
  }

  data_type(type_id _id = type_id::D_NONE)
      : id(_id) {
    switch (id) {
      case type_id::D_NONE: {
        size = 0;
        relops_ = init_relops<void>();
        unaryops_ = init_unaryops<void>();
        binaryops_ = init_binaryops<void>();
        min = nullptr;
        zero = nullptr;
        one = nullptr;
        max = nullptr;
        break;
      }
      case type_id::D_BOOL: {
        size = sizeof(bool);
        relops_ = init_relops<bool>();
        unaryops_ = init_unaryops<bool>();
        binaryops_ = init_binaryops<bool>();
        min = &limits::bool_min;
        zero = &limits::bool_zero;
        one = &limits::bool_one;
        max = &limits::bool_max;
        break;
      }
      case type_id::D_CHAR: {
        size = sizeof(char);
        relops_ = init_relops<char>();
        unaryops_ = init_unaryops<char>();
        binaryops_ = init_binaryops<char>();
        min = &limits::char_min;
        zero = &limits::char_zero;
        one = &limits::char_one;
        max = &limits::char_max;
        break;
      }
      case type_id::D_SHORT: {
        size = sizeof(short);
        relops_ = init_relops<short>();
        unaryops_ = init_unaryops<short>();
        binaryops_ = init_binaryops<short>();
        min = &limits::short_min;
        zero = &limits::short_zero;
        one = &limits::short_one;
        max = &limits::short_max;
        break;
      }
      case type_id::D_INT: {
        size = sizeof(int);
        relops_ = init_relops<int>();
        unaryops_ = init_unaryops<int>();
        binaryops_ = init_binaryops<int>();
        min = &limits::int_min;
        zero = &limits::int_zero;
        one = &limits::int_one;
        max = &limits::int_max;
        break;
      }
      case type_id::D_LONG: {
        size = sizeof(long);
        relops_ = init_relops<long>();
        unaryops_ = init_unaryops<long>();
        binaryops_ = init_binaryops<long>();
        min = &limits::long_min;
        zero = &limits::long_zero;
        one = &limits::long_one;
        max = &limits::long_max;
        break;
      }
      case type_id::D_FLOAT: {
        size = sizeof(float);
        relops_ = init_relops<float>();
        unaryops_ = init_unaryops<float>();
        binaryops_ = init_binaryops<float>();
        min = &limits::float_min;
        zero = &limits::float_zero;
        one = &limits::float_one;
        max = &limits::float_max;
        break;
      }
      case type_id::D_DOUBLE: {
        size = sizeof(double);
        relops_ = init_relops<double>();
        unaryops_ = init_unaryops<double>();
        binaryops_ = init_binaryops<double>();
        min = &limits::double_min;
        zero = &limits::double_zero;
        one = &limits::double_one;
        max = &limits::double_max;
        break;
      }
      default: {
        throw invalid_operation_exception(
            "Must specify length for non-primitive data types");
      }
    }
  }

  data_type(const data_type& other)
      : id(other.id),
        size(other.size),
        min(other.min),
        zero(other.zero),
        one(other.one),
        max(other.max),
        relops_(other.relops_),
        unaryops_(other.unaryops_),
        binaryops_(other.binaryops_) {
  }

  inline bool operator==(const data_type& other) const {
    return id == other.id && size == other.size;
  }

  inline bool operator!=(const data_type& other) const {
    return id != other.id || size != other.size;
  }

  inline const relational_fn& relop(relop_id id) const {
    return relops_[id];
  }

  inline const unary_fn& unaryop(unaryop_id id) const {
    return unaryops_[id];
  }

  inline const binary_fn& binaryop(binaryop_id id) const {
    return binaryops_[id];
  }

  inline std::string to_string() const {
    switch (id) {
      case type_id::D_BOOL:
        return "bool";
      case type_id::D_CHAR:
        return "char";
      case type_id::D_SHORT:
        return "double";
      case type_id::D_INT:
        return "int";
      case type_id::D_LONG:
        return "long";
      case type_id::D_FLOAT:
        return "float";
      case type_id::D_DOUBLE:
        return "double";
      case type_id::D_STRING:
        return "string";
      default:
        return "invalid_type";
    }
  }

 private:
  rel_ops_t relops_;
  unary_ops_t unaryops_;
  binary_ops_t binaryops_;
};

static data_type NONE_TYPE(type_id::D_NONE);
static data_type BOOL_TYPE(type_id::D_BOOL);
static data_type CHAR_TYPE(type_id::D_CHAR);
static data_type SHORT_TYPE(type_id::D_SHORT);
static data_type INT_TYPE(type_id::D_INT);
static data_type LONG_TYPE(type_id::D_LONG);
static data_type FLOAT_TYPE(type_id::D_FLOAT);
static data_type DOUBLE_TYPE(type_id::D_DOUBLE);

static data_type string_type(size_t size) {
  return data_type(size);
}

static data_type& bool_type() {
  return BOOL_TYPE;
}

static data_type& char_type() {
  return CHAR_TYPE;
}

static data_type& short_type() {
  return SHORT_TYPE;
}

static data_type& int_type() {
  return INT_TYPE;
}

static data_type& long_type() {
  return LONG_TYPE;
}

static data_type& float_type() {
  return FLOAT_TYPE;
}

static data_type& double_type() {
  return DOUBLE_TYPE;
}

}

#endif /* DIALOG_DATA_TYPES_H_ */
