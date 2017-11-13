#ifndef DIALOG_DATA_TYPES_H_
#define DIALOG_DATA_TYPES_H_

#include <limits>
#include <string>
#include <array>
#include <cstdint>
#include <regex>
#include <cstring>

#include "arithmetic_ops.h"
#include "exceptions.h"
#include "key_ops.h"
#include "relational_ops.h"
#include "string_ops.h"
#include "serde_ops.h"
#include "string_utils.h"
#include "primitive_types.h"
#include "data.h"

namespace dialog {

static std::vector<void*> init_min() {
  return {nullptr, &limits::bool_min, &limits::char_min, &limits::short_min,
    &limits::int_min, &limits::long_min, &limits::float_min,
    &limits::double_min, nullptr};
}

static std::vector<void*> init_max() {
  return {nullptr, &limits::bool_max, &limits::char_max, &limits::short_max,
    &limits::int_max, &limits::long_max, &limits::float_max,
    &limits::double_max, nullptr};
}

static std::vector<void*> init_one() {
  return {nullptr, &limits::bool_one, &limits::char_one, &limits::short_one,
    &limits::int_one, &limits::long_one, &limits::float_one,
    &limits::double_one, nullptr};
}

static std::vector<void*> init_zero() {
  return {nullptr, &limits::bool_zero, &limits::char_zero, &limits::short_zero,
    &limits::int_zero, &limits::long_zero, &limits::float_zero,
    &limits::double_zero, nullptr};
}

static std::vector<rel_ops_t> init_rops() {
  return {init_relops<void>(), init_relops<bool>(), init_relops<int8_t>(),
    init_relops<int16_t>(), init_relops<int32_t>(), init_relops<int64_t>(),
    init_relops<float>(), init_relops<double>(), init_relops<std::string>()};
}

static std::vector<unary_ops_t> init_uops() {
  return {init_unaryops<void>(), init_unaryops<bool>(), init_unaryops<int8_t>(),
    init_unaryops<int16_t>(), init_unaryops<int32_t>(), init_unaryops<int64_t>(),
    init_unaryops<float>(), init_unaryops<double>(), init_unaryops<std::string>()};
}

static std::vector<binary_ops_t> init_bops() {
  return {init_binaryops<void>(), init_binaryops<bool>(), init_binaryops<int8_t>(),
    init_binaryops<int16_t>(), init_binaryops<int32_t>(), init_binaryops<int64_t>(),
    init_binaryops<float>(), init_binaryops<double>(), init_binaryops<std::string>()};
}

static std::vector<key_op> init_kops() {
  return {key_transform<void>, key_transform<bool>, key_transform<int8_t>,
    key_transform<int16_t>, key_transform<int32_t>, key_transform<int64_t>,
    key_transform<float>, key_transform<double>, key_transform<std::string>};
}

static std::vector<std::string> init_to_strings() {
  return {"none", "bool", "char", "short", "int", "long", "float", "double",
    "string"};
}

static std::vector<size_t> primitive_sizes() {
  return {0, sizeof(bool), sizeof(int8_t), sizeof(int16_t),
    sizeof(int32_t), sizeof(int64_t), sizeof(float), sizeof(double)};
}

static std::vector<void*> MIN = init_min();
static std::vector<void*> MAX = init_max();
static std::vector<void*> ONE = init_one();
static std::vector<void*> ZERO = init_zero();
static std::vector<rel_ops_t> RELOPS = init_rops();
static std::vector<unary_ops_t> UNOPS = init_uops();
static std::vector<binary_ops_t> BINOPS = init_bops();
static std::vector<key_op> KEYOPS = init_kops();
static std::vector<std::string> TO_STRINGS = init_to_strings();
static std::vector<size_t> PRIMITIVE_SIZES = primitive_sizes();
static std::vector<from_string_op> PARSERS = init_parsers();
static std::vector<serialize_op> SERIALIZERS = init_serializers();
static std::vector<deserialize_op> DESERIALIZERS = init_deserializers();

struct data_type {
 public:
  uint16_t id;
  size_t size;

  bool is_primitive() {
    return id >= 0 && id <= 8;
  }

  data_type(type_id _id = type_id::D_NONE, size_t _size = 0)
      : id(_id),
        size(_size) {
    uint16_t string_id = 8;

    if (!is_primitive()) {
      THROW(invalid_operation_exception,
            "Must specify length for non-primitive data types");
    } else if (id != string_id) {
      size = PRIMITIVE_SIZES[id];
    }
  }

  data_type(uint16_t _id, size_t _size) {
    id = _id;
    size = _size;
  }

  void* min() const {
    return MIN[id];
  }

  void* max() const {
    return MAX[id];
  }

  void* one() const {
    return ONE[id];
  }

  void* zero() const {
    return ZERO[id];
  }

  inline data_type& operator=(const data_type& other) {
    id = other.id;
    size = other.size;
    return *this;
  }

  inline bool operator==(const data_type& other) const {
    return id == other.id && size == other.size;
  }

  inline bool operator!=(const data_type& other) const {
    return id != other.id || size != other.size;
  }

  inline const relational_fn& relop(relop_id rid) const {
    return RELOPS[id][rid];
  }

  inline const unary_fn& unaryop(unaryop_id uid) const {
    return UNOPS[id][uid];
  }

  inline const binary_fn& binaryop(binaryop_id bid) const {
    return BINOPS[id][bid];
  }

  inline const key_op& keytransform() const {
    return KEYOPS[id];
  }

  inline bool is_primitive() const {
    return id >= 0 && id <= 8;
  }

  inline std::string to_string() const {
    return TO_STRINGS[id];
  }

  inline static data_type from_string(const std::string& str) {
    std::string tstr = utils::string_utils::to_upper(str);
    std::regex str_re("STRING\\(([0-9]+)\\)");
    std::smatch str_matches;
    if (tstr == "BOOL") {
      return data_type(1, sizeof(bool));
    } else if (tstr == "CHAR") {
      return data_type(2, sizeof(int8_t));
    } else if (tstr == "SHORT") {
      return data_type(3, sizeof(int16_t));
    } else if (tstr == "INT") {
      return data_type(4, sizeof(int32_t));
    } else if (tstr == "LONG") {
      return data_type(5, sizeof(int64_t));
    } else if (tstr == "FLOAT") {
      return data_type(6, sizeof(float));
    } else if (tstr == "DOUBLE") {
      return data_type(7, sizeof(double));
    } else if (std::regex_search(tstr, str_matches, str_re)) {
      return data_type(8, std::stoul(str_matches[1].str()));
    }
    return data_type(0, 0);
  }
};

// type-ids 1-7 are numeric
static inline bool is_numeric(const data_type& type) {
  return type.id >= 1 && type.id <= 7;
}

}

#endif /* DIALOG_DATA_TYPES_H_ */
