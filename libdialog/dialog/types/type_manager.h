#ifndef DIALOG_TYPE_MANAGER_H_
#define DIALOG_TYPE_MANAGER_H_

#include "type_properties.h"
#include "exceptions.h"
#include "atomic.h"

namespace dialog {

namespace detail {
static std::vector<data_type> register_primitives() {
  return {data_type(0, 0), data_type(1, sizeof(bool)),
    data_type(2, sizeof(int8_t)), data_type(3, sizeof(int16_t)),
    data_type(4, sizeof(int32_t)), data_type(5, sizeof(int64_t)),
    data_type(6, sizeof(float)), data_type(7, sizeof(double)),
    data_type(8, 10000)};
}
}
static std::vector<data_type> data_types = detail::register_primitives();

class type_manager {
 public:
  /**
   * Registers a type to the manager
   */
  static size_t register_type(type_properties type_def) {
    size_t id = data_types.size();
    data_types.push_back(data_type(id, type_def.size));

    MIN.push_back(type_def.min);
    MAX.push_back(type_def.max);
    ONE.push_back(type_def.one);
    ZERO.push_back(type_def.zero);

    RELOPS.push_back(type_def.rel_ops);
    UNOPS.push_back(type_def.un_ops);
    BINOPS.push_back(type_def.binary_ops);
    KEYOPS.push_back(type_def.key_ops);
    TO_STRINGS.push_back(type_def.name);

    PARSERS.push_back(type_def.parse_op);
    SERIALIZERS.push_back(type_def.serialize_op);
    DESERIALIZERS.push_back(type_def.deserialize_op);
    return id;
  }

  static size_t get_id_from_type_name(std::string type_name) {
    for (unsigned int i = 0; i < data_types.size(); i++) {
      if (type_name.compare(data_types[i].to_string()) == 0) {
        return i;
      }
    }
    return -1;
  }

  static data_type get_type(const std::string& type_name, size_t size = 0) {
    for (unsigned int i = 0; i < data_types.size(); i++) {
      if (type_name.compare(data_types[i].to_string()) == 0) {
        return data_types[i];
      }
    }
    return data_type(0, 0);
  }

  static bool is_valid_id(uint16_t other_id) {
    return other_id >= 0 && other_id < data_types.size();
  }

  static bool is_primitive(uint16_t other_id) {
    return other_id >= 0 && other_id <= 8;
  }
};

static data_type NONE_TYPE = data_type(0, 0);
static data_type BOOL_TYPE = data_type(1, sizeof(bool));
static data_type CHAR_TYPE = data_type(2, sizeof(int8_t));
static data_type SHORT_TYPE = data_type(3, sizeof(int16_t));
static data_type INT_TYPE = data_type(4, sizeof(int32_t));
static data_type LONG_TYPE = data_type(5, sizeof(int64_t));
static data_type FLOAT_TYPE = data_type(6, sizeof(float));
static data_type DOUBLE_TYPE = data_type(7, sizeof(double));
static data_type STRING_TYPE(size_t size) {
  return data_type(8, size);
}

}

#endif /* DIALOG_THREAD_MANAGER_H_ */
