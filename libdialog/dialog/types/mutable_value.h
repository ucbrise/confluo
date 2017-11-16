#ifndef DIALOG_MUTABLE_VALUE_H_
#define DIALOG_MUTABLE_VALUE_H_

#include <cstdint>

#include "exceptions.h"
#include "immutable_value.h"
#include "types/data_types.h"

namespace dialog {

class mutable_value : public immutable_value {
 public:
  mutable_value(data_type type = NONE_TYPE)
      : immutable_value(type, type.is_none() ? nullptr : new uint8_t[type.size]) {
  }

  mutable_value(const data_type& type, immutable_raw_data value)
      : immutable_value(type, const_cast<void*>(value.ptr)) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, value);
  }

  mutable_value(const data_type& type, const void* value)
      : immutable_value(type,
                        value == nullptr ? nullptr : new uint8_t[type.size]()) {
    if (value != nullptr) {
      type_.unaryop(unary_op_id::ASSIGN)(ptr_,
                                         immutable_raw_data(value, type.size));
    }
  }

  mutable_value(bool value)
      : immutable_value(BOOL_TYPE, new uint8_t[BOOL_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, BOOL_TYPE.size));
  }

  mutable_value(int8_t value)
      : immutable_value(CHAR_TYPE, new uint8_t[CHAR_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, CHAR_TYPE.size));
  }

  mutable_value(int16_t value)
      : immutable_value(SHORT_TYPE, new uint8_t[SHORT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, SHORT_TYPE.size));
  }

  mutable_value(int32_t value)
      : immutable_value(INT_TYPE, new uint8_t[INT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, INT_TYPE.size));
  }

  mutable_value(int64_t value)
      : immutable_value(LONG_TYPE, new uint8_t[LONG_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, LONG_TYPE.size));
  }

  mutable_value(float value)
      : immutable_value(FLOAT_TYPE, new uint8_t[FLOAT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, FLOAT_TYPE.size));
  }

  mutable_value(double value)
      : immutable_value(DOUBLE_TYPE, new uint8_t[DOUBLE_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, DOUBLE_TYPE.size));
  }

  mutable_value(const std::string& str)
      : immutable_value(STRING_TYPE(str.length()), new char[str.length()]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(str.c_str(), str.length()));
  }

  mutable_value(const immutable_value& other)
      : immutable_value(other.type(), new uint8_t[other.type().size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
  }

  mutable_value(const mutable_value& other)
      : immutable_value(
          other.type_,
          other.ptr_ == nullptr ? nullptr : new uint8_t[other.type_.size]()) {
    if (other.ptr_ != nullptr) {
      type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
    }
  }

  mutable_value(mutable_value&& other)
      : immutable_value(other.type_, other.ptr_) {
    other.type_ = NONE_TYPE;
    other.ptr_ = nullptr;
  }

  mutable_value(const data_type& type, mutable_raw_data&& data)
      : immutable_value(type, data.ptr) {
    data.ptr = nullptr;
    data.size = 0;
  }

  ~mutable_value() {
    if (ptr_ != nullptr && !type_.is_none())
      delete[] reinterpret_cast<uint8_t*>(ptr_);
  }

  static mutable_value parse(const std::string& str, const data_type& type) {
    mutable_raw_data data(type.size);
    type.parse_op()(str, data);
    return mutable_value(type, std::move(data));
  }

  immutable_value copy() const {
    return immutable_value(type_, ptr_);
  }

  // Arithmetic operations
  static inline mutable_value unaryop(unary_op_id id,
                                      const immutable_value& n) {
    mutable_value result(n.type());
    result.type_.unaryop(id)(result.ptr_, n.to_data());
    return result;
  }

  friend inline mutable_value operator-(const immutable_value& n) {
    return unaryop(unary_op_id::NEGATIVE, n);
  }

  friend inline mutable_value operator+(const immutable_value& n) {
    return unaryop(unary_op_id::POSITIVE, n);
  }

  friend inline mutable_value operator~(const immutable_value& n) {
    return unaryop(unary_op_id::BW_NOT, n);
  }

  static mutable_value binaryop(binary_op_id id, const immutable_value& first,
                                const immutable_value& second) {
    if (first.type() != second.type())
      THROW(invalid_operation_exception,
            "Cannot operate on values of different types");
    mutable_value result(first.type());
    result.type_.binaryop(id)(result.ptr_, first.to_data(), second.to_data());
    return result;
  }

  friend inline mutable_value operator+(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::ADD, first, second);
  }

  friend inline mutable_value operator-(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::SUBTRACT, first, second);
  }

  friend inline mutable_value operator*(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::MULTIPLY, first, second);
  }

  friend inline mutable_value operator/(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::DIVIDE, first, second);
  }

  friend inline mutable_value operator%(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::MODULO, first, second);
  }

  friend inline mutable_value operator&(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::BW_AND, first, second);
  }

  friend inline mutable_value operator|(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::BW_OR, first, second);
  }

  friend inline mutable_value operator^(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::BW_XOR, first, second);
  }

  friend inline mutable_value operator<<(const immutable_value& first,
                                         const immutable_value& second) {
    return binaryop(binary_op_id::BW_LSHIFT, first, second);
  }

  friend inline mutable_value operator>>(const immutable_value& first,
                                         const immutable_value& second) {
    return binaryop(binary_op_id::BW_RSHIFT, first, second);
  }

  // TODO: Add more assignment operators
  mutable_value& operator=(const immutable_value& other) {
    type_ = other.type();
    if (!type_.is_none()) {
      if (ptr_ != nullptr)
        delete[] reinterpret_cast<uint8_t*>(ptr_);
      ptr_ = new uint8_t[type_.size];
      type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
    }
    return *this;
  }

  mutable_value& operator=(const mutable_value& other) {
    type_ = other.type();
    if (!type_.is_none()) {
      if (ptr_ != nullptr)
        delete[] reinterpret_cast<uint8_t*>(ptr_);
      ptr_ = new uint8_t[type_.size];
      type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
    }
    return *this;
  }

  mutable_value& operator=(mutable_value&& other) {
    if (this != &other) {
      if (ptr_ != nullptr)
        delete[] reinterpret_cast<uint8_t*>(ptr_);

      type_ = other.type_;
      ptr_ = other.ptr_;

      other.ptr_ = nullptr;
      other.type_ = NONE_TYPE;
    }

    return *this;
  }
}
;

}

#endif /* DIALOG_MUTABLE_VALUE_H_ */
