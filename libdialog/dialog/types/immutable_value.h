#ifndef DIALOG_IMMUTABLE_VALUE_H_
#define DIALOG_IMMUTABLE_VALUE_H_

#include <cstdlib>

#include "string_utils.h"
#include "types/data_types.h"
#include "types/type_manager.h"

using namespace utils;

namespace dialog {

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
    if (type_manager::is_valid_id(type_.id) && !type_manager::is_primitive(type_.id)) {
        return type_.to_string() + "()";
    }
    switch (type_.id) {
      case type_id::D_BOOL: {
        return "bool(" + std::to_string(*reinterpret_cast<const bool*>(ptr_))
            + ")";
      }
      case type_id::D_CHAR: {
        return "char(" + std::to_string(*reinterpret_cast<const char*>(ptr_))
            + ")";
      }
      case type_id::D_SHORT: {
        return "short(" + std::to_string(*reinterpret_cast<const short*>(ptr_))
            + ")";
      }
      case type_id::D_INT: {
        return "int(" + std::to_string(*reinterpret_cast<const int*>(ptr_))
            + ")";
      }
      case type_id::D_LONG: {
        return "long(" + std::to_string(*reinterpret_cast<const long*>(ptr_))
            + ")";
      }
      case type_id::D_FLOAT: {
        return "float(" + std::to_string(*reinterpret_cast<const float*>(ptr_))
            + ")";
      }
      case type_id::D_DOUBLE: {
        return "double("
            + std::to_string(*reinterpret_cast<const double*>(ptr_)) + ")";
      }
      case type_id::D_STRING: {
        return "string("
            + immutable_byte_string(reinterpret_cast<uint8_t*>(ptr_),
                                    type_.size).to_string() + ")";
      }
      case type_id::D_NONE: {
        return "none()";
      }
      default: {
        THROW(illegal_state_exception, "Invalid type id");
        //return data_types[type_.id].to_string() + "(" + data(ptr, type_.size).as<T> + ")"; 
      }
    }
  }

 protected:
  data_type type_;
  void* ptr_;
};

}

#endif /* DIALOG_IMMUTABLE_VALUE_H_ */
