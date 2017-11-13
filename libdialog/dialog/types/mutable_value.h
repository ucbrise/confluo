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
      : immutable_value(
          type, type.id == type_id::D_NONE ? nullptr : new uint8_t[type.size]) {
  }

  mutable_value(const data_type& type, data value) 
      : immutable_value(type, const_cast<void*>(value.ptr)) {
      type_.unaryop(unary_op_id::ASSIGN)(ptr_, value);
  }

  mutable_value(const data_type& type, const void* value)
      : immutable_value(type, new uint8_t[type.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(value, type.size));
  }

  mutable_value(bool value)
      : immutable_value(BOOL_TYPE, new uint8_t[BOOL_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(&value, BOOL_TYPE.size));
  }

  mutable_value(int8_t value)
      : immutable_value(CHAR_TYPE, new uint8_t[CHAR_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(&value, CHAR_TYPE.size));
  }

  mutable_value(int16_t value)
      : immutable_value(SHORT_TYPE, new uint8_t[SHORT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(&value, SHORT_TYPE.size));
  }

  mutable_value(int32_t value)
      : immutable_value(INT_TYPE, new uint8_t[INT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(&value, INT_TYPE.size));
  }

  mutable_value(int64_t value)
      : immutable_value(LONG_TYPE, new uint8_t[LONG_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(&value, LONG_TYPE.size));
  }

  mutable_value(float value)
      : immutable_value(FLOAT_TYPE, new uint8_t[FLOAT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(&value, FLOAT_TYPE.size));
  }

  mutable_value(double value)
      : immutable_value(DOUBLE_TYPE, new uint8_t[DOUBLE_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(&value, DOUBLE_TYPE.size));
  }

  mutable_value(const std::string& str)
      : immutable_value(STRING_TYPE(str.length()), new char[str.length()]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, data(str.c_str(), str.length()));
  }

  mutable_value(const immutable_value& other)
      : immutable_value(other.type(), new uint8_t[other.type().size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
  }

  mutable_value(const mutable_value& other)
      : immutable_value(other.type_, new uint8_t[other.type_.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
  }

  mutable_value(mutable_value&& other)
      : immutable_value(other.type_, other.ptr_) {
    other.type_ = NONE_TYPE;
    other.ptr_ = nullptr;
  }

  ~mutable_value() {
    if (ptr_ != nullptr && type_.id != type_id::D_NONE)
      delete[] reinterpret_cast<uint8_t*>(ptr_);
  }

  static mutable_value parse(const std::string& str, const data_type& type) {
    if (!type_manager::is_primitive(type.id)
            && type_manager::is_valid_id(type.id)) {
        return mutable_value(type, type.parse_op()(str));
    }
    switch (type.id) {
      case type_id::D_BOOL: {
        bool val = string_utils::lexical_cast<bool>(str);
        return mutable_value(val);
      }
      case type_id::D_CHAR: {
        int8_t val = string_utils::lexical_cast<int8_t>(str);
        return mutable_value(val);
      }
      case type_id::D_SHORT: {
        int16_t val = string_utils::lexical_cast<int16_t>(str);
        return mutable_value(val);
      }
      case type_id::D_INT: {
        int32_t val = string_utils::lexical_cast<int32_t>(str);
        return mutable_value(val);
      }
      case type_id::D_LONG: {
        int64_t val = string_utils::lexical_cast<int64_t>(str);
        return mutable_value(val);
      }
      case type_id::D_FLOAT: {
        float val = string_utils::lexical_cast<float>(str);
        return mutable_value(val);
      }
      case type_id::D_DOUBLE: {
        double val = string_utils::lexical_cast<double>(str);
        return mutable_value(val);
      }
      case type_id::D_STRING: {
        std::string t_str = str;
        t_str.resize(type.size);
        return mutable_value(type, t_str.c_str());
      }
      default: {
        THROW(parse_exception,
              "Could not parse value " + str + " to type " + type.to_string());
      }
    }
    
  }

  immutable_value copy() const {
    return immutable_value(type_, ptr_);
  }

  // Arithmetic operations
  static inline mutable_value unaryop(unary_op_id id, const immutable_value& n) {
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
    if (type_.id != type_id::D_NONE) {
      if (ptr_ != nullptr)
        delete[] reinterpret_cast<uint8_t*>(ptr_);
      ptr_ = new uint8_t[type_.size];
      type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
    }
    return *this;
  }

  mutable_value& operator=(const mutable_value& other) {
    type_ = other.type();
    if (type_.id != type_id::D_NONE) {
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
