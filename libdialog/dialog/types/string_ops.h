#ifndef DIALOG_STRING_OPS_H_
#define DIALOG_STRING_OPS_H_

#include <vector>
#include <string>

#include "string_utils.h"

using namespace utils;

namespace dialog {

typedef data (*from_string_op)(const std::string&);

template<typename T>
data parse(const std::string& str) {
  return data(new T(string_utils::lexical_cast<T>(str)), sizeof(T));
}

template<>
data parse<void>(const std::string& str) {
  return data(nullptr, 0);
}

template<>
data parse<std::string>(const std::string& str) {
  char* characters = new char[strlen(str.c_str()) + 1];
  strcpy(characters, str.c_str());
  return data(characters, strlen(str.c_str()) + 1);
}

data parse_void(const std::string &str) {
  return data(new char[0], 0);
}

data parse_bool(const std::string& str) {
  bool val = string_utils::lexical_cast<bool>(str);
  return data(new bool(val), sizeof(bool));
}

data parse_char(const std::string& str) {
  int8_t val = string_utils::lexical_cast<int8_t>(str);
  return data(new int8_t(val), sizeof(int8_t));
}

data parse_short(const std::string& str) {
  int16_t val = string_utils::lexical_cast<int16_t>(str);
  return data(new int16_t(val), sizeof(int16_t));
}

data parse_int(const std::string& str) {
  int32_t val = string_utils::lexical_cast<int32_t>(str);
  return data(new int32_t(val), sizeof(int32_t));
}

data parse_long(const std::string& str) {
  int64_t val = string_utils::lexical_cast<int64_t>(str);
  return data(new int64_t(val), sizeof(int64_t));
}

data parse_float(const std::string& str) {
  float val = string_utils::lexical_cast<float>(str);
  return data(new float(val), sizeof(float));
}

data parse_double(const std::string& str) {
  double val = string_utils::lexical_cast<double>(str);
  return data(new double(val), sizeof(double));
}

data parse_string(const std::string& str) {
  char* characters = new char[strlen(str.c_str()) + 1];
  strcpy(characters, str.c_str());
  return data(characters, strlen(str.c_str()) + 1);
}

static std::vector<data (*)(const std::string&)> init_parsers() {
  return {&parse_void, &parse_bool, &parse_char, &parse_short, &parse_int, &parse_long, &parse_float, &parse_double, &parse_string};
}

}

#endif /* DIALOG_STRING_OPS_H_ */
