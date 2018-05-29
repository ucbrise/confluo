#ifndef UTILS_CMD_PARSE_H_
#define UTILS_CMD_PARSE_H_

#include <cassert>
#include <exception>
#include <iomanip>
#include <map>
#include <getopt.h>
#include <string.h>
#include <sstream>
#include <vector>

class cmd_parse_exception : std::exception {
 public:
  cmd_parse_exception(const std::string& msg) {
    msg_ = msg.c_str();
  }

  cmd_parse_exception(const char* msg) {
    msg_ = msg;
  }

  virtual const char* what() const throw () {
    return msg_;
  }

 private:
  const char* msg_;
};

class cmd_option {
 public:
  friend class cmd_options;
  friend class cmd_parser;

  cmd_option(const std::string& lopt, char sopt, bool flag);

  cmd_option& set_required(const bool value);

  cmd_option& set_default(const std::string& value);

  cmd_option& set_description(const std::string& value);

  std::string desc_str() const;

  std::string opt_str() const;

  std::string help_line(size_t opt_width) const;

 private:
  option to_option() const;

  std::string lopt_;
  char sopt_;
  int has_arg_;
  std::string desc_;
  std::string option_str_;
  std::string default_;
  bool required_;
};

class cmd_options {
 public:
  friend class cmd_parser;

  cmd_options();

  void add(const cmd_option& opt);

 private:
  cmd_options finalize();

  std::vector<cmd_option> copts_;
  std::string sopts_;
  std::vector<option> lopts_;
  std::map<char, size_t> sopt_to_idx_;
};

class cmd_parser {
 public:
  cmd_parser(int argc, char* const * argv, cmd_options& opts);

  std::string get(const std::string& key) const;

  int get_int(const std::string& key) const;

  long get_long(const std::string& key) const;

  float get_float(const std::string& key) const;

  double get_double(const std::string& key) const;

  bool get_flag(const std::string& key) const;

  std::string help_msg();

  std::string parsed_values();

 private:
  void parse();

  int argc_;
  char* const * argv_;
  cmd_options opts_;
  std::map<std::string, std::string> values_;
};

#endif /* UTILS_CMD_PARSE_H_ */
