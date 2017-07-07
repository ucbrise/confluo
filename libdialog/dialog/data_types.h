#ifndef DIALOG_DATA_TYPES_H_
#define DIALOG_DATA_TYPES_H_

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

struct data_type {
 public:
  type_id id;
  size_t size;

  data_type(size_t _size)
      : id(type_id::D_STRING),
        size(_size) {
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
        break;
      }
      case type_id::D_BOOL: {
        size = sizeof(bool);
        relops_ = init_relops<bool>();
        unaryops_ = init_unaryops<bool>();
        binaryops_ = init_binaryops<bool>();
        break;
      }
      case type_id::D_CHAR: {
        size = sizeof(char);
        relops_ = init_relops<char>();
        unaryops_ = init_unaryops<char>();
        binaryops_ = init_binaryops<char>();
        break;
      }
      case type_id::D_SHORT: {
        size = sizeof(short);
        relops_ = init_relops<short>();
        unaryops_ = init_unaryops<short>();
        binaryops_ = init_binaryops<short>();
        break;
      }
      case type_id::D_INT: {
        size = sizeof(int);
        relops_ = init_relops<int>();
        unaryops_ = init_unaryops<int>();
        binaryops_ = init_binaryops<int>();
        break;
      }
      case type_id::D_LONG: {
        size = sizeof(long);
        relops_ = init_relops<long>();
        unaryops_ = init_unaryops<long>();
        binaryops_ = init_binaryops<long>();
        break;
      }
      case type_id::D_FLOAT: {
        size = sizeof(float);
        relops_ = init_relops<float>();
        unaryops_ = init_unaryops<float>();
        binaryops_ = init_binaryops<float>();
        break;
      }
      case type_id::D_DOUBLE: {
        size = sizeof(double);
        relops_ = init_relops<double>();
        unaryops_ = init_unaryops<double>();
        binaryops_ = init_binaryops<double>();
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
        size(other.id),
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
