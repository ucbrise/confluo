#ifndef DIALOG_IMMUTABLE_VALUE_H_
#define DIALOG_IMMUTABLE_VALUE_H_

#include <cstdlib>

#include "data_types.h"
#include "relational_ops.h"
#include "string_utils.h"

using namespace utils;

namespace dialog {

struct immutable_value_t {
  immutable_value_t(data_type type = NONE_TYPE)
      : type_(type),
        data_(nullptr) {
  }

  immutable_value_t(const data_type& type, void* data)
      : type_(type),
        data_(data) {
  }

  immutable_value_t(const immutable_value_t& other)
      : type_(other.type_),
        data_(other.data_) {
  }

  inline data_type const& type() const {
    return type_;
  }

  inline void const* data() const {
    return data_;
  }

  inline byte_string to_key(double bucket_size) const {
    return type_.keytransform()(data_, bucket_size);
  }

  static immutable_value_t parse(const std::string& str, data_type type) {
    switch (type.id) {
      case type_id::D_BOOL: {
        bool val = string_utils::lexical_cast<bool>(str);
        return immutable_value_t(type, valdup(val));
      }
      case type_id::D_CHAR: {
        char val = string_utils::lexical_cast<char>(str);
        return immutable_value_t(type, valdup(val));
      }
      case type_id::D_SHORT: {
        short val = string_utils::lexical_cast<short>(str);
        return immutable_value_t(type, valdup(val));
      }
      case type_id::D_INT: {
        int val = string_utils::lexical_cast<int>(str);
        return immutable_value_t(type, valdup(val));
      }
      case type_id::D_LONG: {
        long val = string_utils::lexical_cast<long>(str);
        return immutable_value_t(type, valdup(val));
      }
      case type_id::D_FLOAT: {
        float val = string_utils::lexical_cast<float>(str);
        return immutable_value_t(type, valdup(val));
      }
      case type_id::D_DOUBLE: {
        double val = string_utils::lexical_cast<double>(str);
        return immutable_value_t(type, valdup(val));
      }
      case type_id::D_STRING: {
        return immutable_value_t(type, strdup(str.c_str()));
      }
      default: {
        throw std::bad_cast();
      }
    }
  }

  // Relational operators
  static bool relop(relop_id id, const immutable_value_t& first,
                    const immutable_value_t& second) {
    if (first.type_ != second.type_)
      THROW(invalid_operation_exception,
            "Cannot compare values of different types");
    return first.type_.relop(id)(first.data_, second.data_);
  }

  friend inline bool operator <(const immutable_value_t& first,
                         const immutable_value_t& second) {
    return relop(relop_id::LT, first, second);
  }

  friend inline bool operator <=(const immutable_value_t& first,
                          const immutable_value_t& second) {
    return relop(relop_id::LE, first, second);
  }

  friend inline bool operator >(const immutable_value_t& first,
                         const immutable_value_t& second) {
    return relop(relop_id::GT, first, second);
  }

  friend inline bool operator >=(const immutable_value_t& first,
                          const immutable_value_t& second) {
    return relop(relop_id::GE, first, second);
  }

  friend inline bool operator ==(const immutable_value_t& first,
                          const immutable_value_t& second) {
    return relop(relop_id::EQ, first, second);
  }

  friend inline bool operator !=(const immutable_value_t& first,
                          const immutable_value_t& second) {
    return relop(relop_id::NEQ, first, second);
  }

  std::string to_string() const {
    switch (type_.id) {
      case type_id::D_BOOL: {
        return "bool(" + std::to_string(*reinterpret_cast<const bool*>(data_))
            + ")";
      }
      case type_id::D_CHAR: {
        return "char(" + std::to_string(*reinterpret_cast<const char*>(data_))
            + ")";
      }
      case type_id::D_SHORT: {
        return "short(" + std::to_string(*reinterpret_cast<const short*>(data_))
            + ")";
      }
      case type_id::D_INT: {
        return "int(" + std::to_string(*reinterpret_cast<const int*>(data_))
            + ")";
      }
      case type_id::D_LONG: {
        return "long(" + std::to_string(*reinterpret_cast<const long*>(data_))
            + ")";
      }
      case type_id::D_FLOAT: {
        return "float(" + std::to_string(*reinterpret_cast<const float*>(data_))
            + ")";
      }
      case type_id::D_DOUBLE: {
        return "double("
            + std::to_string(*reinterpret_cast<const double*>(data_)) + ")";
      }
      case type_id::D_STRING: {
        return "string("
            + std::string(*reinterpret_cast<const char*>(data_), type_.size)
            + ")";
      }
      case type_id::D_NONE: {
        return "none()";
      }
      default: {
        throw std::bad_cast();
      }
    }
  }

 protected:
  template<typename T>
  static inline void* valdup(const T& val) {
    void* out = malloc(sizeof(T));

    if (out != nullptr)
      memcpy(out, &val, sizeof(T));

    return out;
  }

  data_type type_;
  void* data_;
};

}

#endif /* DIALOG_IMMUTABLE_VALUE_H_ */
