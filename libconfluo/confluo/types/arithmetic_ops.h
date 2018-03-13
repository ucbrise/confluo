#ifndef CONFLUO_TYPES_ARITHMETIC_OPS_H_
#define CONFLUO_TYPES_ARITHMETIC_OPS_H_

#include <cstdint>

#include "exceptions.h"
#include "raw_data.h"

namespace confluo {

/**
 * Unary arithmetic/bitwise operators
 */
enum unary_op_id
  : uint8_t {
    ASSIGN = 0,  //!< ASSIGN (=)
  NEGATIVE = 1,  //!< NEGATIVE (-)
  POSITIVE = 2,  //!< POSITIVE (+)
  BW_NOT = 3   //!< BW_NOT (~)
};

/**
 * Binary arithmetic/bitwise operators
 */
enum binary_op_id
  : uint8_t {
    ADD = 0,      //!< ADD (+)
  SUBTRACT = 1,  //!< SUBTRACT (-)
  MULTIPLY = 2,  //!< MULTIPLY (*)
  DIVIDE = 3,   //!< DIVIDE (/)
  MODULO = 4,   //!< MODULO (%)
  BW_AND = 5,   //!< BW_AND (&)
  BW_OR = 6,    //!< BW_OR (|)
  BW_XOR = 7,   //!< BW_XOR (^)
  BW_LSHIFT = 8,   //!< BW_LSHIFT (<<)
  BW_RSHIFT = 9  //!< BW_RSHIFT (>>)
};

/** Function pointer for a unary operator */
typedef void (*unary_op_t)(void* res, const immutable_raw_data& v);

/** Function pointer for a binary operator */
typedef void (*binary_op_t)(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2);

/** List of unary operators */
typedef std::vector<unary_op_t> unary_ops_t;
/** List of binary operators */
typedef std::vector<binary_op_t> binary_ops_t;

// Unary arithmetic operators
/**
 * Assigns the value of the raw immutable data to the result pointer
 * @tparam T The type of data
 * @param res The pointer to the resultant data
 * @param v The raw immutable data to assign to the result pointer
 */
template<typename T>
inline void assign(void* res, const immutable_raw_data& v) {
  *(reinterpret_cast<T*>(res)) = v.as<T>();
}

/**
 * Assigns the value of the raw immutable data to the result pointer for
 * string types
 *
 * @param res The result pointer to initialize
 * @param v The value of the raw immutable data to copy
 */
template<>
inline void assign<std::string>(void* res, const immutable_raw_data& v) {
  memcpy(res, v.ptr, v.size);
}

/**
 * Assigns the value of the raw immutable data to the result pointer for
 * void types
 *
 * @param res The result pointer to initialize
 * @param v The raw immutable data to copy
 */
template<>
inline void assign<void>(void* res, const immutable_raw_data& v) {
  return;
}

/**
 * Negates the raw immutable data and stores it in the result pointer
 *
 * @tparam T The type of data
 * @param res The result pointer which will contain the negated value
 * @param v The immutable raw data to negate
 */
template<typename T>
inline void negative(void* res, const immutable_raw_data& v) {
  *(reinterpret_cast<T*>(res)) = -v.as<T>();
}

/**
 * Negates the raw immutable data and has result pointer point
 * to the negated value for the string type
 *
 * @param res The result pointer that will point to the negated value
 * @param v The immutable raw data to be negated
 *
 * @throw unsupported_exception
 */
template<>
inline void negative<std::string>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "- not supported for string type");
}

/**
 * Negates the raw immutable data and has the result pointer point to the
 * negated value for the none type
 *
 * @param res The result pointer that will point to the negated value
 * @param v The immutable raw data to be negated
 *
 * @throw unsupported_exception
 */
template<>
inline void negative<void>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "- not supported for none type");
}

/**
 * Has the result pointer point to the value of the raw immutable data
 *
 * @tparam T The type of data
 * @param res The result pointer that points to the value
 * @param v The raw immutable data that contains the value
 */
template<typename T>
inline void positive(void* res, const immutable_raw_data& v) {
  *(reinterpret_cast<T*>(res)) = +v.as<T>();
}

/**
 * Has the result pointer point to the value of the raw immutable data
 * for the string type
 *
 * @tparam T The type of data
 * @param res The result pointer that points to the value
 * @param v The raw immutable data that contains the value
 * @throw unsupported_exception
 */
template<>
inline void positive<std::string>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "+ not supported for string type");
}

/**
 * Has the result pointer point to the value of the raw immutable data
 * for the void type
 *
 * @tparam T The type of data
 * @param res The result pointer that points to the value
 * @param v The raw immutable data that contains the value
 * @throw unsupported_exception
 */
template<>
inline void positive<void>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "+ not supported for none type");
}

// Binary arithmetic operators
/**
 * Performs binary addition and stores the result in the result pointer
 *
 * @tparam T The data type of the operands
 * @param res The result of the addition
 * @param v1 The first operand of the addition expression
 * @param v2 The second operand of the addition expression
 */
template<typename T>
inline void add(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() + v2.as<T>();
}

/**
 * Performs binary addition and stores the result in the result pointer
 * for the string type
 *
 * @param res The result of the addition
 * @param v1 The first operand of the addition expression
 * @param v2 The second operand of the addition expression
 */
template<>
inline void add<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "+ not supported for string type");
}

/**
 * Performs binary addition and stores the result in the result pointer
 * for the none type
 *
 * @param res The result of the addition
 * @param v1 The first operand of the addition expression
 * @param v2 The second operand of the addition expression
 * @throw unsupported_exception
 */
template<>
inline void add<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "+ not supported for none type");
}

/**
 * Performs binary subtraction and stores the result in the result pointer
 *
 * @tparam The data type
 * @param res The result of the subtraction
 * @param v1 The first operand of the subtraction expression
 * @param v2 The second operand of the subtraction expression
 */
template<typename T>
inline void subtract(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() - v2.as<T>();
}

/**
 * Performs binary subtraction and stores the result in the result pointer
 * for strings
 *
 * @param res The result of the subtraction
 * @param v1 The first operand of the subtraction expression
 * @param v2 The second operand of the subtraction expression
 * @throw unsupported_exception
 */
template<>
inline void subtract<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "- not supported for string type");
}

/**
 * Performs binary subtraction and stores the result in the result pointer
 * for none type
 *
 * @param res The result of the subtraction
 * @param v1 The first operand of the subtraction expression
 * @param v2 The second operand of the subtraction expression
 * @throw unsupported_exception
 */
template<>
inline void subtract<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "- not supported for none type");
}

/**
 * Performs binary multiplication and stores the result in the 
 * result pointer
 *
 * @tparam T The data type of the immutable values
 * @param res The result of the multiplication
 * @param v1 The first operand of the multiplication expression
 * @param v2 The second operand of the multiplication expression
 */
template<typename T>
inline void multiply(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() * v2.as<T>();
}

/**
 * Performs binary multiplication and stores the result in the 
 * result pointer for strings
 *
 * @param res The result of the multiplication
 * @param v1 The first operand of the multiplication expression
 * @param v2 The second operand of the multiplication expression
 * @throw unsupported_exception
 */
template<>
inline void multiply<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "* not supported for string type");
}

/**
 * Performs binary multiplication and stores the result in the 
 * result pointer for the none type
 *
 * @param res The result of the addition
 * @param v1 The first operand of the multiplication expression
 * @param v2 The second operand of the multiplication expression
 * @throw unsupported_exception
 */
template<>
inline void multiply<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "* not supported for none type");
}

/**
 * Performs binary division and stores the result in the 
 * result pointer 
 *
 * @tparam The data type of the immutable values
 * @param res The result of the division
 * @param v1 The immutable value containing the dividend
 * @param v2 The immutable value containing the divisor
 */
template<typename T>
inline void divide(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() / v2.as<T>();
}

/**
 * Performs binary division and stores the result in the 
 * result pointer for strings
 *
 * @param res The result of the division
 * @param v1 The immutable value containing the dividend
 * @param v2 The immutable value containing the divisor
 * @throw unsupported_exception
 */
template<>
inline void divide<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "/ not supported for string type");
}

/**
 * Performs binary division and stores the result in the 
 * result pointer for the none type
 *
 * @param res The result of the division
 * @param v1 The immutable value containing the dividend
 * @param v2 The immutable value containing the divisor
 * @throw unsupported_exception
 */
template<>
inline void divide<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "/ not supported for none type");
}

/**
 * Performs binary division and stores the remainder in the 
 * result pointer 
 *
 * @tparam The data type of the immutable values
 * @param res The remainder of the division
 * @param v1 The immutable value containing the dividend
 * @param v2 The immutable value containing the divisor
 */
template<typename T>
inline void modulo(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() % v2.as<T>();
}

/**
 * Performs binary division and stores the remainder in the 
 * result pointer for strings
 *
 * @param res The remainder of the division
 * @param v1 The immutable value containing the dividend
 * @param v2 The immutable value containing the divisor
 * @throw unsupported_exception
 */
template<>
inline void modulo<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "% not supported for string type");
}

/**
 * Performs binary division and stores the remainder in the 
 * result pointer for the none type
 *
 * @param res The remainder of the division
 * @param v1 The immutable value containing the dividend
 * @param v2 The immutable value containing the divisor
 * @throw unsupported_exception
 */
template<>
inline void modulo<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "% not supported for none type");
}

/**
 * Performs binary division and stores the remainder in the 
 * result pointer for the float type
 *
 * @param res The remainder of the division
 * @param v1 The immutable value containing the dividend
 * @param v2 The immutable value containing the divisor
 * @throw unsupported_exception
 */
template<>
inline void modulo<float>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "% not supported for float type");
}

/**
 * Performs binary division and stores the remainder in the 
 * result pointer for the double type
 *
 * @param res The remainder of the division
 * @param v1 The immutable value containing the dividend
 * @param v2 The immutable value containing the divisor
 * @throw unsupported_exception
 */
template<>
inline void modulo<double>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "% not supported for double type");
}

// Bitwise operators

/**
 * Performs bitwise not operation and stores the result in the 
 * result pointer 
 *
 * @tparam T The type of data the immutable value contains
 * @param res The result of the bitwise not operation
 * @param v1 The immutable value that the operator is applied to
 */
template<typename T>
inline void bw_not(void* res, const immutable_raw_data& v) {
  *(reinterpret_cast<T*>(res)) = ~v.as<T>();
}

/**
 * Performs bitwise not operation and stores the result in the 
 * result pointer for strings
 *
 * @param res The result of the bitwise not operation
 * @param v1 The immutable value that the operator is applied to
 * @throw unsupported_exception
 */
template<>
inline void bw_not<std::string>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "~ not supported for string type");
}

/**
 * Performs bitwise not operation and stores the result in the 
 * result pointer for the void type
 *
 * @param res The result of the bitwise not operation
 * @param v1 The immutable value that the operator is applied to
 * @throw unsupported_exception
 */
template<>
inline void bw_not<void>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "~ not supported for none type");
}

/**
 * Performs bitwise not operation and stores the result in the 
 * result pointer for the float type
 *
 * @param res The result of the bitwise not operation
 * @param v1 The immutable value that the operator is applied to
 * @throw unsupported_exception
 */
template<>
inline void bw_not<float>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "~ not supported for float type");
}

/**
 * Performs bitwise not operation and stores the result in the 
 * result pointer for the double type
 *
 * @param res The result of the bitwise not operation
 * @param v1 The immutable value that the operator is applied to
 * @throw unsupported_exception
 */
template<>
inline void bw_not<double>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "~ not supported for double type");
}

/**
 * Performs bitwise and operation and stores the result in the 
 * result pointer 
 *
 * @tparam T The data type of the immutable data values
 * @param res The result of the bitwise and operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 */
template<typename T>
inline void bw_and(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() & v2.as<T>();
}

/**
 * Performs bitwise and operation and stores the result in the 
 * result pointer for strings
 *
 * @param res The result of the bitwise and operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_and<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "& not supported for string type");
}

/**
 * Performs bitwise and operation and stores the result in the 
 * result pointer for the none type
 *
 * @param res The result of the bitwise and operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_and<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "& not supported for none type");
}

/**
 * Performs bitwise and operation and stores the result in the 
 * result pointer for floats
 *
 * @param res The result of the bitwise and operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_and<float>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "& not supported for float type");
}

/**
 * Performs bitwise and operation and stores the result in the 
 * result pointer for doubles
 *
 * @param res The result of the bitwise and operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_and<double>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "& not supported for double type");
}

/**
 * Performs bitwise or operation and stores the result in the 
 * result pointer 
 *
 * @tparam T The type of data of the immutable values
 * @param res The result of the bitwise or operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 */
template<typename T>
inline void bw_or(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() | v2.as<T>();
}

/**
 * Performs bitwise or operation and stores the result in the 
 * result pointer for strings
 *
 * @param res The result of the bitwise or operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_or<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "| not supported for string type");
}

/**
 * Performs bitwise or operation and stores the result in the 
 * result pointer for the none type
 *
 * @param res The result of the bitwise or operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_or<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "| not supported for none type");
}

/**
 * Performs bitwise or operation and stores the result in the 
 * result pointer for the float type
 *
 * @param res The result of the bitwise or operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_or<float>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "| not supported for float type");
}

/**
 * Performs bitwise or operation and stores the result in the 
 * result pointer for the double type
 *
 * @param res The result of the bitwise or operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_or<double>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "| not supported for double type");
}

/**
 * Performs bitwise xor operation and stores the result in the 
 * result pointer 
 *
 * @tparam T the type of the immutable raw data
 * @param res The result of the bitwise xor operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 */
template<typename T>
inline void bw_xor(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>()
      ^ v2.as<T>();
}

/**
 * Performs bitwise xor operation and stores the result in the 
 * result pointer for strings
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_xor<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "^ not supported for string type");
}

/**
 * Performs bitwise xor operation and stores the result in the 
 * result pointer for the none type
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_xor<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "^ not supported for none type");
}

/**
 * Performs bitwise xor operation and stores the result in the 
 * result pointer for floats
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_xor<float>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "^ not supported for float type");
}

/**
 * Performs bitwise xor operation and stores the result in the 
 * result pointer for doubles
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The first immutable value in the expression
 * @param v2 The second immutable value in the expression
 * @throw unsupported_exception
 */
template<>
inline void bw_xor<double>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "^ not supported for double type");
}

/**
 * Performs bitwise left shift operation and stores the result in the 
 * result pointer 
 *
 * @tparam T The data type of the immutable raw data
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 */
template<typename T>
inline void bw_lshift(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() << v2.as<T>();
}

/**
 * Performs bitwise left shift operation and stores the result in the 
 * result pointer for strings
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 * @throw unsupported_exception
 */
template<>
inline void bw_lshift<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "<< not supported for string type");
}

/**
 * Performs bitwise left shift operation and stores the result in the 
 * result pointer for the none type
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 * @throw unsupported_exception
 */
template<>
inline void bw_lshift<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "<< not supported for none type");
}

/**
 * Performs bitwise left shift operation and stores the result in the 
 * result pointer for floats
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 * @throw unsupported_exception
 */
template<>
inline void bw_lshift<float>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "<< not supported for float type");
}

/**
 * Performs bitwise left shift operation and stores the result in the 
 * result pointer for doubles
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 * @throw unsupported_exception
 */
template<>
inline void bw_lshift<double>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, "<< not supported for double type");
}

/**
 * Performs bitwise right shift operation and stores the result in the 
 * result pointer
 *
 * @tparam T The data type of the immutable raw data values
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 */
template<typename T>
inline void bw_rshift(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  *(reinterpret_cast<T*>(res)) = v1.as<T>() >> v2.as<T>();
}

/**
 * Performs bitwise right shift operation and stores the result in the 
 * result pointer for strings
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 * @throw unsupported_exception
 */
template<>
inline void bw_rshift<std::string>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, ">> not supported for string type");
}

/**
 * Performs bitwise right shift operation and stores the result in the 
 * result pointer for the none type
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 * @throw unsupported_exception
 */
template<>
inline void bw_rshift<void>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, ">> not supported for none type");
}

/**
 * Performs bitwise right shift operation and stores the result in the 
 * result pointer for floats
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 * @throw unsupported_exception
 */
template<>
inline void bw_rshift<float>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, ">> not supported for float type");
}

/**
 * Performs bitwise right shift operation and stores the result in the 
 * result pointer for doubles
 *
 * @param res The result of the bitwise xor operation
 * @param v1 The value to shift
 * @param v2 The amount to shift by
 * @throw unsupported_exception
 */
template<>
inline void bw_rshift<double>(void* res, const immutable_raw_data& v1, const immutable_raw_data& v2) {
  THROW(unsupported_exception, ">> not supported for double type");
}

/**
 * Gets a list of the unary operators for the given type
 *
 * @tparam T The data type the operators act on
 *
 * @return A vector containing the unary operator functions
 */
template<typename T>
static unary_ops_t init_unaryops() {
  return {assign<T>, negative<T>, positive<T>, bw_not<T>};
}

/**
 * Gets a list of binary operators for the given type
 *
 * @tparam T The data type the operators act on
 *
 * @return A vector containing the binary operator functions
 */
template<typename T>
static binary_ops_t init_binaryops() {
  return {add<T>, subtract<T>, multiply<T>, divide<T>,
    modulo<T>, bw_and<T>, bw_or<T>, bw_xor<T>, bw_lshift<T>, bw_rshift<T>};
}

}

#endif /* CONFLUO_TYPES_ARITHMETIC_OPS_H_ */
