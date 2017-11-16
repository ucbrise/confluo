#ifndef DIALOG_NUMERIC_H_
#define DIALOG_NUMERIC_H_

#include "immutable_value.h"
#include "string_utils.h"
#include "types/data_types.h"

namespace dialog {

class numeric {
 public:
  static const size_t MAX_SIZE = sizeof(uint64_t);

  numeric()
      : type_(NONE_TYPE) {
  }

  numeric(data_type type)
      : type_(type) {
    memcpy(data_, type_.zero(), type_.size);
  }

  numeric(bool val)
      : type_(BOOL_TYPE) {
    as<bool>() = val;
  }

  numeric(int8_t val)
      : type_(CHAR_TYPE) {
    as<int8_t>() = val;
  }

  numeric(int16_t val)
      : type_(SHORT_TYPE) {
    as<int16_t>() = val;
  }

  numeric(int32_t val)
      : type_(INT_TYPE) {
    as<int32_t>() = val;
  }

  numeric(int64_t val)
      : type_(LONG_TYPE) {
    as<int64_t>() = val;
  }

  numeric(float val)
      : type_(FLOAT_TYPE) {
    as<float>() = val;
  }

  numeric(double val)
      : type_(DOUBLE_TYPE) {
    as<double>() = val;
  }

  numeric(const data_type& type, void* data)
      : type_(type) {
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    memcpy(data_, data, type_.size);
  }

  numeric(const immutable_value& val)
      : type_(val.type()) {
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    memcpy(data_, val.ptr(), type_.size);
  }

  static numeric parse(const std::string& str, const data_type& type) {
    mutable_raw_data data(type.size);
    type.parse_op()(str, data);
    return numeric(type, data.ptr);
  }

  inline immutable_raw_data to_data() const {
    return immutable_raw_data(data_, type_.size);
  }

  data_type const& type() const {
    return type_;
  }

  // Relational operators
  static bool relop(reational_op_id id, const numeric& first,
                    const numeric& second) {
    if (first.type_ != second.type_)
      THROW(
          invalid_operation_exception,
          "Comparing values of different types: (" + first.type().name() + ", "
              + second.type().name() + ")");
    return first.type_.relop(id)(first.to_data(), second.to_data());
  }

  friend inline bool operator <(const numeric& first, const numeric& second) {
    return relop(reational_op_id::LT, first, second);
  }

  friend inline bool operator <=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::LE, first, second);
  }

  friend inline bool operator >(const numeric& first, const numeric& second) {
    return relop(reational_op_id::GT, first, second);
  }

  friend inline bool operator >=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::GE, first, second);
  }

  friend inline bool operator ==(const numeric& first, const numeric& second) {
    return relop(reational_op_id::EQ, first, second);
  }

  friend inline bool operator !=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::NEQ, first, second);
  }

  // Arithmetic operations
  static inline numeric unaryop(unary_op_id id, const numeric& n) {
    numeric result(n.type());
    result.type_.unaryop(id)(result.data_, n.to_data());
    return result;
  }

  friend inline numeric operator-(const numeric& n) {
    return unaryop(unary_op_id::NEGATIVE, n);
  }

  friend inline numeric operator+(const numeric& n) {
    return unaryop(unary_op_id::POSITIVE, n);
  }

  friend inline numeric operator~(const numeric& n) {
    return unaryop(unary_op_id::BW_NOT, n);
  }

  static numeric binaryop(binary_op_id id, const numeric& first,
                          const numeric& second) {
    if (first.type() != second.type())
      THROW(
          invalid_operation_exception,
          "Cannot operate on values of different types: (" + first.type().name()
              + ", " + second.type().name() + ")");
    numeric result(first.type());
    result.type_.binaryop(id)(result.data_, first.to_data(), second.to_data());
    return result;
  }

  friend inline numeric operator+(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::ADD, first, second);
  }

  friend inline numeric operator-(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::SUBTRACT, first, second);
  }

  friend inline numeric operator*(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::MULTIPLY, first, second);
  }

  friend inline numeric operator/(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::DIVIDE, first, second);
  }

  friend inline numeric operator%(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::MODULO, first, second);
  }

  friend inline numeric operator&(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_AND, first, second);
  }

  friend inline numeric operator|(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_OR, first, second);
  }

  friend inline numeric operator^(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_XOR, first, second);
  }

  friend inline numeric operator<<(const numeric& first,
                                   const numeric& second) {
    return binaryop(binary_op_id::BW_LSHIFT, first, second);
  }

  friend inline numeric operator>>(const numeric& first,
                                   const numeric& second) {
    return binaryop(binary_op_id::BW_RSHIFT, first, second);
  }

  numeric& operator=(const immutable_value& other) {
    type_ = other.type();
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    type_.unaryop(unary_op_id::ASSIGN)(data_, other.to_data());
    return *this;
  }

  numeric& operator=(bool value) {
    type_ = BOOL_TYPE;
    as<bool>() = value;
    return *this;
  }

  numeric& operator=(int8_t value) {
    type_ = CHAR_TYPE;
    as<int8_t>() = value;
    return *this;
  }

  numeric& operator=(int16_t value) {
    type_ = SHORT_TYPE;
    as<int16_t>() = value;
    return *this;
  }

  numeric& operator=(int32_t value) {
    type_ = INT_TYPE;
    as<int32_t>() = value;
    return *this;
  }

  numeric& operator=(int64_t value) {
    type_ = LONG_TYPE;
    as<int64_t>() = value;
    return *this;
  }

  numeric& operator=(float value) {
    type_ = FLOAT_TYPE;
    as<float>() = value;
    return *this;
  }

  numeric& operator=(double value) {
    type_ = DOUBLE_TYPE;
    as<double>() = value;
    return *this;
  }

  template<typename T,
      typename std::enable_if<
          std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* =
          nullptr>
  T& as() {
    return *reinterpret_cast<T*>(data_);
  }

  template<typename T,
      typename std::enable_if<
          std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* =
          nullptr>
  const T& as() const {
    return *reinterpret_cast<const T*>(data_);
  }

  std::string to_string() const {
    return type_.name() + "(" + type_.to_string_op()(to_data()) + ")";
  }

 private:
  data_type type_;
  uint8_t data_[MAX_SIZE];
};

}

#endif /* DIALOG_NUMERIC_H_ */
