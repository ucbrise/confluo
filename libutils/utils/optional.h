#ifndef UTILS_OPTIONAL_H_
#define UTILS_OPTIONAL_H_

namespace utils {

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
    throw -1;
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

#endif /* UTILS_OPTIONAL_H_ */
