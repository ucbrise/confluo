#ifndef UTILS_CONFIG_UTILS_H_
#define UTILS_CONFIG_UTILS_H_

#include <cstdlib>

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <exception>

#include "logger.h"
#include "file_utils.h"
#include "string_utils.h"

namespace utils {

class config_utils {
 public:
  static std::string read_from_env(const std::string& env_var,
                                   const std::string& default_val);
};

class configuration_exception : public std::exception {
 public:
  configuration_exception() = default;

  configuration_exception(const std::string& msg)
      : msg_(msg) {
  }

  const char* what() const noexcept {
    return msg_.c_str();
  }

 private:
  std::string msg_;
};

class configuration_map {
 public:
  typedef std::map<std::string, std::string> conf_map_type;

  configuration_map(const std::string& paths);

  template<typename T>
  T get(const std::string& key, const T& default_val = T()) {
    conf_map_type::iterator it;
    if ((it = conf_map_.find(key)) == conf_map_.end())
    return default_val;

    return as<T>(it->second);
  }

private:
  template<typename T>
  static T as(std::string const &val) {
    std::istringstream istr(val);
    T ret;
    if (!(istr >> ret))
    throw configuration_exception(
        "Invalid " + std::string(typeid(T).name()) + " received.");

    return ret;
  }

  void remove_comment(std::string& line) const;

  bool is_empty(const std::string& line) const;

  bool is_valid(const std::string& line) const;

  std::string extract_key(size_t sep_pos, const std::string& line) const;

  std::string extract_value(size_t sep_pos, const std::string& line);

  void extract_entry(const std::string& line);

  void parse_line(const std::string& line, size_t line_no);

  void extract_all(std::string& path);

  std::string paths_;
  conf_map_type conf_map_;
};

}

#endif /* UTILS_CONFIG_UTILS_H_ */
