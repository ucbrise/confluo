#ifndef MONOLOG_EXCEPTIONS_H_
#define MONOLOG_EXCEPTIONS_H_

#include <exception>

namespace monolog {

class log_overflow_exception : public std::exception {
  virtual const char* what() const throw () {
    return "Log is full!";
  }
};

}

#endif /* MONOLOG_EXCEPTIONS_H_ */
