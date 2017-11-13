#ifndef DIALOG_TYPE_MANAGER_H_
#define DIALOG_TYPE_MANAGER_H_

#include "type_properties.h"
#include "exceptions.h"

namespace dialog {

class type_manager {
 public:
  /**
   * Registers a type to the manager
   *
   * @param type_def Properties for data type
   * @return Type id.
   */
  static size_t register_type(const type_properties& type_def) {
    size_t id = DATA_TYPES.size();
    DATA_TYPES.push_back(type_def);
    return id;
  }

  /**
   * Get type from type name and size
   *
   * @param type_name Type name.
   * @param size Size; if not provided, assumes the default type size.
   * @return Wrapper around data type
   */
  static data_type get_type(const std::string& type_name, size_t size = 0) {
    for (unsigned int i = 0; i < DATA_TYPES.size(); i++) {
      if (type_name.compare(DATA_TYPES[i].name) == 0) {
        return data_type(i, size ? size : DATA_TYPES[i].size);
      }
    }
    return data_type();
  }

  /**
   * Get type from type id and size
   *
   * @param id Type id.
   * @param size Size; if not provided, assumes the default type size.
   * @return Wrapper around data type
   */
  static data_type get_type(size_t id, size_t size = 0) {
    if (is_valid_id(id)) {
      return data_type(id, size ? size : DATA_TYPES[id].size);
    }
    return data_type();
  }

  /**
   * Checks if type is valid
   *
   * @param id Id to check.
   * @return True if id is valid, false otherwise
   */
  static bool is_valid_id(size_t id) {
    return id >= 1 && id < DATA_TYPES.size();
  }

  /**
   * Checks if type is primitive
   *
   * @param id Id to check.
   * @return True if id is primitive, false otherwise
   */
  static bool is_primitive(size_t id) {
    return id >= 1 && id <= 8;
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
