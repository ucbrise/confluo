#ifndef SLOG_EXCEPTIONS_H_
#define SLOG_EXCEPTIONS_H_

#include <exception>

namespace slog {

class log_overflow_exception : public std::exception {
  virtual const char* what() const throw () {
    return "Log is full!";
  }
};

}

#endif /* SLOG_EXCEPTIONS_H_ */
