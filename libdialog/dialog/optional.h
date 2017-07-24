#ifndef DIALOG_OPTIONAL_H_
#define DIALOG_OPTIONAL_H_

#include "exceptions.h"

namespace dialog {

template<typename T>
class optional {
 public:
  optional()
      : has_value_(false) {
  }

  optional(const T& value)
      : value_(value),
        has_value_(true) {
  }

  bool has_value() const {
    return has_value_;
  }

  T value() const {
    if (has_value_)
      return value_;
    THROW(illegal_state_exception, "optional does not have a value");
  }

  optional<T>& operator=(const optional& other) {
    has_value_ = other.has_value_;
    value_ = other.value_;
    return *this;
  }

  optional<T>& operator=(const T& value) {
    has_value_ = true;
    value_ = value;
    return *this;
  }

 private:
  bool has_value_;
  T value_;
};

}

#endif /* DIALOG_OPTIONAL_H_ */
