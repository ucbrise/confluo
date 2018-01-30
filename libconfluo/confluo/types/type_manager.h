#ifndef CONFLUO_TYPES_TYPE_MANAGER_H_
#define CONFLUO_TYPES_TYPE_MANAGER_H_

#include "exceptions.h"
#include "type_properties.h"

namespace confluo {

/**
 * Manages adding types and utility functions for types
 */
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
    size_t id = find_type_properties(type_name);
    return data_type(id, DATA_TYPES[id].size ? DATA_TYPES[id].size : size);
  }

  /**
   * Get type from type id and size
   *
   * @param id Type id.
   * @param size Size; if not provided, assumes the default type size.
   * @return Wrapper around data type
   */
  static data_type get_type(size_t id, size_t size = 0) {
    if (id < DATA_TYPES.size()) {
      return data_type(id, DATA_TYPES[id].size ? DATA_TYPES[id].size : size);
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
static data_type BOOL_TYPE = type_manager::get_type("bool");
static data_type CHAR_TYPE = type_manager::get_type("char");
static data_type UCHAR_TYPE = type_manager::get_type("uchar");
static data_type SHORT_TYPE = type_manager::get_type("short");
static data_type USHORT_TYPE = type_manager::get_type("ushort");
static data_type INT_TYPE = type_manager::get_type("int");
static data_type UINT_TYPE = type_manager::get_type("uint");
static data_type LONG_TYPE = type_manager::get_type("long");
static data_type ULONG_TYPE = type_manager::get_type("ulong");
static data_type FLOAT_TYPE = type_manager::get_type("float");
static data_type DOUBLE_TYPE = type_manager::get_type("double");
static data_type STRING_TYPE(size_t size) {
  return type_manager::get_type("string", size);
}

}

#endif /* CONFLUO_TYPES_THREAD_MANAGER_H_ */
