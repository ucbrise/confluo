#include "cmd_parse.h"

cmd_option::cmd_option(const std::string &lopt, char sopt, bool flag) {
  lopt_ = lopt;
  sopt_ = sopt;
  has_arg_ = flag ? no_argument : required_argument;
  desc_ = "";
  if (flag)
    default_ = "false";
  else
    default_ = "";
  required_ = false;

  option_str_ = "  -" + std::string(1, (char) sopt_) + ",--" + lopt_;
  if (has_arg_ == required_argument) {
    std::string var = lopt_;
    std::transform(var.begin(), var.end(), var.begin(), ::toupper);
    option_str_ += "=[" + var + "]";
  }
}

cmd_option &cmd_option::set_required(const bool value) {
  required_ = value;
  default_ = "";
  return *this;
}

cmd_option &cmd_option::set_default(const std::string &value) {
  default_ = value;
  required_ = false;
  return *this;
}

cmd_option &cmd_option::set_description(const std::string &value) {
  desc_ = value;
  return *this;
}

std::string cmd_option::desc_str() const {
  std::string desc = desc_;
  if (default_ != "" && has_arg_ != no_argument)
    desc += " (default: " + default_ + ")";

  if (required_)
    desc += " [REQUIRED]";

  desc += "\n";
  return desc;
}

std::string cmd_option::opt_str() const {
  return option_str_;
}

std::string cmd_option::help_line(size_t opt_width) const {
  std::stringstream stream;
  stream << std::left << std::setw(static_cast<int>(opt_width)) << opt_str() << desc_str();
  return stream.str();
}

option cmd_option::to_option() const {
  return {strdup(lopt_.c_str()), has_arg_, nullptr, sopt_};
}

cmd_options::cmd_options() {
  sopts_ = "";
}

void cmd_options::add(const cmd_option &opt) {
  copts_.push_back(opt);
  sopts_ += opt.sopt_;
  if (opt.has_arg_ == required_argument)
    sopts_ += ':';
  lopts_.push_back(opt.to_option());
  sopt_to_idx_[opt.sopt_] = copts_.size() - 1;
}

cmd_options cmd_options::finalize() {
  if (lopts_.back().name != nullptr) {
    add(cmd_option("help", 'h', true).set_description(
        "Print this help message and exit"));
    lopts_.push_back({NULL, 0, NULL, 0});
  }
  return *this;
}

cmd_parser::cmd_parser(int argc, char *const *argv, cmd_options &opts) {
  argc_ = argc;
  argv_ = argv;
  opts_ = opts.finalize();
  parse();
}

std::string cmd_parser::get(const std::string &key) const {
  return values_.at(key);
}

int cmd_parser::get_int(const std::string &key) const {
  return std::stoi(values_.at(key));
}

long cmd_parser::get_long(const std::string &key) const {
  return std::stol(values_.at(key));
}

float cmd_parser::get_float(const std::string &key) const {
  return std::stof(values_.at(key));
}

double cmd_parser::get_double(const std::string &key) const {
  return std::stod(values_.at(key));
}

bool cmd_parser::get_flag(const std::string &key) const {
  std::string val = values_.at(key);
  if (val == "true")
    return true;
  else if (val == "false")
    return false;
  throw cmd_parse_exception("could not parse flag");
}

std::string cmd_parser::help_msg() {
  std::string msg = "Usage: " + std::string(argv_[0]) + " [OPTIONS]\n\n";
  msg += "OPTIONS:\n";
  std::vector<cmd_option> copts = opts_.copts_;

  // Compute formatting widths
  size_t opt_width = 0;
  for (const cmd_option &opt : copts) {
    opt_width = std::max(opt.opt_str().length(), opt_width);
  }
  opt_width += 8;

  for (const cmd_option &opt : copts)
    msg += opt.help_line(opt_width);

  return msg;
}

std::string cmd_parser::parsed_values() {
  std::string parsed = "";
  for (auto kv : values_) {
    if (kv.first != "help") {
      parsed += kv.first + " -> \"" + kv.second + "\"; ";
    }
  }
  return parsed;
}

void cmd_parser::parse() {
  int c;
  option *lopts = &(opts_.lopts_[0]);
  const char *sopts = opts_.sopts_.c_str();
  std::vector<cmd_option> copts = opts_.copts_;

  std::vector<bool> parsed(opts_.copts_.size(), false);

  // Add default values
  for (const cmd_option &opt : copts) {
    if (opt.default_ != "")
      values_[opt.lopt_] = opt.default_;
  }

  while ((c = getopt_long(argc_, argv_, sopts, lopts, nullptr)) != -1) {
    if (c == '?')
      throw cmd_parse_exception("unknown or ambiguous option");

    if (c == ':')
      throw cmd_parse_exception("missing option argument");

    int opt_idx = static_cast<int>(opts_.sopt_to_idx_.at(static_cast<char>(c)));
    parsed[opt_idx] = true;
    std::string key = std::string(copts[opt_idx].lopt_);
    if (copts[opt_idx].has_arg_ == required_argument) {
      values_[key] = std::string(optarg);
    } else {
      values_[key] = "true";
    }
  }

  // Make sure all required values were provided
  std::string err = "";
  for (size_t i = 0; i < parsed.size(); i++)
    if (copts[i].required_ && !parsed[i])
      err += "required argument " + opts_.copts_[i].lopt_ + " not provided\n";

  if (err != "")
    throw cmd_parse_exception(err.c_str());
}
