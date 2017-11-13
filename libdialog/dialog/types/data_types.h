#ifndef DIALOG_DATA_TYPES_H_
#define DIALOG_DATA_TYPES_H_

#include <limits>
#include <string>
#include <array>
#include <cstdint>
#include <regex>
#include <cstring>

#include "primitive_types.h"
#include "type_properties.h"

namespace dialog {

struct data_type {
 public:
  size_t id;
  size_t size;

  data_type()
      : id(0),
        size(0) {
  }

  data_type(uint16_t _id, size_t _size)
      : id(_id),
        size(_size) {
    id = _id;
    size = _size;
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

  void* min() const {
    return DATA_TYPES[id].min;
  }

  void* max() const {
    return DATA_TYPES[id].max;
  }

  void* one() const {
    return DATA_TYPES[id].one;
  }

  void* zero() const {
    return DATA_TYPES[id].zero;
  }

  inline const relational_op_t& relop(reational_op_id rid) const {
    return DATA_TYPES[id].relational_ops[rid];
  }

  inline const unary_op_t& unaryop(unary_op_id uid) const {
    return DATA_TYPES[id].unary_ops[uid];
  }

  inline const binary_op_t& binaryop(binary_op_id bid) const {
    return DATA_TYPES[id].binary_ops[bid];
  }

  inline const key_op_t& key_transform() const {
    return DATA_TYPES[id].key_transform_op;
  }

  inline const serialize_op_t& serialize_op() const {
    return DATA_TYPES[id].serialize_op;
  }

  inline const deserialize_op_t& deserialize_op() const {
    return DATA_TYPES[id].deserialize_op;
  }

  inline const parse_op_t& parse_op() const {
    return DATA_TYPES[id].parse_op;
  }

  inline bool is_primitive() const {
    return id >= 1 && id <= 8;
  }

  inline std::string to_string() const {
    return DATA_TYPES[id].name;
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
