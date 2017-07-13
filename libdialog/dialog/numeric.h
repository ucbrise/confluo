#ifndef DIALOG_NUMERIC_H_
#define DIALOG_NUMERIC_H_

#include <cstdint>

#include "data_types.h"
#include "exceptions.h"

namespace dialog {

class numeric_t {
 public:
  numeric_t(const data_type& type = NONE_TYPE)
      : type_(type) {
  }

  numeric_t(const data_type& type, const void* value)
      : type_(type) {
    if (type.id != type_id::D_NONE)
      type.unaryop(unaryop_id::ASSIGN)(storage_, value);
  }

  numeric_t(bool value)
      : type_(BOOL_TYPE) {
    type_.unaryop(unaryop_id::ASSIGN)(storage_, &value);
  }

  numeric_t(char value)
      : type_(CHAR_TYPE) {
    type_.unaryop(unaryop_id::ASSIGN)(storage_, &value);
  }

  numeric_t(short value)
      : type_(SHORT_TYPE) {
    type_.unaryop(unaryop_id::ASSIGN)(storage_, &value);
  }

  numeric_t(int value)
      : type_(INT_TYPE) {
    type_.unaryop(unaryop_id::ASSIGN)(storage_, &value);
  }

  numeric_t(long value)
      : type_(LONG_TYPE) {
    type_.unaryop(unaryop_id::ASSIGN)(storage_, &value);
  }

  numeric_t(float value)
      : type_(FLOAT_TYPE) {
    type_.unaryop(unaryop_id::ASSIGN)(storage_, &value);
  }

  numeric_t(double value)
      : type_(DOUBLE_TYPE) {
    type_.unaryop(unaryop_id::ASSIGN)(storage_, &value);
  }

  numeric_t(const numeric_t& other)
      : type_(other.type_) {
    *reinterpret_cast<uint64_t*>(storage_) =
        *reinterpret_cast<const uint64_t*>(other.storage_);
  }

  const data_type& get_type() const {
    return type_;
  }

  const void* get_data() const {
    return reinterpret_cast<const void*>(storage_);
  }

  void* get_data() {
    return reinterpret_cast<void*>(storage_);
  }

  // Relational operators
  friend bool relop(relop_id id, const numeric_t& first,
                    const numeric_t& second) {
    if (first.type_ != second.type_)
      THROW(invalid_operation_exception,
            "Cannot compare values of different types");
    return first.type_.relop(id)(first.storage_, second.storage_);
  }

  friend bool operator <(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::LT, first, second);
  }

  friend bool operator <=(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::LE, first, second);
  }

  friend bool operator >(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::GT, first, second);
  }

  friend bool operator >=(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::GE, first, second);
  }

  friend bool operator ==(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::EQ, first, second);
  }

  friend bool operator !=(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::NEQ, first, second);
  }

  friend numeric_t unaryop(unaryop_id id, const numeric_t& n) {
    numeric_t result(n.type_);
    result.type_.unaryop(id)(result.storage_, n.storage_);
    return result;
  }

  friend numeric_t operator-(const numeric_t& n) {
    return unaryop(unaryop_id::NEGATIVE, n);
  }

  friend numeric_t operator+(const numeric_t& n) {
    return unaryop(unaryop_id::POSITIVE, n);
  }

  friend numeric_t operator~(const numeric_t& n) {
    return unaryop(unaryop_id::BW_NOT, n);
  }

  friend numeric_t binaryop(binaryop_id id, const numeric_t& first,
                            const numeric_t& second) {
    if (first.type_ != second.type_)
      THROW(invalid_operation_exception,
            "Cannot operate on values of different types");
    numeric_t result(first.type_);
    result.type_.binaryop(id)(result.storage_, first.storage_, second.storage_);
    return result;
  }

  friend numeric_t operator+(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::ADD, first, second);
  }

  friend numeric_t operator-(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::SUBTRACT, first, second);
  }

  friend numeric_t operator*(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::MULTIPLY, first, second);
  }

  friend numeric_t operator/(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::DIVIDE, first, second);
  }

  friend numeric_t operator%(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::MODULO, first, second);
  }

  friend numeric_t operator&(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::BW_AND, first, second);
  }

  friend numeric_t operator|(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::BW_OR, first, second);
  }

  friend numeric_t operator^(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::BW_XOR, first, second);
  }

  friend numeric_t operator<<(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::BW_LSHIFT, first, second);
  }

  friend numeric_t operator>>(const numeric_t& first, const numeric_t& second) {
    return binaryop(binaryop_id::BW_RSHIFT, first, second);
  }

  numeric_t& operator=(const numeric_t& other) {
    type_ = other.type_;
    if (type_.id != type_id::D_NONE)
      type_.unaryop(unaryop_id::ASSIGN)(storage_, other.storage_);
    return *this;
  }

  template<typename T>
  T as() {
    return *reinterpret_cast<T*>(storage_);
  }

 private:
  data_type type_;
  unsigned char storage_[8];
};

}

#endif /* DIALOG_NUMERIC_H_ */
