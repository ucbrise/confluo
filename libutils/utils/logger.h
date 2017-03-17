#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include <sstream>
#include <string>
#include <stdio.h>

#include "utils.h"

#define SET_LOG_LEVEL(level) utils::logging::log::LOG_LEVEL = utils::logging::log_level::level;
#define LOG_TRACE utils::logging::log().get(utils::logging::log_level::TRACE)
#define LOG_DEBUG utils::logging::log().get(utils::logging::log_level::DEBUG)
#define LOG_INFO utils::logging::log().get(utils::logging::log_level::INFO)
#define LOG_WARN utils::logging::log().get(utils::logging::log_level::WARN)
#define LOG_ERROR utils::logging::log().get(utils::logging::log_level::ERROR)
#define LOG_FATAL utils::logging::log().get(utils::logging::log_level::FATAL)

namespace utils {
namespace logging {

enum log_level {
  ALL = 0,
  TRACE = 1,
  DEBUG = 2,
  INFO = 3,
  WARN = 4,
  ERROR = 5,
  FATAL = 6,
  OFF = 7
};

class log {
 public:
  static log_level LOG_LEVEL;

  log() {
    msg_level_ = log_level::INFO;
  }

  virtual ~log() {
    os_ << std::endl;
    fprintf(stderr, "%s", os_.str().c_str());
    fflush(stderr);
  }

  std::ostringstream& get(const log_level level) {
    msg_level_ = level;
    os_ << time_utils::current_date_time();
    os_ << " " << to_string(level) << ": ";
    return os_;
  }

 private:
  std::string to_string(const log_level level) {
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
    return "";
  }

  std::ostringstream os_;
  log_level msg_level_;
};
}
}

#endif /* UTILS_LOGGER_H_ */
