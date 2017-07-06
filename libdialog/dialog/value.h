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
  data_type type;
  void* data;

  static value_t from_string(const std::string& str, data_type type) {
    value_t ret;
    ret.type = type;
    switch (type.id) {
      case type_id::D_BOOL: {
        bool val = string_utils::lexical_cast<bool>(str);
        ret.data = valdup(val);
        break;
      }
      case type_id::D_CHAR: {
        char val = string_utils::lexical_cast<char>(str);
        ret.data = valdup(val);
        break;
      }
      case type_id::D_SHORT: {
        short val = string_utils::lexical_cast<short>(str);
        ret.data = valdup(val);
        break;
      }
      case type_id::D_INT: {
        int val = string_utils::lexical_cast<int>(str);
        ret.data = valdup(val);
        break;
      }
      case type_id::D_LONG: {
        long val = string_utils::lexical_cast<double>(str);
        ret.data = valdup(val);
        break;
      }
      case type_id::D_FLOAT: {
        float val = string_utils::lexical_cast<float>(str);
        ret.data = valdup(val);
        break;
      }
      case type_id::D_DOUBLE: {
        double val = string_utils::lexical_cast<double>(str);
        ret.data = valdup(val);
        break;
      }
      case type_id::D_STRING: {
        ret.data = strdup(str.c_str());
        break;
      }
      default: {
        throw std::bad_cast();
      }
    }
    return ret;
  }

  bool relop(const relop_id& op, const value_t& other) const {
    return type.relop(op)(data, other.data);
  }
};

}

#endif /* DIALOG_VALUE_H_ */
