#include "logger.h"

namespace utils {
namespace log {

log_level logger::LOG_LEVEL = log_level::INFO;

logger::logger() {
  msg_level_ = log_level::INFO;
}

logger::~logger() {
  os_ << std::endl;
  if (msg_level_ >= LOG_LEVEL) {
    fprintf(stderr, "%s", os_.str().c_str());
    fflush(stderr);
  }
}

std::ostringstream &logger::get(const log_level level) {
  msg_level_ = level;
  os_ << time_utils::current_date_time();
  os_ << " " << to_string(level) << ": ";
  return os_;
}

std::string logger::to_string(const log_level level) {
  switch (level) {
    case log_level::TRACE:
      return "TRACE";
    case log_level::DEBUG:
      return "DEBUG";
    case log_level::INFO:
      return "INFO";
    case log_level::WARN:
      return "WARN";
    case log_level::ERROR:
      return "ERROR";
    case log_level::FATAL:
      return "FATAL";
    default:
      return "";
  }
}

}
}