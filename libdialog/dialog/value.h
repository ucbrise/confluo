#ifndef DIALOG_VALUE_H_
#define DIALOG_VALUE_H_

#include <cstdlib>

#include "data_types.h"
#include "relational_ops.h"
#include "string_utils.h"

using namespace utils;

namespace dialog {

template<typename T>
void* valdup(const T& val) {
  void* out = malloc(sizeof(T));

  if (out != nullptr)
    memcpy(out, &val, sizeof(T));

  return out;
}

struct value_t {
  value_t()
      : type_(NONE_TYPE),
        data_(nullptr) {
  }

  value_t(const data_type& type, const void* data)
      : type_(type),
        data_(data) {
  }

  value_t(const value_t& other)
      : type_(other.type_),
        data_(other.data_) {
  }

  inline const data_type& type() const {
    return type_;
  }

  inline const void* data() const {
    return data_;
  }

  static value_t from_string(const std::string& str, data_type type) {
    switch (type.id) {
      case type_id::D_BOOL: {
        bool val = string_utils::lexical_cast<bool>(str);
        return value_t(type, valdup(val));
      }
      case type_id::D_CHAR: {
        char val = string_utils::lexical_cast<char>(str);
        return value_t(type, valdup(val));
      }
      case type_id::D_SHORT: {
        short val = string_utils::lexical_cast<short>(str);
        return value_t(type, valdup(val));
      }
      case type_id::D_INT: {
        int val = string_utils::lexical_cast<int>(str);
        return value_t(type, valdup(val));
      }
      case type_id::D_LONG: {
        long val = string_utils::lexical_cast<long>(str);
        return value_t(type, valdup(val));
      }
      case type_id::D_FLOAT: {
        float val = string_utils::lexical_cast<float>(str);
        return value_t(type, valdup(val));
      }
      case type_id::D_DOUBLE: {
        double val = string_utils::lexical_cast<double>(str);
        return value_t(type, valdup(val));
      }
      case type_id::D_STRING: {
        return value_t(type, strdup(str.c_str()));
      }
      default: {
        throw std::bad_cast();
      }
    }
  }

  inline bool relop(const relop_id& op, const value_t& other) const {
    return type_.relop(op)(data_, other.data_);
  }

 private:
  data_type type_;
  const void* data_;
};

}

#endif /* DIALOG_VALUE_H_ */
