#ifndef CONFLUO_TYPES_STRING_OPS_H_
#define CONFLUO_TYPES_STRING_OPS_H_

#include <string>

#include "string_utils.h"

using namespace utils;

namespace confluo {

/** Parses the contents from the given string to the data */
typedef void (*parse_op_t)(const std::string&, void*);
/** Gets the string representation of the immutable raw data */
typedef std::string (*to_string_op_t)(const immutable_raw_data&);

/**
 * Parses the given string and sets the specified pointer to the contents
 * of the string
 *
 * @tparam T The type of data
 * @param str The string that is parsed
 * @param out The pointer that is set
 */
template<typename T>
void parse(const std::string& str, void* out) {
  try {
    T val = string_utils::lexical_cast<T>(str);
    memcpy(out, &val, sizeof(T));
  } catch (std::bad_cast& ex) {
    throw invalid_operation_exception("Cannot parse " + str);
  }
}

/**
 * Parses the given string and sets the specified pointer to the contents
 * of the string, for the void data type
 *
 * @param str The string that is parsed
 * @param out The pointer that is set
 * @throw unsupported_exception This operation is not defined for the void
 * type
 */
template<>
void parse<void>(const std::string& str, void* out) {
  THROW(unsupported_exception, "Cannot parse none type");
}

/**
 * Parses the given string and sets the specified pointer to the contents
 * of the string, for the string data type
 *
 * @param str The string that is parsed
 * @param out The pointer that is set
 */
template<>
void parse<std::string>(const std::string& str, void* out) {
  memcpy(out, str.data(), str.length());
  *(reinterpret_cast<char*>(out) + str.length()) = '\0';
}

/**
 * Gets the string representation for the specified raw immutable data 
 *
 * @tparam T The data type 
 * @param data The raw immutable data to get the string representation of
 *
 * @return String representation of the immutable data
 */
template<typename T>
std::string to_string(const immutable_raw_data& data) {
  return std::to_string(data.as<T>());
}

/**
 * Gets the string representation for the specified raw immutable data,
 * for the boolean data type
 *
 * @param data The raw immutable data to get the string representation of
 *
 * @return String representation of the immutable data
 */
template<>
std::string to_string<bool>(const immutable_raw_data& data) {
  return data.as<bool>() ? "true" : "false";
}

/**
 * Gets the string representation for the specified raw immutable data,
 * for the character data type
 *
 * @param data The raw immutable data to get the string representation of
 *
 * @return String representation of the immutable data
 */
template<>
std::string to_string<int8_t>(const immutable_raw_data& data) {
  return std::string(1, data.as<char>());
}

/**
 * Gets the string representation for the specified raw immutable data,
 * for the void data type
 *
 * @param data The raw immutable data to get the string representation of
 * @throw unsupported_exception This operation is not defined for void
 * types
 *
 * @return String representation of the immutable data
 */
template<>
std::string to_string<void>(const immutable_raw_data& data) {
  THROW(unsupported_exception, "Cannot convert none type to string");
}

/**
 * Gets the string representation for the specified raw immutable data,
 * for the string data type
 *
 * @param data The raw immutable data to get the string representation of
 *
 * @return String representation of the immutable data
 */
template<>
std::string to_string<std::string>(const immutable_raw_data& data) {
  return std::string(reinterpret_cast<const char*>(data.ptr));
}

}

#endif /* CONFLUO_TYPES_STRING_OPS_H_ */
