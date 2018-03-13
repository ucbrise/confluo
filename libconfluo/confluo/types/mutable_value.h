#ifndef CONFLUO_TYPES_MUTABLE_VALUE_H_
#define CONFLUO_TYPES_MUTABLE_VALUE_H_

#include <cstdint>

#include "data_type.h"
#include "exceptions.h"
#include "immutable_value.h"

namespace confluo {

/**
 * The mutable value class. Contains modifiable data and arithmetic 
 * operations the can be performed on the data.
 */
class mutable_value : public immutable_value {
 public:
  /**
   * Constructs a mutable value of the specified type
   *
   * @param type The data type of the specified value
   */
  mutable_value(data_type type = NONE_TYPE)
      : immutable_value(type,
                        type.is_none() ? nullptr : new uint8_t[type.size]()) {
  }

  /**
   * Constructs a mutable value from a specified data type and raw data
   *
   * @param type The type of data
   * @param value The data itself for the mutable value
   */
  mutable_value(const data_type& type, immutable_raw_data value)
      : immutable_value(type, const_cast<void*>(value.ptr)) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, value);
  }

  /**
   * Constructs a mutable 
   *
   * @param type The type
   * @param value The value
   */
  mutable_value(const data_type& type, const void* value)
      : immutable_value(type,
                        value == nullptr ? nullptr : new uint8_t[type.size]()) {
    if (value != nullptr) {
      type_.unaryop(unary_op_id::ASSIGN)(ptr_,
                                         immutable_raw_data(value, type.size));
    }
  }

  /**
   * Constructs a boolean mutable value
   *
   * @param value The boolan mutable value to create
   */
  mutable_value(bool value)
      : immutable_value(BOOL_TYPE, new uint8_t[BOOL_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, BOOL_TYPE.size));
  }

  /**
   * Constructs a character mutable value
   *
   * @param value The character value to make a mutable value of
   */
  mutable_value(int8_t value)
      : immutable_value(CHAR_TYPE, new uint8_t[CHAR_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, CHAR_TYPE.size));
  }

  /**
   * Constructs a short mutable value
   *
   * @param value The short value to make a mutable value of
   */
  mutable_value(int16_t value)
      : immutable_value(SHORT_TYPE, new uint8_t[SHORT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, SHORT_TYPE.size));
  }

  /**
   * Constructs a mutable value from a integer value
   *
   * @param value The integer value to make a mutable value from
   */
  mutable_value(int32_t value)
      : immutable_value(INT_TYPE, new uint8_t[INT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, INT_TYPE.size));
  }

  /**
   * Constructs a mutable value from a given long value
   *
   * @param value The long value used to construct the mutable value
   */
  mutable_value(int64_t value)
      : immutable_value(LONG_TYPE, new uint8_t[LONG_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, LONG_TYPE.size));
  }

  /**
   * Constructs a mutable value from a single precision floating point
   * number
   *
   * @param value The floating point value to construct a mutable value
   * from
   */
  mutable_value(float value)
      : immutable_value(FLOAT_TYPE, new uint8_t[FLOAT_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, FLOAT_TYPE.size));
  }

  /**
   * Constructs a mutable value from a double precision floating point
   * number
   *
   * @param value The double value to construct a mutable value
   * from
   */
  mutable_value(double value)
      : immutable_value(DOUBLE_TYPE, new uint8_t[DOUBLE_TYPE.size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(&value, DOUBLE_TYPE.size));
  }

  /**
   * Constructs a mutable value from a string
   *
   * @param str The string to construct a mutable value from
   */
  mutable_value(const std::string& str)
      : immutable_value(STRING_TYPE(str.length()), new char[str.length()]()) {
    type_.unaryop(unary_op_id::ASSIGN)(
        ptr_, immutable_raw_data(str.c_str(), str.length()));
  }

  /**
   * Constructs a mutable value from an immutable value
   *
   * @param other The immutable value whose contents are used to construct
   * this mutable value
   */
  mutable_value(const immutable_value& other)
      : immutable_value(other.type(), new uint8_t[other.type().size]()) {
    type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
  }

  /**
   * Constructs a mutable value from the given mutable value
   *
   * @param other The mutable value whose contents are copied into this
   * mutable value
   */
  mutable_value(const mutable_value& other)
      : immutable_value(
          other.type_,
          other.ptr_ == nullptr ? nullptr : new uint8_t[other.type_.size]()) {
    if (other.ptr_ != nullptr) {
      type_.unaryop(unary_op_id::ASSIGN)(ptr_, other.to_data());
    }
  }

  /**
   * Moves the contents of the rvalue mutable value to this mutable value
   *
   * @param other The rvalue mutable value that is moved into this mutable
   * value
   */
  mutable_value(mutable_value&& other)
      : immutable_value(other.type_, other.ptr_) {
    other.type_ = NONE_TYPE;
    other.ptr_ = nullptr;
  }

  /**
   * Deallocates the data in this mutable value
   */
  ~mutable_value() {
    if (ptr_ != nullptr && !type_.is_none())
      delete[] reinterpret_cast<uint8_t*>(ptr_);
  }

  /**
   * Parses the given string to construct a mutable value of the given type
   *
   * @param str The string to parse the mutable value from
   * @param type The data type of the mutable value
   *
   * @return A mutable value containing the contents of the string
   */
  static mutable_value parse(const std::string& str, const data_type& type) {
    mutable_value value(type);
    type.parse_op()(str, value.ptr_);
    return value;
  }

  /**
   * Copies the data from this mutable value to an immutable value
   *
   * @return An immutable value with the contents of this mutable value
   */
  immutable_value copy() const {
    return immutable_value(type_, ptr_);
  }

  // Arithmetic operations
  /**
   * Performs a unary operation on the given immutable value
   *
   * @param id The unique identifier for the unary operator
   * @param n The immutable value that the unary operator is applied to
   *
   * @return A mutable value containing the result of the unary operation
   */
  static inline mutable_value unaryop(unary_op_id id,
                                      const immutable_value& n) {
    mutable_value result(n.type());
    result.type_.unaryop(id)(result.ptr_, n.to_data());
    return result;
  }

  /**
   * Unary negation operator 
   *
   * @param n The immutable value to negate
   *
   * @return A mutable value that contains the negated value
   */
  friend inline mutable_value operator-(const immutable_value& n) {
    return unaryop(unary_op_id::NEGATIVE, n);
  }

  /**
   * Unary positive operator 
   *
   * @param n The immutable value to get the value
   *
   * @return A mutable value that contains the value of the immutable value
   */
  friend inline mutable_value operator+(const immutable_value& n) {
    return unaryop(unary_op_id::POSITIVE, n);
  }

  /**
   * Bitwise not unary operator
   *
   * @param n The immutable value to perform the bitwise not operator on
   *
   * @return A mutable value containing the resultant value with all of
   * the bits flipped
   */
  friend inline mutable_value operator~(const immutable_value& n) {
    return unaryop(unary_op_id::BW_NOT, n);
  }

  /**
   * Performs a binary operation on two immutable values
   *
   * @param id The unique identifier of the binary operator
   * @param first The first immutable value in the expression
   * @param second The second immutable value in the expression
   *
   * @return A mutable value containing the result of the binary operation
   */
  static mutable_value binaryop(binary_op_id id, const immutable_value& first,
                                const immutable_value& second) {
    if (first.type() != second.type())
      THROW(invalid_operation_exception,
            "Cannot operate on values of different types");
    mutable_value result(first.type());
    result.type_.binaryop(id)(result.ptr_, first.to_data(), second.to_data());
    return result;
  }

  /**
   * The addition operator
   *
   * @param first The first immutable value in the addition expression
   * @param second The second immutable value in the addition expression
   *
   * @return A mutable value containing the sum of the two immutable values
   */
  friend inline mutable_value operator+(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::ADD, first, second);
  }

  /**
   * The subtraction operator
   *
   * @param first The first immutable value in the subtraction expression
   * @param second The second immutable value in the subtraction expression
   *
   * @return A mutable value containing the difference of the two 
   * immutable values
   */
  friend inline mutable_value operator-(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::SUBTRACT, first, second);
  }

  /**
   * The multiplication operator
   *
   * @param first The first immutable value in the multiplication expression
   * @param second The second immutable value in the multiplication
   * expression
   *
   * @return A mutable value containing the product of the two 
   * immutable values
   */
  friend inline mutable_value operator*(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::MULTIPLY, first, second);
  }

  /**
   * The division operator
   *
   * @param first The first immutable value in the division expression
   * @param second The second immutable value in the division
   * expression
   *
   * @return A mutable value containing the quotient of the expression
   */
  friend inline mutable_value operator/(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::DIVIDE, first, second);
  }

  /**
   * The modulo operator
   *
   * @param first The dividend immutable value
   * @param second The divisor immutable value
   *
   * @return A mutable value containing the remainder 
   */
  friend inline mutable_value operator%(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::MODULO, first, second);
  }

  /**
   * The bitwise and operator
   *
   * @param first The first immutable value in the and expression
   * @param second The second immutable value in the and expression
   *
   * @return A mutable value containing the result of the bitwise and
   * operator applied to the two operands
   */
  friend inline mutable_value operator&(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::BW_AND, first, second);
  }

  /**
   * The bitwise or operator
   *
   * @param first The first immutable value in the or expression
   * @param second The second immutable value in the or expression
   *
   * @return A mutable value containing the result of the bitwise or
   * operator applied to the two operands
   */
  friend inline mutable_value operator|(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::BW_OR, first, second);
  }

  /**
   * The bitwise xor operator
   *
   * @param first The first immutable value in the xor expression
   * @param second The second immutable value in the xor expression
   *
   * @return A mutable value containing the result of the bitwise xor
   * operator applied to the two operands
   */
  friend inline mutable_value operator^(const immutable_value& first,
                                        const immutable_value& second) {
    return binaryop(binary_op_id::BW_XOR, first, second);
  }

  /**
   * The bitwise left shift operator
   *
   * @param first The immutable value containing the data to shift
   * @param second The immutable value containing the amount to shift by
   *
   * @return A mutable value containing the result of the bitwise left
   * shift operator applied to the two operands
   */
  friend inline mutable_value operator<<(const immutable_value& first,
                                         const immutable_value& second) {
    return binaryop(binary_op_id::BW_LSHIFT, first, second);
  }

  /**
   * The bitwise right shift operator
   *
   * @param first The immutable value containing the data to shift
   * @param second The immutable value containing the amount to shift by
   *
   * @return A mutable value containing the result of the bitwise right
   * shift operator applied to the two operands
   */
  friend inline mutable_value operator>>(const immutable_value& first,
                                         const immutable_value& second) {
    return binaryop(binary_op_id::BW_RSHIFT, first, second);
  }

  // TODO: Add more assignment operators
  /**
   * Assigns the immutable value to this mutable value
   *
   * @param other The immutable value whose contents are copied into this
   * mutable value
   *
   * @return This updated mutable value
   */
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

  /**
   * Assigns another mutable value to this mutable value
   *
   * @param other The mutable value whose contents are copied into this
   * mutable value
   *
   * @return This updated mutable value
   */
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

  /**
   * Moves the rvalue mutable value into this mutable value
   *
   * @param other The other mutable value that is moved into this mutable
   * value
   *
   * @return This updated mutable value
   */
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

#endif /* CONFLUO_TYPES_MUTABLE_VALUE_H_ */
