#ifndef CONFLUO_TYPES_STRING_OPS_H_
#define CONFLUO_TYPES_STRING_OPS_H_

#include <string>

#include "string_utils.h"

using namespace utils;

namespace confluo {

typedef void (*parse_op_t)(const std::string&, void*);
typedef std::string (*to_string_op_t)(const immutable_raw_data&);

template<typename T>
void parse(const std::string& str, void* out) {
  try {
    T val = string_utils::lexical_cast<T>(str);
    memcpy(out, &val, sizeof(T));
  } catch (std::bad_cast& ex) {
    throw invalid_operation_exception("Cannot parse " + str);
  }
}

template<>
void parse<void>(const std::string& str, void* out) {
  THROW(unsupported_exception, "Cannot parse none type");
}

template<>
void parse<std::string>(const std::string& str, void* out) {
  memcpy(out, str.data(), str.length());
  *(reinterpret_cast<char*>(out) + str.length()) = '\0';
}

template<typename T>
std::string to_string(const immutable_raw_data& data) {
  return std::to_string(data.as<T>());
}

template<>
std::string to_string<bool>(const immutable_raw_data& data) {
  return data.as<bool>() ? "true" : "false";
}

template<>
std::string to_string<int8_t>(const immutable_raw_data& data) {
  return std::string(1, data.as<char>());
}

template<>
std::string to_string<void>(const immutable_raw_data& data) {
  THROW(unsupported_exception, "Cannot convert none type to string");
}

template<>
std::string to_string<std::string>(const immutable_raw_data& data) {
  return std::string(reinterpret_cast<const char*>(data.ptr));
}

}

#endif /* CONFLUO_TYPES_STRING_OPS_H_ */
