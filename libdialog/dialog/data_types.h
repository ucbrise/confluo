#ifndef DIALOG_DATA_TYPES_H_
#define DIALOG_DATA_TYPES_H_

#include <string>
#include <array>
#include <cstdint>

#include "relational_ops.h"
#include "arithmetic_ops.h"

namespace dialog {

enum type_id
  : uint16_t {
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
  type_id id;
  size_t size;
  bool indexable;
  rel_ops_t relops;
  unary_ops_t unaryops;
  binary_ops_t binaryops;

  inline void set_size(size_t sz) {
    size = sz;
  }

  inline bool operator==(const data_type& other) const {
    return id == other.id && size == other.size;
  }

  inline bool operator!=(const data_type& other) const {
    return id != other.id || size != other.size;
  }

  inline const relational_fn& relop(relop_id id) const {
    return relops[id];
  }

  inline const unary_fn& unaryop(unaryop_id id) const {
    return unaryops[id];
  }

  inline const binary_fn& binaryop(binaryop_id id) const {
    return binaryops[id];
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
};

static std::array<data_type, 8> init_types() {
  std::array<data_type, 8> arr;
  arr[type_id::D_BOOL] = {type_id::D_BOOL, sizeof(bool), true, init_relops<bool>(), init_unaryops<bool>(), init_binaryops<bool>()};
  arr[type_id::D_CHAR] = {type_id::D_CHAR, sizeof(char), true, init_relops<char>(), init_unaryops<char>(), init_binaryops<char>()};
  arr[type_id::D_SHORT] = {type_id::D_SHORT, sizeof(short), true, init_relops<short>(), init_unaryops<short>(), init_binaryops<short>()};
  arr[type_id::D_INT] = {type_id::D_INT, sizeof(int), true, init_relops<int>(), init_unaryops<int>(), init_binaryops<int>()};
  arr[type_id::D_LONG] = {type_id::D_LONG, sizeof(long), true, init_relops<long>(), init_unaryops<long>(), init_binaryops<long>()};
  arr[type_id::D_FLOAT] = {type_id::D_FLOAT, sizeof(float), true, init_relops<float>(), init_unaryops<float>(), init_binaryops<float>()};
  arr[type_id::D_DOUBLE] = {type_id::D_DOUBLE, sizeof(double), true, init_relops<double>(), init_unaryops<double>(), init_binaryops<double>()};
  return arr;
}

static std::array<data_type, 8> data_types = init_types();

static data_type string_type(size_t size) {
  return {type_id::D_STRING, size, false, init_relops<std::string>(), init_unaryops<std::string>(), init_binaryops<std::string>()};
}

static data_type bool_type() {
  return data_types[type_id::D_BOOL];
}

static data_type char_type() {
  return data_types[type_id::D_CHAR];
}

static data_type short_type() {
  return data_types[type_id::D_SHORT];
}

static data_type int_type() {
  return data_types[type_id::D_INT];
}

static data_type long_type() {
  return data_types[type_id::D_LONG];
}

static data_type float_type() {
  return data_types[type_id::D_FLOAT];
}

static data_type double_type() {
  return data_types[type_id::D_DOUBLE];
}

}

#endif /* DIALOG_DATA_TYPES_H_ */
