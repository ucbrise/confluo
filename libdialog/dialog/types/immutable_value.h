#ifndef DIALOG_IMMUTABLE_VALUE_H_
#define DIALOG_IMMUTABLE_VALUE_H_

#include <cstdlib>

#include "string_utils.h"
#include "types/data_types.h"
#include "types/type_manager.h"

using namespace utils;

namespace confluo {

class immutable_value {
 public:
  immutable_value(const data_type& type = NONE_TYPE)
      : type_(type),
        ptr_(nullptr) {
  }

  immutable_value(const data_type& type, void* data)
      : type_(type),
        ptr_(data) {
  }

  inline data_type const& type() const {
    return type_;
  }

  inline void const* ptr() const {
    return ptr_;
  }

  inline immutable_raw_data to_data() const {
    return immutable_raw_data(ptr_, type_.size);
  }

  inline byte_string to_key(double bucket_size) const {
    return type_.key_transform()(to_data(), bucket_size);
  }

  // Relational operators
  static bool relop(reational_op_id id, const immutable_value& first,
                    const immutable_value& second) {
    if (first.type_ != second.type_)
      THROW(invalid_operation_exception, "Comparing values of different types");
    return first.type_.relop(id)(first.to_data(), second.to_data());
  }

  friend inline bool operator <(const immutable_value& first,
                                const immutable_value& second) {
    return relop(reational_op_id::LT, first, second);
  }

  friend inline bool operator <=(const immutable_value& first,
                                 const immutable_value& second) {
    return relop(reational_op_id::LE, first, second);
  }

  friend inline bool operator >(const immutable_value& first,
                                const immutable_value& second) {
    return relop(reational_op_id::GT, first, second);
  }

  friend inline bool operator >=(const immutable_value& first,
                                 const immutable_value& second) {
    return relop(reational_op_id::GE, first, second);
  }

  friend inline bool operator ==(const immutable_value& first,
                                 const immutable_value& second) {
    return relop(reational_op_id::EQ, first, second);
  }

  friend inline bool operator !=(const immutable_value& first,
                                 const immutable_value& second) {
    return relop(reational_op_id::NEQ, first, second);
  }

  template<typename T>
  T& as() {
    return *reinterpret_cast<T*>(ptr_);
  }

  template<typename T>
  const T& as() const {
    return *reinterpret_cast<const T*>(ptr_);
  }

  std::string to_string() const {
    return type_.name() + "(" + type_.to_string_op()(to_data()) + ")";
  }

 protected:
  data_type type_;
  void* ptr_;
};

}

#endif /* DIALOG_IMMUTABLE_VALUE_H_ */
