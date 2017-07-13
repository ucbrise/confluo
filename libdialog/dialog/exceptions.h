#ifndef DIALOG_EXCEPTIONS_H_
#define DIALOG_EXCEPTIONS_H_

#include <exception>
#include <string>

#include "error_handling.h"

namespace dialog {

#define DEFINE_EXCEPTION(name)\
class name : public std::exception {\
 public:\
  name(const std::string& msg, const std::string& st)\
      : msg_(msg + "\n" + st) {\
  }\
  const char* what() const noexcept {\
    return msg_.c_str();\
  }\
 private:\
  const std::string msg_;\
};

DEFINE_EXCEPTION(parse_exception)
DEFINE_EXCEPTION(invalid_access_exception)
DEFINE_EXCEPTION(invalid_operation_exception)
DEFINE_EXCEPTION(unsupported_exception)
DEFINE_EXCEPTION(management_exception)

#define THROW(ex, msg)\
    throw ex(msg, utils::error_handling::stacktrace())

}

#endif /* DIALOG_EXCEPTIONS_H_ */
