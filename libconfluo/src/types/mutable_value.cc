#include "types/mutable_value.h"

namespace confluo {

mutable_value::mutable_value(data_type type)
    : immutable_value(type, type.is_none() ? nullptr : new uint8_t[type.size]()) {
}

mutable_value::mutable_value(const data_type &type, immutable_raw_data value)
    : immutable_value(type, const_cast<void *>(value.ptr)) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, value);
}

mutable_value::mutable_value(const data_type &type, const void *value)
    : immutable_value(type, value == nullptr ? nullptr : new uint8_t[type.size]()) {
  if (value != nullptr) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(value, type.size));
  }
}

mutable_value::mutable_value(bool value)
    : immutable_value(primitive_types::BOOL_TYPE(), new uint8_t[primitive_types::BOOL_TYPE().size]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(&value, primitive_types::BOOL_TYPE().size));
}

mutable_value::mutable_value(int8_t value)
    : immutable_value(primitive_types::CHAR_TYPE(), new uint8_t[primitive_types::CHAR_TYPE().size]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(&value, primitive_types::CHAR_TYPE().size));
}

mutable_value::mutable_value(int16_t value)
    : immutable_value(primitive_types::SHORT_TYPE(), new uint8_t[primitive_types::SHORT_TYPE().size]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(&value, primitive_types::SHORT_TYPE().size));
}

mutable_value::mutable_value(int32_t value)
    : immutable_value(primitive_types::INT_TYPE(), new uint8_t[primitive_types::INT_TYPE().size]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(&value, primitive_types::INT_TYPE().size));
}

mutable_value::mutable_value(int64_t value)
    : immutable_value(primitive_types::LONG_TYPE(), new uint8_t[primitive_types::LONG_TYPE().size]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(&value, primitive_types::LONG_TYPE().size));
}

mutable_value::mutable_value(float value)
    : immutable_value(primitive_types::FLOAT_TYPE(), new uint8_t[primitive_types::FLOAT_TYPE().size]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(&value, primitive_types::FLOAT_TYPE().size));
}

mutable_value::mutable_value(double value)
    : immutable_value(primitive_types::DOUBLE_TYPE(), new uint8_t[primitive_types::DOUBLE_TYPE().size]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(&value, primitive_types::DOUBLE_TYPE().size));
}

mutable_value::mutable_value(const std::string &str)
    : immutable_value(primitive_types::STRING_TYPE(str.length()), new char[str.length()]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, immutable_raw_data(str.c_str(), str.length()));
}

mutable_value::mutable_value(const immutable_value &other)
    : immutable_value(other.type(), new uint8_t[other.type().size]()) {
  type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
}

mutable_value::mutable_value(const mutable_value &other)
    : immutable_value(
    other.type_,
    other.ptr_ == nullptr ? nullptr : new uint8_t[other.type_.size]()) {
  if (other.ptr_ != nullptr) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
  }
}

mutable_value::mutable_value(mutable_value &&other)
    : immutable_value(other.type_, other.ptr_) {
  other.type_ = primitive_types::NONE_TYPE();
  other.ptr_ = nullptr;
}

mutable_value::~mutable_value() {
  if (ptr_ != nullptr && !type_.is_none())
    delete[] reinterpret_cast<uint8_t *>(ptr_);
}

mutable_value mutable_value::parse(const std::string &str, const data_type &type) {
  mutable_value value(type);
  type.parse_op()(str, value.ptr_);
  return value;
}

immutable_value mutable_value::copy() const {
  return immutable_value(type_, ptr_);
}

mutable_value mutable_value::unaryop(unary_op_id id, const immutable_value &n) {
  mutable_value result(n.type());
  result.type_.unaryop(id)(result.ptr_, n.to_data());
  return result;
}

mutable_value operator-(const immutable_value &n) {
  return mutable_value::unaryop(unary_op_id::NEGATIVE, n);
}

mutable_value operator+(const immutable_value &n) {
  return mutable_value::unaryop(unary_op_id::POSITIVE, n);
}

mutable_value operator~(const immutable_value &n) {
  return mutable_value::unaryop(unary_op_id::BW_NOT, n);
}

mutable_value mutable_value::binaryop(binary_op_id id, const immutable_value &first, const immutable_value &second) {
  if (first.type() != second.type())
    THROW(invalid_operation_exception, "Cannot operate on values of different types");
  mutable_value result(first.type());
  result.type_.binaryop(id)(result.ptr_, first.to_data(), second.to_data());
  return result;
}

mutable_value operator+(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::ADD, first, second);
}

mutable_value operator-(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::SUBTRACT, first, second);
}

mutable_value operator*(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::MULTIPLY, first, second);
}

mutable_value operator/(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::DIVIDE, first, second);
}

mutable_value operator%(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::MODULO, first, second);
}

mutable_value operator&(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::BW_AND, first, second);
}

mutable_value operator|(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::BW_OR, first, second);
}

mutable_value operator^(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::BW_XOR, first, second);
}

mutable_value operator<<(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::BW_LSHIFT, first, second);
}

mutable_value operator>>(const immutable_value &first, const immutable_value &second) {
  return mutable_value::binaryop(binary_op_id::BW_RSHIFT, first, second);
}

mutable_value &mutable_value::operator=(const immutable_value &other) {
  type_ = other.type();
  if (!type_.is_none()) {
    if (ptr_ != nullptr)
      delete[] reinterpret_cast<uint8_t *>(ptr_);
    ptr_ = new uint8_t[type_.size];
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
  }
  return *this;
}

mutable_value &mutable_value::operator=(const mutable_value &other) {
  type_ = other.type();
  if (!type_.is_none()) {
    if (ptr_ != nullptr)
      delete[] reinterpret_cast<uint8_t *>(ptr_);
    ptr_ = new uint8_t[type_.size];
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
  }
  return *this;
}

mutable_value &mutable_value::operator=(mutable_value &&other) {
  if (this != &other) {
    if (ptr_ != nullptr)
      delete[] reinterpret_cast<uint8_t *>(ptr_);

    type_ = other.type_;
    ptr_ = other.ptr_;

    other.ptr_ = nullptr;
    other.type_ = primitive_types::NONE_TYPE();
  }

  return *this;
}

}