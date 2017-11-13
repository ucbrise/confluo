#ifndef DIALOG_STRING_OPS_H_
#define DIALOG_STRING_OPS_H_

#include <vector>
#include <string>

#include "string_utils.h"

using namespace utils;

namespace dialog {

typedef data (*parse_op_t)(const std::string&);

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

}

#endif /* DIALOG_STRING_OPS_H_ */
