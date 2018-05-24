#include <types/type_properties.h>
#include "types/data_type.h"

namespace confluo {

data_type::data_type()
    : id(0),
      size(0) {
}

data_type::data_type(uint16_t _id, size_t _size)
    : id(_id),
      size(_size) {
  id = _id;
  size = _size;
}

data_type &data_type::operator=(const data_type &other) {
  id = other.id;
  size = other.size;
  return *this;
}

bool data_type::operator==(const data_type &other) const {
  return id == other.id && size == other.size;
}

bool data_type::operator!=(const data_type &other) const {
  return id != other.id || size != other.size;
}

void *data_type::min() const {
  return DATA_TYPES[id].min;
}

void *data_type::max() const {
  return DATA_TYPES[id].max;
}

void *data_type::one() const {
  return DATA_TYPES[id].one;
}

void *data_type::zero() const {
  return DATA_TYPES[id].zero;
}

const relational_op_t &data_type::relop(reational_op_id rid) const {
  return DATA_TYPES[id].relational_ops[rid];
}

const unary_op_t &data_type::unaryop(unary_op_id uid) const {
  return DATA_TYPES[id].unary_ops[uid];
}

const binary_op_t &data_type::binaryop(binary_op_id bid) const {
  return DATA_TYPES[id].binary_ops[bid];
}

const key_op_t &data_type::key_transform() const {
  return DATA_TYPES[id].key_transform_op;
}

const serialize_op_t &data_type::serialize_op() const {
  return DATA_TYPES[id].serialize_op;
}

const deserialize_op_t &data_type::deserialize_op() const {
  return DATA_TYPES[id].deserialize_op;
}

const parse_op_t &data_type::parse_op() const {
  return DATA_TYPES[id].parse_op;
}

const to_string_op_t &data_type::to_string_op() const {
  return DATA_TYPES[id].to_string_op;
}

bool data_type::is_valid() const {
  return id >= 1 && id < DATA_TYPES.size();
}

bool data_type::is_none() const {
  return id == 0;
}

bool data_type::is_primitive() const {
  return id >= 1 && id <= 8;
}

bool data_type::is_numeric() const {
  return DATA_TYPES[id].is_numeric;
}

std::string data_type::name() const {
  return DATA_TYPES[id].name;
}

bool data_type::is_bounded() const {
  return DATA_TYPES[id].min != nullptr && DATA_TYPES[id].max != nullptr;
}

void data_type::serialize(std::ostream &out) const {
  out.write(reinterpret_cast<const char*>(&id), sizeof(size_t));
  out.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
}

data_type data_type::deserialize(std::istream &in) {
  size_t id, size;
  in.read(reinterpret_cast<char*>(&id), sizeof(size_t));
  in.read(reinterpret_cast<char*>(&size), sizeof(size_t));
  return data_type(id, size);
}

data_type data_type::from_string(const std::string &str) {
  std::regex type_re("([a-zA-Z_]+)(?:\\(([[:digit:]]*)\\))?");
  std::smatch type_parts;
  if (std::regex_match(str, type_parts, type_re)) {
    std::string name = type_parts[1].str();
    size_t size = 0;
    if (type_parts.size() == 3 && type_parts[2].str() != "") {
      size = std::stoull(type_parts[2].str());
    }
    size_t id = find_type_properties(name);
    if (id == 0) {
      THROW(parse_exception, "Unknown type name " + str);
    }
    return data_type(id, DATA_TYPES[id].size ? DATA_TYPES[id].size : size);
  }
  THROW(parse_exception, "Malformed type name " + str);
}

}