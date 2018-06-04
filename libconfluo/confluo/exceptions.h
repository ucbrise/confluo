#ifndef CONFLUO_EXCEPTIONS_H_
#define CONFLUO_EXCEPTIONS_H_

#include <exception>
#include <string>

#include "error_handling.h"

namespace confluo {

/**
 * Defines an exception for a given name
 */
#define DEFINE_EXCEPTION(name)\
class name : public std::exception {\
 public:\
  explicit name(std::string msg)\
      : msg_(std::move(msg)) {\
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

/** Defines exception for archival */
DEFINE_EXCEPTION(archival_exception)
/** Defines exception when parsing fails */
DEFINE_EXCEPTION(parse_exception)
/** Defines exception for an invalid access */
DEFINE_EXCEPTION(invalid_access_exception)
/** Defines exception for an invalid cast */
DEFINE_EXCEPTION(invalid_cast_exception)
/** Defines exception for an invalid operation */
DEFINE_EXCEPTION(invalid_operation_exception)
/** Defines an exception for an illegal state */
DEFINE_EXCEPTION(illegal_state_exception)
/** Defines exception for a memory error */
DEFINE_EXCEPTION(memory_exception)
/** Defines exception for an unsupported operation */
DEFINE_EXCEPTION(unsupported_exception)
/** Defines exception for a management error */
DEFINE_EXCEPTION(management_exception)

#define THROW(ex, msg)\
    throw ex(msg)

}

#endif /* CONFLUO_EXCEPTIONS_H_ */
