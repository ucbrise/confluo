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

};

class primitive_types {
 public:
  /** The none data type */
  static data_type &NONE_TYPE() {
    static data_type none_type(0, 0);
    return none_type;
  }

  /** The boolean data type */
  static data_type &BOOL_TYPE() {
    static data_type bool_type(primitive_type::D_BOOL, sizeof(bool));
    return bool_type;
  }

  /** The character data type */
  static data_type &CHAR_TYPE() {
    static data_type char_type(primitive_type::D_CHAR, sizeof(int8_t));
    return char_type;
  }

  /** The unsigned character data type */
  static data_type &UCHAR_TYPE() {
    static data_type uchar_type(primitive_type::D_UCHAR, sizeof(uint8_t));
    return uchar_type;
  }

  /** The short data type */
  static data_type &SHORT_TYPE() {
    static data_type short_type(primitive_type::D_SHORT, sizeof(int16_t));
    return short_type;
  }

  /** The unsigned short data type */
  static data_type &USHORT_TYPE() {
    static data_type ushort_type(primitive_type::D_USHORT, sizeof(uint16_t));
    return ushort_type;
  }

  /** The integer data type */
  static data_type &INT_TYPE() {
    static data_type int_type(primitive_type::D_INT, sizeof(int32_t));
    return int_type;
  }

  /** The unsigned integer data type */
  static data_type &UINT_TYPE() {
    static data_type unit_type(primitive_type::D_UINT, sizeof(uint32_t));
    return unit_type;
  }

  /** The long data type */
  static data_type &LONG_TYPE() {
    static data_type long_type(primitive_type::D_LONG, sizeof(int64_t));
    return long_type;
  }

  /** The unsigned long data type */
  static data_type &ULONG_TYPE() {
    static data_type ulong_type(primitive_type::D_ULONG, sizeof(uint64_t));
    return ulong_type;
  }

  /** The single precision floating point data type */
  static data_type &FLOAT_TYPE() {
    static data_type float_type(primitive_type::D_FLOAT, sizeof(float));
    return float_type;
  }

  /** The double precision floating point data type */
  static data_type &DOUBLE_TYPE() {
    static data_type double_type(primitive_type::D_DOUBLE, sizeof(double));
    return double_type;
  }

  /** The string data type */
  static data_type STRING_TYPE(size_t size) {
    return data_type(primitive_type::D_STRING, size);
  }
};

}

#endif /* CONFLUO_TYPES_THREAD_MANAGER_H_ */
