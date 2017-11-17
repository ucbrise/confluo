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

namespace confluo {

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

  inline const to_string_op_t& to_string_op() const {
    return DATA_TYPES[id].to_string_op;
  }

  inline bool is_valid() const {
    return id >= 1 && id < DATA_TYPES.size();
  }

  inline bool is_none() const {
    return id == 0;
  }

  inline bool is_primitive() const {
    return id >= 1 && id <= 8;
  }

  inline bool is_numeric() const {
    return DATA_TYPES[id].is_numeric;
  }

  inline std::string name() const {
    return DATA_TYPES[id].name;
  }

  inline bool is_bounded() const {
    return DATA_TYPES[id].min != nullptr && DATA_TYPES[id].max != nullptr;
  }

  inline void serialize(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&id), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
  }

  inline static data_type deserialize(std::istream& in) {
    size_t id, size;
    in.read(reinterpret_cast<char*>(&id), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&size), sizeof(size_t));
    return data_type(id, size);
  }

  inline static data_type from_string(const std::string& str) {
    std::regex type_re("([a-zA-Z_]+)(?:\\(([[:digit:]]*)\\))?");
    std::smatch type_parts;
    if (std::regex_match(str, type_parts, type_re)) {
      std::string name = type_parts[1].str();
      size_t size = 0;
      if (type_parts.size() == 3 && type_parts[2].str() != "") {
        size = std::stoull(type_parts[2].str());
      }
      size_t id = find_type_properties(name);
      return data_type(id, DATA_TYPES[id].size ? DATA_TYPES[id].size : size);
    }
    THROW(parse_exception, "Malformed type name " + str);
  }
};

}

#endif /* DIALOG_DATA_TYPES_H_ */
