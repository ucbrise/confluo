#ifndef CONFLUO_EXCEPTIONS_H_
#define CONFLUO_EXCEPTIONS_H_

#include <exception>
#include <string>

#include "error_handling.h"

namespace confluo {

#define DEFINE_EXCEPTION(name)\
class name : public std::exception {\
 public:\
  name(const std::string& msg)\
      : msg_(msg) {\
  }\
  name()\
      : msg_("") {\
  }\
  const char* what() const noexcept {\
    return msg_.c_str();\
  }\
  name& operator=(const name& other) {\
    msg_ = other.msg_;\
    return *this;\
  }\
 private:\
  std::string msg_;\
};

DEFINE_EXCEPTION(archival_exception)
DEFINE_EXCEPTION(parse_exception)
DEFINE_EXCEPTION(invalid_access_exception)
DEFINE_EXCEPTION(invalid_cast_exception)
DEFINE_EXCEPTION(invalid_operation_exception)
DEFINE_EXCEPTION(illegal_state_exception)
DEFINE_EXCEPTION(memory_exception)
DEFINE_EXCEPTION(unsupported_exception)
DEFINE_EXCEPTION(management_exception)

#define THROW(ex, msg)\
    throw ex(msg)

}

#endif /* CONFLUO_EXCEPTIONS_H_ */
