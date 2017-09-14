#ifndef DIALOG_CONFIGURATION_PARSER_H_
#define DIALOG_CONFIGURATION_PARSER_H_

#include <cstdlib>

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <exception>

#include "logger.h"
#include "file_utils.h"

namespace utils {

class config_utils {
 public:
  static std::string read_from_env(const std::string& env_var,
                                   const std::string& default_val) {
    if (const char* env_p = std::getenv(env_var.c_str())) {
      return std::string(env_p);
    }
    return default_val;
  }
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

  configuration_map(const std::string& paths)
      : paths_(paths) {
    bool found = false;
    std::vector<std::string> path_vector = string_utils::split(paths, ':');
    for (std::string p : path_vector) {
      if (file_utils::exists_file(p)) {
        extract_all(p);
        found = true;
        break;
      }
    }

    if (!found) {
      LOG_WARN <<"Could not find configuration file in any of "
          << string_utils::mk_string(path_vector, ", ");
    }
  }

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

  void remove_comment(std::string& line) const {
    std::string::size_type pos;
    if ((pos = line.find("#")) != line.npos)
    line.erase(pos);
  }

  bool is_empty(const std::string& line) const {
    return line.find_first_not_of(' ') == line.npos;
  }

  bool is_valid(const std::string& line) const {
    std::string temp = line;
    temp.erase(0, temp.find_first_not_of("\t "));
    if (temp[0] == '=')
    return false;

    for (size_t i = temp.find(':') + 1; i < temp.length(); i++)
    if (temp[i] != ' ')
    return true;

    return false;
  }

  std::string extract_key(size_t sep_pos, const std::string& line) const {
    std::string key = line.substr(0, sep_pos);
    if (key.find('\t') != line.npos || key.find(' ') != line.npos)
    key.erase(key.find_first_of("\t "));
    return key;
  }

  std::string extract_value(size_t sep_pos, const std::string& line) {
    std::string value = line.substr(sep_pos + 1);
    value.erase(0, value.find_first_not_of("\t "));
    value.erase(value.find_last_not_of("\t ") + 1);
    return value;
  }

  void extract_entry(const std::string& line) {
    std::string temp = line;
    temp.erase(0, temp.find_first_not_of("\t "));
    size_t sep_pos = temp.find(':');

    std::string key = extract_key(sep_pos, temp);
    std::string value = extract_value(sep_pos, temp);

    conf_map_.insert(std::make_pair(key, value));
  }

  void parse_line(const std::string& line, size_t line_no) {
    if (line.find(':') == line.npos)
    throw configuration_exception(
        "Could not find ':' on line#" + std::to_string(line_no) + " '" + line
        + "'");

    if (!is_valid(line))
    throw configuration_exception(
        "Bad format for line#" + std::to_string(line_no) + " '" + line + "'");

    extract_entry(line);
  }

  void extract_all(std::string& path) {
    std::ifstream cfile;
    cfile.open(path.c_str());
    if (!cfile) {
      throw configuration_exception("Could not open config file " + path);
    }

    std::string line;
    size_t line_no = 0;
    while (std::getline(cfile, line)) {
      line_no++;
      std::string temp = line;

      if (temp.empty())
      continue;

      remove_comment(temp);
      if (is_empty(temp))
      continue;

      parse_line(temp, line_no);
    }

    cfile.close();
  }

  std::string paths_;
  conf_map_type conf_map_;
};

}

#endif /* DIALOG_CONFIGURATION_PARSER_H_ */
