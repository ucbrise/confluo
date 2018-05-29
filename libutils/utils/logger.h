#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include <sstream>
#include <string>
#include <stdio.h>

#include "time_utils.h"

#define SET_LOG_LEVEL(level) utils::log::logger::LOG_LEVEL = utils::log::log_level::level;
#define LOG_TRACE utils::log::logger().get(utils::log::log_level::TRACE)
#define LOG_DEBUG utils::log::logger().get(utils::log::log_level::DEBUG)
#define LOG_INFO utils::log::logger().get(utils::log::log_level::INFO)
#define LOG_WARN utils::log::logger().get(utils::log::log_level::WARN)
#define LOG_ERROR utils::log::logger().get(utils::log::log_level::ERROR)
#define LOG_FATAL utils::log::logger().get(utils::log::log_level::FATAL)

namespace utils {
namespace log {

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

class logger {
 public:
  static log_level LOG_LEVEL;

  logger();

  virtual ~logger();

  std::ostringstream& get(const log_level level);

 private:
  std::string to_string(const log_level level);

  std::ostringstream os_;
  log_level msg_level_;
};

}
}

#endif /* UTILS_LOGGER_H_ */
