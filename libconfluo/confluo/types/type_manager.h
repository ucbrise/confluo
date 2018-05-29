#ifndef CONFLUO_TYPES_TYPE_MANAGER_H_
#define CONFLUO_TYPES_TYPE_MANAGER_H_

#include "exceptions.h"
#include "type_properties.h"
#include "data_type.h"

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
  static size_t register_type(const type_properties &type_def);

  /**
   * Get type from type name and size
   *
   * @param type_name Type name.
   * @param size Size; if not provided, assumes the default type size.
   * @return Wrapper around data type
   */
  static data_type get_type(const std::string &type_name, size_t size = 0);

  /**
   * Get type from type id and size
   *
   * @param id Type id.
   * @param size Size; if not provided, assumes the default type size.
   * @return Wrapper around data type
   */
  static data_type get_type(size_t id, size_t size = 0);

  /**
   * Checks if type is valid
   *
   * @param id Id to check.
   * @return True if id is valid, false otherwise
   */
  static bool is_valid_id(size_t id);

  /**
   * Checks if type is primitive
   *
   * @param id Id to check.
   * @return True if id is primitive, false otherwise
   */
  static bool is_primitive(size_t id);
};

/** The none data type */
extern data_type NONE_TYPE;
/** The boolean data type */
extern data_type BOOL_TYPE;
/** The character data type */
extern data_type CHAR_TYPE;
/** The unsigned character data type */
extern data_type UCHAR_TYPE;
/** The short data type */
extern data_type SHORT_TYPE;
/** The unsigned short data type */
extern data_type USHORT_TYPE;
/** The integer data type */
extern data_type INT_TYPE;
/** The unsigned integer data type */
extern data_type UINT_TYPE;
/** The long data type */
extern data_type LONG_TYPE;
/** The unsigned long data type */
extern data_type ULONG_TYPE;
/** The single precision floating point data type */
extern data_type FLOAT_TYPE;
/** The double precision floating point data type */
extern data_type DOUBLE_TYPE;
/** The string data type */
data_type STRING_TYPE(size_t size);

}

#endif /* CONFLUO_TYPES_THREAD_MANAGER_H_ */
