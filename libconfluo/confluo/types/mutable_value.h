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
  mutable_value(data_type type = NONE_TYPE);

  /**
   * Constructs a mutable value from a specified data type and raw data
   *
   * @param type The type of data
   * @param value The data itself for the mutable value
   */
  mutable_value(const data_type& type, immutable_raw_data value);

  /**
   * Constructs a mutable 
   *
   * @param type The type
   * @param value The value
   */
  mutable_value(const data_type& type, const void* value);

  /**
   * Constructs a boolean mutable value
   *
   * @param value The boolan mutable value to create
   */
  mutable_value(bool value);

  /**
   * Constructs a character mutable value
   *
   * @param value The character value to make a mutable value of
   */
  mutable_value(int8_t value);

  /**
   * Constructs a short mutable value
   *
   * @param value The short value to make a mutable value of
   */
  mutable_value(int16_t value);

  /**
   * Constructs a mutable value from a integer value
   *
   * @param value The integer value to make a mutable value from
   */
  mutable_value(int32_t value);

  /**
   * Constructs a mutable value from a given long value
   *
   * @param value The long value used to construct the mutable value
   */
  mutable_value(int64_t value);

  /**
   * Constructs a mutable value from a single precision floating point
   * number
   *
   * @param value The floating point value to construct a mutable value
   * from
   */
  mutable_value(float value);

  /**
   * Constructs a mutable value from a double precision floating point
   * number
   *
   * @param value The double value to construct a mutable value
   * from
   */
  mutable_value(double value);

  /**
   * Constructs a mutable value from a string
   *
   * @param str The string to construct a mutable value from
   */
  mutable_value(const std::string& str);

  /**
   * Constructs a mutable value from an immutable value
   *
   * @param other The immutable value whose contents are used to construct
   * this mutable value
   */
  mutable_value(const immutable_value& other);

  /**
   * Constructs a mutable value from the given mutable value
   *
   * @param other The mutable value whose contents are copied into this
   * mutable value
   */
  mutable_value(const mutable_value& other);

  /**
   * Moves the contents of the rvalue mutable value to this mutable value
   *
   * @param other The rvalue mutable value that is moved into this mutable
   * value
   */
  mutable_value(mutable_value&& other);

  /**
   * Deallocates the data in this mutable value
   */
  ~mutable_value();

  /**
   * Parses the given string to construct a mutable value of the given type
   *
   * @param str The string to parse the mutable value from
   * @param type The data type of the mutable value
   *
   * @return A mutable value containing the contents of the string
   */
  static mutable_value parse(const std::string& str, const data_type& type);

  /**
   * Copies the data from this mutable value to an immutable value
   *
   * @return An immutable value with the contents of this mutable value
   */
  immutable_value copy() const;

  // Arithmetic operations
  /**
   * Performs a unary operation on the given immutable value
   *
   * @param id The unique identifier for the unary operator
   * @param n The immutable value that the unary operator is applied to
   *
   * @return A mutable value containing the result of the unary operation
   */
  static mutable_value unaryop(unary_op_id id, const immutable_value& n);

  /**
   * Unary negation operator 
   *
   * @param n The immutable value to negate
   *
   * @return A mutable value that contains the negated value
   */
  friend mutable_value operator-(const immutable_value& n);

  /**
   * Unary positive operator 
   *
   * @param n The immutable value to get the value
   *
   * @return A mutable value that contains the value of the immutable value
   */
  friend mutable_value operator+(const immutable_value& n);

  /**
   * Bitwise not unary operator
   *
   * @param n The immutable value to perform the bitwise not operator on
   *
   * @return A mutable value containing the resultant value with all of
   * the bits flipped
   */
  friend mutable_value operator~(const immutable_value& n);

  /**
   * Performs a binary operation on two immutable values
   *
   * @param id The unique identifier of the binary operator
   * @param first The first immutable value in the expression
   * @param second The second immutable value in the expression
   *
   * @return A mutable value containing the result of the binary operation
   */
  static mutable_value binaryop(binary_op_id id, const immutable_value& first, const immutable_value& second);

  /**
   * The addition operator
   *
   * @param first The first immutable value in the addition expression
   * @param second The second immutable value in the addition expression
   *
   * @return A mutable value containing the sum of the two immutable values
   */
  friend mutable_value operator+(const immutable_value& first, const immutable_value& second);

  /**
   * The subtraction operator
   *
   * @param first The first immutable value in the subtraction expression
   * @param second The second immutable value in the subtraction expression
   *
   * @return A mutable value containing the difference of the two 
   * immutable values
   */
  friend mutable_value operator-(const immutable_value& first, const immutable_value& second);

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
  friend mutable_value operator*(const immutable_value& first, const immutable_value& second);

  /**
   * The division operator
   *
   * @param first The first immutable value in the division expression
   * @param second The second immutable value in the division
   * expression
   *
   * @return A mutable value containing the quotient of the expression
   */
  friend mutable_value operator/(const immutable_value& first, const immutable_value& second);

  /**
   * The modulo operator
   *
   * @param first The dividend immutable value
   * @param second The divisor immutable value
   *
   * @return A mutable value containing the remainder 
   */
  friend mutable_value operator%(const immutable_value& first, const immutable_value& second);

  /**
   * The bitwise and operator
   *
   * @param first The first immutable value in the and expression
   * @param second The second immutable value in the and expression
   *
   * @return A mutable value containing the result of the bitwise and
   * operator applied to the two operands
   */
  friend mutable_value operator&(const immutable_value& first, const immutable_value& second);

  /**
   * The bitwise or operator
   *
   * @param first The first immutable value in the or expression
   * @param second The second immutable value in the or expression
   *
   * @return A mutable value containing the result of the bitwise or
   * operator applied to the two operands
   */
  friend mutable_value operator|(const immutable_value& first, const immutable_value& second);

  /**
   * The bitwise xor operator
   *
   * @param first The first immutable value in the xor expression
   * @param second The second immutable value in the xor expression
   *
   * @return A mutable value containing the result of the bitwise xor
   * operator applied to the two operands
   */
  friend mutable_value operator^(const immutable_value& first, const immutable_value& second);

  /**
   * The bitwise left shift operator
   *
   * @param first The immutable value containing the data to shift
   * @param second The immutable value containing the amount to shift by
   *
   * @return A mutable value containing the result of the bitwise left
   * shift operator applied to the two operands
   */
  friend mutable_value operator<<(const immutable_value& first, const immutable_value& second);

  /**
   * The bitwise right shift operator
   *
   * @param first The immutable value containing the data to shift
   * @param second The immutable value containing the amount to shift by
   *
   * @return A mutable value containing the result of the bitwise right
   * shift operator applied to the two operands
   */
  friend mutable_value operator>>(const immutable_value& first, const immutable_value& second);

  // TODO: Add more assignment operators
  /**
   * Assigns the immutable value to this mutable value
   *
   * @param other The immutable value whose contents are copied into this
   * mutable value
   *
   * @return This updated mutable value
   */
  mutable_value& operator=(const immutable_value& other);

  /**
   * Assigns another mutable value to this mutable value
   *
   * @param other The mutable value whose contents are copied into this
   * mutable value
   *
   * @return This updated mutable value
   */
  mutable_value& operator=(const mutable_value& other);

  /**
   * Moves the rvalue mutable value into this mutable value
   *
   * @param other The other mutable value that is moved into this mutable
   * value
   *
   * @return This updated mutable value
   */
  mutable_value& operator=(mutable_value&& other);
}
;

}

#endif /* CONFLUO_TYPES_MUTABLE_VALUE_H_ */
