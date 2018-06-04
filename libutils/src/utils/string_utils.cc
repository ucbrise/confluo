#include "string_utils.h"

namespace utils {

template<>
bool string_utils::lexical_cast<bool>(const std::string &s) {
  std::stringstream ss(to_lower(s));

  bool result;
  if ((ss >> std::boolalpha >> result).fail() || !(ss >> std::ws).eof()) {
    throw std::bad_cast();
  }

  return result;
}

std::vector<std::string> string_utils::split(const std::string &s, char delim, size_t count) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  size_t i = 0;
  while (std::getline(ss, item, delim) && i < count) {
    elems.push_back(std::move(item));
    i++;
  }
  while (std::getline(ss, item, delim))
    elems.back() += item;
  return elems;
}

std::vector<std::string> string_utils::split(const std::string &s, char delim) {
  return split(s, delim, UINT64_MAX);
}

std::string string_utils::mk_string(const std::vector<std::string> &v, const std::string &delim) {
  std::string str = "";
  size_t i = 0;
  for (; i < v.size() - 1; i++) {
    str += v[i] + delim;
  }
  return str + v[i];
}

std::string string_utils::to_upper(const std::string &str) {
  return transform(str, ::toupper);
}

std::string string_utils::to_lower(const std::string &str) {
  return transform(str, ::tolower);
}

}