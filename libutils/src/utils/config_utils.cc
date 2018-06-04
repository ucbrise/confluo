#include "config_utils.h"

namespace utils {

std::string config_utils::read_from_env(const std::string &env_var, const std::string &default_val) {
  if (const char *env_p = std::getenv(env_var.c_str())) {
    return std::string(env_p);
  }
  return default_val;
}

configuration_map::configuration_map(const std::string &paths)
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
    LOG_WARN << "Could not find configuration file in any of "
             << string_utils::mk_string(path_vector, ", ");
  }
}

void configuration_map::remove_comment(std::string &line) const {
  std::string::size_type pos;
  if ((pos = line.find("#")) != line.npos)
    line.erase(pos);
}

bool configuration_map::is_empty(const std::string &line) const {
  return line.find_first_not_of(' ') == line.npos;
}

bool configuration_map::is_valid(const std::string &line) const {
  std::string temp = line;
  temp.erase(0, temp.find_first_not_of("\t "));
  if (temp[0] == '=')
    return false;

  for (size_t i = temp.find(':') + 1; i < temp.length(); i++)
    if (temp[i] != ' ')
      return true;

  return false;
}

std::string configuration_map::extract_key(size_t sep_pos, const std::string &line) const {
  std::string key = line.substr(0, sep_pos);
  if (key.find('\t') != line.npos || key.find(' ') != line.npos)
    key.erase(key.find_first_of("\t "));
  return key;
}

std::string configuration_map::extract_value(size_t sep_pos, const std::string &line) {
  std::string value = line.substr(sep_pos + 1);
  value.erase(0, value.find_first_not_of("\t "));
  value.erase(value.find_last_not_of("\t ") + 1);
  return value;
}

void configuration_map::extract_entry(const std::string &line) {
  std::string temp = line;
  temp.erase(0, temp.find_first_not_of("\t "));
  size_t sep_pos = temp.find(':');

  std::string key = extract_key(sep_pos, temp);
  std::string value = extract_value(sep_pos, temp);

  conf_map_.insert(std::make_pair(key, value));
}

void configuration_map::parse_line(const std::string &line, size_t line_no) {
  if (line.find(':') == line.npos)
    throw configuration_exception(
        "Could not find ':' on line#" + std::to_string(line_no) + " '" + line
            + "'");

  if (!is_valid(line))
    throw configuration_exception(
        "Bad format for line#" + std::to_string(line_no) + " '" + line + "'");

  extract_entry(line);
}

void configuration_map::extract_all(std::string &path) {
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

}