#ifndef CONFLUO_TYPES_NUMERIC_H_
#define CONFLUO_TYPES_NUMERIC_H_

#include "data_type.h"
#include "immutable_value.h"
#include "string_utils.h"

namespace confluo {

class numeric;

/** Helpers for casting numerics **/
static numeric cast(const numeric& val, const data_type& type);

/**
 * Gets the greater data type 
 *
 * @param t1 The first data type
 * @param t2 The second data type
 *
 * @return The greater data type based on id
 */
static data_type max(const data_type& t1, const data_type& t2) {
  return type_manager::get_type(std::max(t1.id, t2.id));
}

class numeric {
 public:
  /** The maximum size of data a numeric can hold in bytes */
  static const size_t MAX_SIZE = sizeof(uint64_t);

  /**
   * Constructs a numeric of the none type
   */
  numeric()
      : type_(NONE_TYPE) {
  }

  /**
   * Constructs a numeric that contains the given data type and the zero
   * value of that data type
   *
   * @param type The data type of the numeric
   */
  numeric(data_type type)
      : type_(type) {
    memcpy(data_, type_.zero(), type_.size);
  }

  /**
   * Constructs a numeric from a boolean value
   *
   * @param val The boolean value used to create this numeric
   */
  numeric(bool val)
      : type_(BOOL_TYPE) {
    as<bool>() = val;
  }

  /**
   * Constructs a numeric from a character value
   *
   * @param val The character value used to create this numeric
   */
  numeric(int8_t val)
      : type_(CHAR_TYPE) {
    as<int8_t>() = val;
  }

  /**
   * Constructs a numeric from an unsigned character value
   *
   * @param val The unsigned character value to create this numeric
   */
  numeric(uint8_t val)
      : type_(UCHAR_TYPE) {
    as<uint8_t>() = val;
  }

  /**
   * Constructs a numeric from the signed short value
   *
   * @param val The signed short value to create this numeric from
   */
  numeric(int16_t val)
      : type_(SHORT_TYPE) {
    as<int16_t>() = val;
  }

  /**
   * Constructs a numeric from the unsigned short value
   *
   * @param val The unsigned short value used to create this numeric
   */
  numeric(uint16_t val)
      : type_(USHORT_TYPE) {
    as<uint16_t>() = val;
  }

  /**
   * Constructs a numeric from the integer value
   *
   * @param val The integer value used to construct this numeric
   */
  numeric(int32_t val)
      : type_(INT_TYPE) {
    as<int32_t>() = val;
  }

  /**
   * Constructs a numeric from an unsigned integer value
   *
   * @param val The unsigned integer value used to construct the numeric
   */
  numeric(uint32_t val)
      : type_(UINT_TYPE) {
    as<uint32_t>() = val;
  }

  /**
   * Constructs a numeric from a signed long value
   *
   * @param val The signed long value used to construct this numeric
   */
  numeric(int64_t val)
      : type_(LONG_TYPE) {
    as<int64_t>() = val;
  }

  /**
   * Constructs a numeric from an unsigned long value
   *
   * @param val The unsigned long value used to construct this numeric
   */
  numeric(uint64_t val)
      : type_(ULONG_TYPE) {
    as<uint64_t>() = val;
  }

  /**
   * Constructs a numeric from a float value
   *
   * @param val The float value used to construct this numeric
   */
  numeric(float val)
      : type_(FLOAT_TYPE) {
    as<float>() = val;
  }
 
  /**
   * Constructs a numeric from a double precision floating point value
   *
   * @param val The double value used to create this numeric
   */  
  numeric(double val)
      : type_(DOUBLE_TYPE) {
    as<double>() = val;
  }

  /**
   * Constructs a numeric from the given data type and data
   *
   * @param type The type of data the numeric contains
   * @param data The data that the numeric contains
   * @throw invalid_cast_exception
   */
  numeric(const data_type& type, void* data)
      : type_(type) {
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    memcpy(data_, data, type_.size);
  }

  /**
   * Constructs a numeric from the given immutable value
   *
   * @param val The immutable value, which contents are used to intialize
   * this numeric
   * @throw invalid_cast_exception
   */
  numeric(const immutable_value& val)
      : type_(val.type()) {
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    memcpy(data_, val.ptr(), type_.size);
  }

  /**
   * Gets whether this numeric is valid
   *
   * @return True if this numeric is valid, false otherwise
   */
  bool is_valid() const {
    return !type_.is_none();
  }

  /**
   * Parses the numeric from a string
   *
   * @param str The string to parse the numeric from
   * @param type The data type of the numeric
   *
   * @return A numeric generated from the contents of the string
   */
  static numeric parse(const std::string& str, const data_type& type) {
    numeric value(type);
    type.parse_op()(str, value.data_);
    return value;
  }

  /**
   * Converts this numeric to raw immutable data
   *
   * @return Immutable raw data containing the data and type of this numeric
   */
  inline immutable_raw_data to_data() const {
    return immutable_raw_data(data_, type_.size);
  }

  /**
   * Gets the data type of this numeric
   *
   * @return The data type of this numeric
   */
  data_type const& type() const {
    return type_;
  }

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
  static bool relop(reational_op_id id, const numeric& first,
                    const numeric& second) {
    // Promote to the larger type
    data_type m = max(first.type_, second.type_);
    numeric v1 = cast(first, m);
    numeric v2 = cast(second, m);
    return m.relop(id)(v1.to_data(), v2.to_data());
  }

  /**
   * Less than operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is less than the data in
   * the second numeric, false otherwise
   */
  friend inline bool operator <(const numeric& first, const numeric& second) {
    return relop(reational_op_id::LT, first, second);
  }

  /**
   * Less than or equal to operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is less than or equal to
   * the data in the second numeric
   */
  friend inline bool operator <=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::LE, first, second);
  }

  /**
   * Greater than operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is greater than the 
   * data in the second numeric
   */
  friend inline bool operator >(const numeric& first, const numeric& second) {
    return relop(reational_op_id::GT, first, second);
  }

  /**
   * Greater than or equal to operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is greater than or equal
   * to the data in the second numeric
   */
  friend inline bool operator >=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::GE, first, second);
  }

  /**
   * Equality operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is equal
   * to the data in the second numeric
   */
  friend inline bool operator ==(const numeric& first, const numeric& second) {
    return relop(reational_op_id::EQ, first, second);
  }

  /**
   * Not equal operator
   *
   * @param first The first numeric in the relational expression
   * @param second The second numeric in the relational expression
   *
   * @return True if the data in the first numeric is not equal
   * to the data in the second numeric
   */
  friend inline bool operator !=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::NEQ, first, second);
  }

  // Arithmetic operations
  /**
   * Performs a unary operation on the given numeric
   *
   * @param id The unique identifier for the unary operator
   * @param n The numeric the unary operator is applied to
   *
   * @return The numeric containing the result of the unary operation
   */
  static inline numeric unaryop(unary_op_id id, const numeric& n) {
    numeric result(n.type());
    result.type_.unaryop(id)(result.data_, n.to_data());
    return result;
  }

  /**
   * Unary negation operator
   *
   * @param n The numeric to be negated
   *
   * @return A numeric containing the negated numeric
   */
  friend inline numeric operator-(const numeric& n) {
    return unaryop(unary_op_id::NEGATIVE, n);
  }

  /**
   * Unary positive operator
   *
   * @param n The numeric the positive unary operator is applied to
   *
   * @return A numeric containing the value of the given numeric
   */
  friend inline numeric operator+(const numeric& n) {
    return unaryop(unary_op_id::POSITIVE, n);
  }

  /**
   * Bitwise not unary operator
   *
   * @param n The numeric the bitwise not unary operator is applied to
   *
   * @return A numeric containing the flipped bits of the given numeric
   * value
   */
  friend inline numeric operator~(const numeric& n) {
    return unaryop(unary_op_id::BW_NOT, n);
  }

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
  static numeric binaryop(binary_op_id id, const numeric& first,
                          const numeric& second) {
    // Promote to the larger type
    data_type m = max(first.type_, second.type_);
    numeric v1 = cast(first, m);
    numeric v2 = cast(second, m);
    numeric result(m);
    m.binaryop(id)(result.data_, v1.to_data(), v2.to_data());
    return result;
  }

  /**
   * The addition operator
   *
   * @param first The first numeric in the expression
   * @param second The second numeric in the expression
   *
   * @return A numeric containing the sum of the values of the numerics
   */
  friend inline numeric operator+(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::ADD, first, second);
  }

  /**
   * The subtraction operator
   *
   * @param first The first numeric in the expression
   * @param second The second numeric in the expression
   *
   * @return A numeric containing the difference of the values 
   * of the numerics
   */
  friend inline numeric operator-(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::SUBTRACT, first, second);
  }

  /**
   * The multiplication operator
   *
   * @param first The first numeric in the expression
   * @param second The second numeric in the expression
   *
   * @return A numeric containing the product of the values 
   * of the numerics
   */
  friend inline numeric operator*(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::MULTIPLY, first, second);
  }

  /**
   * The division operator
   *
   * @param first The numeric containing the dividend
   * @param second The numeric containing the divisor
   *
   * @return A numeric containing the quotient of the expression
   */
  friend inline numeric operator/(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::DIVIDE, first, second);
  }

  /**
   * The modulo operator
   *
   * @param first The numeric containing the dividend
   * @param second The numeric containing the divisor
   *
   * @return A numeric containing the remainder of the expression
   */
  friend inline numeric operator%(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::MODULO, first, second);
  }

  /**
   * The bitwise and operator
   *
   * @param first The first numeric in the and expression
   * @param second The second numeric in the and expression
   *
   * @return A numeric containing the result of the and expression
   */
  friend inline numeric operator&(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_AND, first, second);
  }

  /**
   * The bitwise or operator
   *
   * @param first The first numeric in the or expression
   * @param second The second numeric in the or expression
   *
   * @return A numeric containing the result of the or expression
   */
  friend inline numeric operator|(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_OR, first, second);
  }

  /**
   * The bitwise xor operator
   *
   * @param first The first numeric in the xor expression
   * @param second The second numeric in the xor expression
   *
   * @return A numeric containing the result of the xor expression
   */
  friend inline numeric operator^(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_XOR, first, second);
  }

  /**
   * The bitwise left logical shift operator
   *
   * @param first The numeric containing the value that will be shifted
   * @param second The numeric containing the amount to shift
   *
   * @return A numeric containing the result of the left shift expression
   */
  friend inline numeric operator<<(const numeric& first,
                                   const numeric& second) {
    return binaryop(binary_op_id::BW_LSHIFT, first, second);
  }

  /**
   * The bitwise right logical shift operator
   *
   * @param first The numeric containing the value that will be shifted
   * @param second The numeric containing the amount to shift
   *
   * @return A numeric containing the result of the right shift expression
   */
  friend inline numeric operator>>(const numeric& first,
                                   const numeric& second) {
    return binaryop(binary_op_id::BW_RSHIFT, first, second);
  }

  /**
   * Constructs this numeric from the given immutable value
   *
   * @param other The immutable value containing the data type and value
   * that this numeric will contain
   *
   * @return This updated numeric
   */
  numeric& operator=(const immutable_value& other) {
    type_ = other.type();
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    type_.unaryop(unary_op_id::ASSIGN)(data_, other.to_data());
    return *this;
  }

  /**
   * Assigns the boolean value to this numeric
   *
   * @param value The boolean value that is copied to this numeric's value
   *
   * @return This updated numeric
   */
  numeric& operator=(bool value) {
    type_ = BOOL_TYPE;
    as<bool>() = value;
    return *this;
  }

  /**
   * Assigns the character value to this numeric
   *
   * @param value The character value that is copied to this numeric's value
   *
   * @return This updated numeric
   */
  numeric& operator=(int8_t value) {
    type_ = CHAR_TYPE;
    as<int8_t>() = value;
    return *this;
  }

  /**
   * Assigns the unsigned character value to this numeric
   *
   * @param value The unsigned character value that is copied to this
   * numeric's value
   *
   * @return The updated numeric
   */
  numeric& operator=(uint8_t value) {
    type_ = UCHAR_TYPE;
    as<uint8_t>() = value;
    return *this;
  }

  /**
   * Assigns the short value to this numeric
   *
   * @param value The short value that is copied to this numeric's value
   *
   * @return The updated numeric
   */
  numeric& operator=(int16_t value) {
    type_ = SHORT_TYPE;
    as<int16_t>() = value;
    return *this;
  }

  /**
   * Assigns the unsigned short value to this numeric
   *
   * @param value The unsigned short value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric& operator=(uint16_t value) {
    type_ = USHORT_TYPE;
    as<uint16_t>() = value;
    return *this;
  }

  /**
   * Assigns the integer value to this numeric
   *
   * @param value The integer value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric& operator=(int32_t value) {
    type_ = INT_TYPE;
    as<int32_t>() = value;
    return *this;
  }

  /**
   * Assigns the unsigned integer value to this numeric
   *
   * @param value The unsigned integer value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric& operator=(uint32_t value) {
    type_ = UINT_TYPE;
    as<uint32_t>() = value;
    return *this;
  }

  /**
   * Assigns the long value to this numeric
   *
   * @param value The long value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric& operator=(int64_t value) {
    type_ = LONG_TYPE;
    as<int64_t>() = value;
    return *this;
  }

  /**
   * Assigns the unsigned long value to this numeric
   *
   * @param value The value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric& operator=(uint64_t value) {
    type_ = ULONG_TYPE;
    as<uint64_t>() = value;
    return *this;
  }

  /**
   * Assigns the float value to this numeric
   *
   * @param value The float value to assign to this numeric
   *
   * @return The updated numeric
   */
  numeric& operator=(float value) {
    type_ = FLOAT_TYPE;
    as<float>() = value;
    return *this;
  }

  /**
   * Assigns the double value to this numeric
   *
   * @param value The double value that is copied to this numeric's value
   *
   * @return This updated numeric
   */
  numeric& operator=(double value) {
    type_ = DOUBLE_TYPE;
    as<double>() = value;
    return *this;
  }

  /**
   * Casts the numeric's data to type T
   *
   * @tparam T The desired data type
   *
   * @return The value of type T
   */
  template<typename T,
      typename std::enable_if<
          std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* =
          nullptr>
  T& as() {
    return *reinterpret_cast<T*>(data_);
  }

  /**
   * Casts the numeric's data to type T
   *
   * @tparam T The desired data type
   *
   * @return The value of type T that is not modifiable
   */
  template<typename T,
      typename std::enable_if<
          std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* =
          nullptr>
  const T& as() const {
    return *reinterpret_cast<const T*>(data_);
  }

  /**
   * Gets a string representation of the numeric
   *
   * @return A formatted string containing the type and data of the numeric
   */
  std::string to_string() const {
    return type_.name() + "(" + type_.to_string_op()(to_data()) + ")";
  }

  data_type type() {
    return type_;
  }

  uint8_t* data() {
    return data_;
  }

 private:
  data_type type_;
  uint8_t data_[MAX_SIZE];
};

// Cast functions for primitive types
using cast_fn = numeric (*)(const numeric& v);

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
  static numeric cast(const numeric& v) {
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
  static numeric cast(const numeric& v) {
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
  static numeric cast(const numeric& v) {
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
  static numeric cast(const numeric& v) {
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
  static numeric cast(const numeric& v) {
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
numeric type_cast(const numeric& v) {
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
static std::vector<cast_fn> init_type_cast_ops() {
  return {type_cast<IN, void>, type_cast<IN, bool>, type_cast<IN, int8_t>, type_cast<IN, uint8_t>,
    type_cast<IN, int16_t>, type_cast<IN, uint16_t>, type_cast<IN, int32_t>,
    type_cast<IN, uint32_t>, type_cast<IN, int64_t>, type_cast<IN, uint64_t>,
    type_cast<IN, float>, type_cast<IN, double>};
}

/**
 * A vector containing the cast operators for all the types
 */
static std::vector<std::vector<cast_fn>> CAST_OPS = {
    init_type_cast_ops<void>(), init_type_cast_ops<bool>(), init_type_cast_ops<
        int8_t>(), init_type_cast_ops<uint8_t>(), init_type_cast_ops<int16_t>(),
    init_type_cast_ops<uint16_t>(), init_type_cast_ops<int32_t>(),
    init_type_cast_ops<uint32_t>(), init_type_cast_ops<int64_t>(),
    init_type_cast_ops<uint64_t>(), init_type_cast_ops<float>(),
    init_type_cast_ops<double>() };

/**
 * Indexes into the cast operators vector to call the appropriate cast
 * function
 *
 * @param val The numeric to cast
 * @param type The type to cast the numeric to
 *
 * @return The casted numeric
 */
static numeric cast(const numeric& val, const data_type& type) {
  return CAST_OPS[val.type().id][type.id](val);
}

}

#endif /* CONFLUO_TYPES_NUMERIC_H_ */
