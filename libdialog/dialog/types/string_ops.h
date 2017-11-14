#ifndef DIALOG_STRING_OPS_H_
#define DIALOG_STRING_OPS_H_

#include <vector>
#include <string>

#include "string_utils.h"

using namespace utils;

namespace dialog {

typedef void (*parse_op_t)(const std::string&, mutable_raw_data&);

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

}

#endif /* DIALOG_STRING_OPS_H_ */
