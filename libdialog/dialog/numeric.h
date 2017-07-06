#ifndef DIALOG_NUMERIC_H_
#define DIALOG_NUMERIC_H_

#include <cstdint>

#include "data_types.h"
#include "exceptions.h"

namespace dialog {

class numeric_t {
 public:
  using storage = typename std::aligned_storage<sizeof(uint64_t), alignof(uint64_t)>::type;

  numeric_t(data_type type)
      : type_(type) {
    // Is this safe to do?
    *(reinterpret_cast<uint64_t*>(storage_)) = 0;
  }

  numeric_t(data_type type, const void* value)
      : type_(type) {
    type.unaryop(unaryop_id::ASSIGN)(storage_, value);
  }

  // Relational operators
  friend bool relop(relop_id id, const numeric_t& first,
                    const numeric_t& second) const {
    if (first.type_ != second.type_)
      throw invalid_operation_exception(
          "Cannot compare values of different types");
    return type_.relop(id)(first.storage_, second.storage_);
  }

  friend bool operator <(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::LT, first, second);
  }

  friend bool operator <=(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::LT, first, second);
  }

  friend bool operator >(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::LT, first, second);
  }

  friend bool operator >=(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::LT, first, second);
  }

  friend bool operator ==(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::LT, first, second);
  }

  friend bool operator !=(const numeric_t& first, const numeric_t& second) {
    return relop(relop_id::LT, first, second);
  }

  friend numeric_t unaryop(unaryop_id id, const numeric_t& n) {
    numeric_t result;
    type_.unaryop(id)(result.storage_, n.storage_);
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
    numeric_t result;
    if (first.type_ != second.type_)
      throw invalid_operation_exception(
          "Cannot operate on values of different types");
    type_.binaryop(id)(result.storage_, first.storage_, second.storage_);
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

  // Assignment operator
  numeric_t& operator=(const numeric_t& other) {
    type_.unaryop(unaryop_id::ASSIGN)(storage_, other.storage_);
    return *this;
  }

 private:
  data_type type_;
  storage storage_;
};

}

#endif /* DIALOG_NUMERIC_H_ */
