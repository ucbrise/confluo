#ifndef DIALOG_MUTABLE_VALUE_H_
#define DIALOG_MUTABLE_VALUE_H_

#include <cstdint>

#include "immutable_value.h"
#include "data_types.h"
#include "exceptions.h"

namespace dialog {

class mutable_value_t : public immutable_value_t {
 public:
  mutable_value_t(data_type type = NONE_TYPE)
      : immutable_value_t(
          type, type.id == type_id::D_NONE ? nullptr : new uint8_t[type.size]) {
  }

  mutable_value_t(const data_type& type, const void* value)
      : immutable_value_t(type, new uint8_t[type.size]) {
    type.unaryop(unaryop_id::ASSIGN)(data_, value);
  }

  mutable_value_t(bool value)
      : immutable_value_t(BOOL_TYPE, new uint8_t[BOOL_TYPE.size]) {
    type_.unaryop(unaryop_id::ASSIGN)(data_, &value);
  }

  mutable_value_t(char value)
      : immutable_value_t(CHAR_TYPE, new uint8_t[CHAR_TYPE.size]) {
    type_.unaryop(unaryop_id::ASSIGN)(data_, &value);
  }

  mutable_value_t(short value)
      : immutable_value_t(SHORT_TYPE, new uint8_t[SHORT_TYPE.size]) {
    type_.unaryop(unaryop_id::ASSIGN)(data_, &value);
  }

  mutable_value_t(int value)
      : immutable_value_t(INT_TYPE, new uint8_t[INT_TYPE.size]) {
    type_.unaryop(unaryop_id::ASSIGN)(data_, &value);
  }

  mutable_value_t(long value)
      : immutable_value_t(LONG_TYPE, new uint8_t[LONG_TYPE.size]) {
    type_.unaryop(unaryop_id::ASSIGN)(data_, &value);
  }

  mutable_value_t(float value)
      : immutable_value_t(FLOAT_TYPE, new uint8_t[FLOAT_TYPE.size]) {
    type_.unaryop(unaryop_id::ASSIGN)(data_, &value);
  }

  mutable_value_t(double value)
      : immutable_value_t(DOUBLE_TYPE, new uint8_t[DOUBLE_TYPE.size]) {
    type_.unaryop(unaryop_id::ASSIGN)(data_, &value);
  }

  mutable_value_t(const mutable_value_t& other)
      : immutable_value_t(other.type_, new uint8_t[other.type_.size]) {
    type_.unaryop(unaryop_id::ASSIGN)(data_, other.data_);
  }

  mutable_value_t(mutable_value_t&& other)
      : immutable_value_t(other.type_, other.data_) {
    other.type_ = NONE_TYPE;
    other.data_ = nullptr;
  }

  ~mutable_value_t() {
    if (data_ != nullptr && type_.id != type_id::D_NONE)
      delete[] reinterpret_cast<uint8_t*>(data_);
  }

  // Arithmetic operations
  static inline mutable_value_t unaryop(unaryop_id id,
                                        const immutable_value_t& n) {
    mutable_value_t result(n.type());
    result.type_.unaryop(id)(result.data_, n.data());
    return result;
  }

  friend inline mutable_value_t operator-(const immutable_value_t& n) {
    return unaryop(unaryop_id::NEGATIVE, n);
  }

  friend inline mutable_value_t operator+(const immutable_value_t& n) {
    return unaryop(unaryop_id::POSITIVE, n);
  }

  friend inline mutable_value_t operator~(const immutable_value_t& n) {
    return unaryop(unaryop_id::BW_NOT, n);
  }

  static mutable_value_t binaryop(binaryop_id id,
                                  const immutable_value_t& first,
                                  const immutable_value_t& second) {
    if (first.type() != second.type())
      THROW(invalid_operation_exception,
            "Cannot operate on values of different types");
    mutable_value_t result(first.type());
    result.type_.binaryop(id)(result.data_, first.data(), second.data());
    return result;
  }

  friend inline mutable_value_t operator+(const immutable_value_t& first,
                                          const immutable_value_t& second) {
    return binaryop(binaryop_id::ADD, first, second);
  }

  friend inline mutable_value_t operator-(const immutable_value_t& first,
                                          const immutable_value_t& second) {
    return binaryop(binaryop_id::SUBTRACT, first, second);
  }

  friend inline mutable_value_t operator*(const immutable_value_t& first,
                                          const immutable_value_t& second) {
    return binaryop(binaryop_id::MULTIPLY, first, second);
  }

  friend inline mutable_value_t operator/(const immutable_value_t& first,
                                          const immutable_value_t& second) {
    return binaryop(binaryop_id::DIVIDE, first, second);
  }

  friend inline mutable_value_t operator%(const immutable_value_t& first,
                                          const immutable_value_t& second) {
    return binaryop(binaryop_id::MODULO, first, second);
  }

  friend inline mutable_value_t operator&(const immutable_value_t& first,
                                          const immutable_value_t& second) {
    return binaryop(binaryop_id::BW_AND, first, second);
  }

  friend inline mutable_value_t operator|(const immutable_value_t& first,
                                          const immutable_value_t& second) {
    return binaryop(binaryop_id::BW_OR, first, second);
  }

  friend inline mutable_value_t operator^(const immutable_value_t& first,
                                          const immutable_value_t& second) {
    return binaryop(binaryop_id::BW_XOR, first, second);
  }

  friend inline mutable_value_t operator<<(const immutable_value_t& first,
                                           const immutable_value_t& second) {
    return binaryop(binaryop_id::BW_LSHIFT, first, second);
  }

  friend inline mutable_value_t operator>>(const immutable_value_t& first,
                                           const immutable_value_t& second) {
    return binaryop(binaryop_id::BW_RSHIFT, first, second);
  }

  mutable_value_t& operator=(const immutable_value_t& other) {
    type_ = other.type();
    if (type_.id != type_id::D_NONE) {
      if (data_ != nullptr)
        delete[] reinterpret_cast<uint8_t*>(data_);
      data_ = new uint8_t[type_.size];
      type_.unaryop(unaryop_id::ASSIGN)(data_, other.data());
    }
    return *this;
  }

  mutable_value_t& operator=(const mutable_value_t& other) {
    type_ = other.type();
    if (type_.id != type_id::D_NONE) {
      if (data_ != nullptr)
        delete[] reinterpret_cast<uint8_t*>(data_);
      data_ = new uint8_t[type_.size];
      type_.unaryop(unaryop_id::ASSIGN)(data_, other.data());
    }
    return *this;
  }

  mutable_value_t& operator=(mutable_value_t&& other) {
    if (this != &other) {
      if (data_ != nullptr)
        delete[] reinterpret_cast<uint8_t*>(data_);

      type_ = other.type_;
      data_ = other.data_;

      other.data_ = nullptr;
      other.type_ = NONE_TYPE;
    }

    return *this;
  }

  template<typename T>
  T as() {
    return *reinterpret_cast<T*>(data_);
  }
};

}

#endif /* DIALOG_MUTABLE_VALUE_H_ */
