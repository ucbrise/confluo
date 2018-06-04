#ifndef CONFLUO_TYPES_NUMERIC_H_
#define CONFLUO_TYPES_NUMERIC_H_

#include "data_type.h"
#include "immutable_value.h"
#include "string_utils.h"

namespace confluo {

class numeric;

/** Helpers for casting numerics **/

/**
 * Cast the numeric value to data type
 *
 * @param val The value
 * @param type The data type
 * @return The cast value
 */
numeric cast(const numeric &val, const data_type &type);

/**
 * Gets the greater data type 
 *
 * @param t1 The first data type
 * @param t2 The second data type
 *
 * @return The greater data type based on id
 */
data_type max(const data_type &t1, const data_type &t2);

class numeric {
 public:
  /**
   * Constructs a numeric of the none type
   */
  numeric();

  /**
   * Constructs a numeric that contains the given data type and the zero
   * value of that data type
   *
   * @param type The data type of the numeric
   */
  numeric(data_type type);

  /**
   * Constructs a numeric from a boolean value
   *
   * @param val The boolean value used to create this numeric
   */
  numeric(bool val);

  /**
   * Constructs a numeric from a character value
   *
   * @param val The character value used to create this numeric
   */
  numeric(int8_t val);

  /**
   * Constructs a numeric from an unsigned character value
   *
   * @param val The unsigned character value to create this numeric
   */
  numeric(uint8_t val);

  /**
   * Constructs a numeric from the signed short value
   *
   * @param val The signed short value to create this numeric from
   */
  numeric(int16_t val);

  /**
   * Constructs a numeric from the unsigned short value
   *
   * @param val The unsigned short value used to create this numeric
   */
  numeric(uint16_t val);

  /**
   * Constructs a numeric from the integer value
   *
   * @param val The integer value used to construct this numeric
   */
  numeric(int32_t val);

  /**
   * Constructs a numeric from an unsigned integer value
   *
   * @param val The unsigned integer value used to construct the numeric
   */
  numeric(uint32_t val);

  /**
   * Constructs a numeric from a signed long value
   *
   * @param val The signed long value used to construct this numeric
   */
  numeric(int64_t val);

  /**
   * Constructs a numeric from an unsigned long value
   *
   * @param val The unsigned long value used to construct this numeric
   */
  numeric(uint64_t val);

  /**
   * Constructs a numeric from a float value
   *
   * @param val The float value used to construct this numeric
   */
  numeric(float val);

  /**
   * Constructs a numeric from a double precision floating point value
   *
   * @param val The double value used to create this numeric
   */
  numeric(double val);

  /**
   * Constructs a numeric from the given data type and data
   *
   * @param type The type of data the numeric contains
   * @param data The data that the numeric contains
   * @throw invalid_cast_exception
   */
  numeric(const data_type &type, void *data);

  /**
   * Constructs a numeric from the given immutable value
   *
   * @param val The immutable value, which contents are used to intialize
   * this numeric
   * @throw invalid_cast_exception
   */
  numeric(const immutable_value &val);

  /**
   * Gets whether this numeric is valid
   *
   * @return True if this numeric is valid, false otherwise
   */
  bool is_valid() const;

  /**
   * Parses the numeric from a string
   *
   * @param str The string to parse the numeric from
   * @param type The data type of the numeric
   *
   * @return A numeric generated from the contents of the string
   */
  static numeric parse(const std::string &str, const data_type &type);

  /**
   * Converts this numeric to raw immutable data
   *
   * @return Immutable raw data containing the data and type of this numeric
   */
  immutable_raw_data to_data() const;

  /**
   * Gets the data type of this numeric
   *
   * @return The data type of this numeric
   */
  data_type const &type() const;

  // Relational operators
  /**
   * Performs a relational comparison between two numerics
   *
   * @param id The unique identifier for the relational operator
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   * @throw invalid_operation_exception
   * @return True if the relational comparison is true, false otherwise
   */
  static bool relop(reational_op_id id, const numeric &first, const numeric &second);

  /**
   * Less than operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is less than the data in
   * the second numeric, false otherwise
   */
  friend bool operator<(const numeric &first, const numeric &second);

  /**
   * Less than or equal to operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is less than or equal to
   * the data in the second numeric
   */
  friend bool operator<=(const numeric &first, const numeric &second);

  /**
   * Greater than operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is greater than the 
   * data in the second numeric
   */
  friend bool operator>(const numeric &first, const numeric &second);

  /**
   * Greater than or equal to operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is greater than or equal
   * to the data in the second numeric
   */
  friend bool operator>=(const numeric &first, const numeric &second);

  /**
   * Equality operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is equal
   * to the data in the second numeric
   */
  friend bool operator==(const numeric &first, const numeric &second);

  /**
   * Not equal operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is not equal
   * to the data in the second numeric
   */
  friend bool operator!=(const numeric &first, const numeric &second);

  // Arithmetic operations
  /**
   * Performs a unary operation on the given numeric
   *
   * @param id The unique identifier for the unary operator
   * @param n The numeric the unary operator is applied to
   *
   * @return The numeric containing the result of the unary operation
   */
  static numeric unaryop(unary_op_id id, const numeric &n);

  /**
   * Unary negation operator
   *
   * @param n The numeric to be negated
   *
   * @return A numeric containing the negated numeric
   */
  friend numeric operator-(const numeric &n);

  /**
   * Unary positive operator
   *
   * @param n The numeric the positive unary operator is applied to
   *
   * @return A numeric containing the value of the given numeric
   */
  friend numeric operator+(const numeric &n);

  /**
   * Bitwise not unary operator
   *
   * @param n The numeric the bitwise not unary operator is applied to
   *
   * @return A numeric containing the flipped bits of the given numeric
   * value
   */
  friend numeric operator~(const numeric &n);

  /**
   * Performs the binary operation on the expression containing two
   * numerics
   *
   * @param id The unique identifier of the binary operator
   * @param first The first numeric in the expression
   * @param second The second numeric in the expression
   *
   * @return A numeric containing the result of the binary expression
   */
  static numeric binaryop(binary_op_id id, const numeric &first, const numeric &second);

  /**
   * The addition operator
   *
   * @param first The first numeric in the expression
   * @param second The second numeric in the expression
   *
   * @return A numeric containing the sum of the values of the numerics
   */
  friend numeric operator+(const numeric &first, const numeric &second);

  /**
   * The subtraction operator
   *
   * @param first The first numeric in the expression
   * @param second The second numeric in the expression
   *
   * @return A numeric containing the difference of the values 
   * of the numerics
   */
  friend numeric operator-(const numeric &first, const numeric &second);

  /**
   * The multiplication operator
   *
   * @param first The first numeric in the expression
   * @param second The second numeric in the expression
   *
   * @return A numeric containing the product of the values 
   * of the numerics
   */
  friend numeric operator*(const numeric &first, const numeric &second);

  /**
   * The division operator
   *
   * @param first The numeric containing the dividend
   * @param second The numeric containing the divisor
   *
   * @return A numeric containing the quotient of the expression
   */
  friend numeric operator/(const numeric &first, const numeric &second);

  /**
   * The modulo operator
   *
   * @param first The numeric containing the dividend
   * @param second The numeric containing the divisor
   *
   * @return A numeric containing the remainder of the expression
   */
  friend numeric operator%(const numeric &first, const numeric &second);

  /**
   * The bitwise and operator
   *
   * @param first The first numeric in the and expression
   * @param second The second numeric in the and expression
   *
   * @return A numeric containing the result of the and expression
   */
  friend numeric operator&(const numeric &first, const numeric &second);

  /**
   * The bitwise or operator
   *
   * @param first The first numeric in the or expression
   * @param second The second numeric in the or expression
   *
   * @return A numeric containing the result of the or expression
   */
  friend numeric operator|(const numeric &first, const numeric &second);

  /**
   * The bitwise xor operator
   *
   * @param first The first numeric in the xor expression
   * @param second The second numeric in the xor expression
   *
   * @return A numeric containing the result of the xor expression
   */
  friend numeric operator^(const numeric &first, const numeric &second);

  /**
   * The bitwise left logical shift operator
   *
   * @param first The numeric containing the value that will be shifted
   * @param second The numeric containing the amount to shift
   *
   * @return A numeric containing the result of the left shift expression
   */
  friend numeric operator<<(const numeric &first, const numeric &second);

  /**
   * The bitwise right logical shift operator
   *
   * @param first The numeric containing the value that will be shifted
   * @param second The numeric containing the amount to shift
   *
   * @return A numeric containing the result of the right shift expression
   */
  friend numeric operator>>(const numeric &first, const numeric &second);

  /**
   * Constructs this numeric from the given immutable value
   *
   * @param other The immutable value containing the data type and value
   * that this numeric will contain
   *
   * @return This updated numeric
   */
  numeric &operator=(const immutable_value &other);

  /**
   * Assigns the boolean value to this numeric
   *
   * @param value The boolean value that is copied to this numeric's value
   *
   * @return This updated numeric
   */
  numeric &operator=(bool value);

  /**
   * Assigns the character value to this numeric
   *
   * @param value The character value that is copied to this numeric's value
   *
   * @return This updated numeric
   */
  numeric &operator=(int8_t value);

  /**
   * Assigns the unsigned character value to this numeric
   *
   * @param value The unsigned character value that is copied to this
   * numeric's value
   *
   * @return The updated numeric
   */
  numeric &operator=(uint8_t value);

  /**
   * Assigns the short value to this numeric
   *
   * @param value The short value that is copied to this numeric's value
   *
   * @return The updated numeric
   */
  numeric &operator=(int16_t value);

  /**
   * Assigns the unsigned short value to this numeric
   *
   * @param value The unsigned short value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric &operator=(uint16_t value);

  /**
   * Assigns the integer value to this numeric
   *
   * @param value The integer value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric &operator=(int32_t value);

  /**
   * Assigns the unsigned integer value to this numeric
   *
   * @param value The unsigned integer value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric &operator=(uint32_t value);

  /**
   * Assigns the long value to this numeric
   *
   * @param value The long value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric &operator=(int64_t value);

  /**
   * Assigns the unsigned long value to this numeric
   *
   * @param value The value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric &operator=(uint64_t value);

  /**
   * Assigns the float value to this numeric
   *
   * @param value The float value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric &operator=(float value);

  /**
   * Assigns the double value to this numeric
   *
   * @param value The double value that is copied to this numeric's value
   *
   * @return This updated numeric
   */
  numeric &operator=(double value);

  /**
   * Casts the numeric's data to type T
   *
   * @tparam T The desired data type
   *
   * @return The value of type T
   */
  template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value,
                                               T>::type * = nullptr>
  T &as() {
    return *reinterpret_cast<T *>(data_);
  }

  /**
   * Casts the numeric's data to type T
   *
   * @tparam T The desired data type
   *
   * @return The value of type T that is not modifiable
   */
  template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value,
                                               T>::type * = nullptr>
  const T &as() const {
    return *reinterpret_cast<const T *>(data_);
  }

  /**
   * Gets a string representation of the numeric
   *
   * @return A formatted string containing the type and data of the numeric
   */
  std::string to_string() const;

  /**
   * Get the type for the numeric
   *
   * @return The numeric type
   */
  data_type type();

  /**
   * Get raw data for the numeric
   *
   * @return Raw data for the numeric
   */
  uint8_t *data();

 private:
  data_type type_;
  uint8_t data_[sizeof(uint64_t)];
};

// Cast functions for primitive types
using cast_fn = numeric (*)(const numeric &v);

namespace detail {

/**
 * Casts one numeric to the another type
 *
 * @tparam IN The type of the input numeric
 * @tparam OUT The desired type for the output
 */
template<typename IN, typename OUT>
struct cast_helper {
  /**
   * Casts one numeric to another type
   *
   * @param v The numeric to cast
   *
   * @return The casted numeric
   */
  static numeric cast(const numeric &v) {
    return numeric(static_cast<OUT>(v.as<IN>()));
  }
};

/**
 * Casts one numeric to the same type
 *
 * @tparam T The type to cast the numeric to
 */
template<typename T>
struct cast_helper<T, T> {
  /**
   * Casts one numeric to another type
   *
   * @param v The numeric to cast
   *
   * @return The casted numeric
   */
  static numeric cast(const numeric &v) {
    return v;
  }
};

/**
 * Casts a numeric of the void type to the OUT type
 *
 * @tparam OUT The desired output type
 */
template<typename OUT>
struct cast_helper<void, OUT> {
  /**
   * Casts a numeric to another type
   *
   * @param v The numeric to cast
   * @throw invalid_cast_exception Cannot cast from void type
   * @return The casted numeric
   */
  static numeric cast(const numeric &v) {
    throw invalid_cast_exception("Cannot cast none type to any other type");
  }
};

/**
 * Casts a numeric from the in type to void
 *
 * @tparam IN The type of the input numeric
 */
template<typename IN>
struct cast_helper<IN, void> {
  /**
   * Casts a numeric to another type
   *
   * @param v The numeric to cast
   * @throw invalid_cast_exception Cannot cast to void type
   * @return The casted numeric
   */
  static numeric cast(const numeric &v) {
    throw invalid_cast_exception("Cannot cast any type to none type");
  }
};

/**
 * Casts a numeric from the void type to the void type
 */
template<>
struct cast_helper<void, void> {
  /**
   * Casts a numeric to another type
   *
   * @param v The numeric to cast
   * @throw invalid_cast_exception Cannot cast to void type
   * @return The casted numeric
   */
  static numeric cast(const numeric &v) {
    throw invalid_cast_exception("Cannot cast none type to none type");
  }
};

}

/**
 * Casts a numeric from one type to another
 *
 * @tparam IN The type of the input numeric
 * @tparam OUT The desired output type
 * @param v The input numeric
 *
 * @return The casted numeric
 */
template<typename IN, typename OUT>
numeric type_cast(const numeric &v) {
  return detail::cast_helper<IN, OUT>::cast(v);
}

/**
 * Initializes the type cast operators for all the types
 *
 * @tparam IN The type of the input numeric
 *
 * @return A vector containing all of the casting operators
 */
template<typename IN>
std::vector<cast_fn> init_type_cast_ops() {
  return {type_cast<IN, void>, type_cast<IN, bool>, type_cast<IN, int8_t>, type_cast<IN, uint8_t>,
          type_cast<IN, int16_t>, type_cast<IN, uint16_t>, type_cast<IN, int32_t>,
          type_cast<IN, uint32_t>, type_cast<IN, int64_t>, type_cast<IN, uint64_t>,
          type_cast<IN, float>, type_cast<IN, double>};
}

class cast_ops {
 public:
  static cast_ops &instance() {
    static cast_ops ops;
    return ops;
  }

  std::vector<cast_fn> const &at(size_t i) const {
    return cast_ops_.at(i);
  }

  std::vector<cast_fn> &operator[](size_t i) {
    return cast_ops_[i];
  }

  size_t size() const {
    return cast_ops_.size();
  }

 private:
  cast_ops() : cast_ops_{init_type_cast_ops<void>(), init_type_cast_ops<bool>(), init_type_cast_ops<int8_t>(),
                         init_type_cast_ops<uint8_t>(), init_type_cast_ops<int16_t>(), init_type_cast_ops<uint16_t>(),
                         init_type_cast_ops<int32_t>(), init_type_cast_ops<uint32_t>(), init_type_cast_ops<int64_t>(),
                         init_type_cast_ops<uint64_t>(), init_type_cast_ops<float>(), init_type_cast_ops<double>()} {}

  std::vector<std::vector<cast_fn>> cast_ops_;
};

}

#endif /* CONFLUO_TYPES_NUMERIC_H_ */
