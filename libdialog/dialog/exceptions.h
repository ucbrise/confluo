#ifndef DIALOG_EXCEPTIONS_H_
#define DIALOG_EXCEPTIONS_H_

#include <exception>
#include <string>

namespace dialog {

#define DEFINE_EXCEPTION(name)\
class name : public std::exception {\
 public:\
  name(const std::string& msg)\
      : msg_(msg) {\
  }\
  const char* what() const noexcept {\
    return msg_.c_str();\
  }\
 private:\
  const std::string msg_;\
};

DEFINE_EXCEPTION(parse_exception)
DEFINE_EXCEPTION(invalid_operation_exception)
DEFINE_EXCEPTION(unsupported_exception)

}

#endif /* DIALOG_EXCEPTIONS_H_ */
