#ifndef CONFLUO_TYPES_STRING_OPS_H_
#define CONFLUO_TYPES_STRING_OPS_H_

#include <string>

#include "string_utils.h"

using namespace utils;

namespace confluo {

typedef void (*parse_op_t)(const std::string&, mutable_raw_data&);
typedef std::string (*to_string_op_t)(const immutable_raw_data&);

template<typename T>
void parse(const std::string& str, mutable_raw_data& out) {
  out.set(string_utils::lexical_cast<T>(str));
}

template<>
void parse<void>(const std::string& str, mutable_raw_data& out) {
  THROW(unsupported_exception, "Cannot parse none type");
}

template<>
void parse<std::string>(const std::string& str, mutable_raw_data& out) {
  out.set(str);
}

template<typename T>
std::string to_string(const immutable_raw_data& data) {
  return std::to_string(data.as<T>());
}

template<>
std::string to_string<void>(const immutable_raw_data& data) {
  THROW(unsupported_exception, "Cannot convert none type to string");
}

template<>
std::string to_string<std::string>(const immutable_raw_data& data) {
  return data.as<std::string>();
}

}

#endif /* CONFLUO_TYPES_STRING_OPS_H_ */
