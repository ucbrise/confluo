#include "types/type_manager.h"

namespace confluo {

/** The none data type */
data_type NONE_TYPE = data_type(0, 0);
/** The boolean data type */
data_type BOOL_TYPE = type_manager::get_type("bool");
/** The character data type */
data_type CHAR_TYPE = type_manager::get_type("char");
/** The unsigned character data type */
data_type UCHAR_TYPE = type_manager::get_type("uchar");
/** The short data type */
data_type SHORT_TYPE = type_manager::get_type("short");
/** The unsigned short data type */
data_type USHORT_TYPE = type_manager::get_type("ushort");
/** The integer data type */
data_type INT_TYPE = type_manager::get_type("int");
/** The unsigned integer data type */
data_type UINT_TYPE = type_manager::get_type("uint");
/** The long data type */
data_type LONG_TYPE = type_manager::get_type("long");
/** The unsigned long data type */
data_type ULONG_TYPE = type_manager::get_type("ulong");
/** The single precision floating point data type */
data_type FLOAT_TYPE = type_manager::get_type("float");
/** The double precision floating point data type */
data_type DOUBLE_TYPE = type_manager::get_type("double");

data_type STRING_TYPE(size_t size) {
  return type_manager::get_type("string", size);
}

size_t type_manager::register_type(const type_properties &type_def) {
  size_t id = DATA_TYPES.size();
  DATA_TYPES.push_back(type_def);
  return id;
}

data_type type_manager::get_type(const std::string &type_name, size_t size) {
  return data_type(type_name, size);
}

data_type type_manager::get_type(size_t id, size_t size) {
  if (id < DATA_TYPES.size()) {
    return data_type(static_cast<uint16_t>(id), size);
  }
  return data_type();
}

bool type_manager::is_valid_id(size_t id) {
  return id >= 1 && id < DATA_TYPES.size();
}

bool type_manager::is_primitive(size_t id) {
  return id >= 1 && id <= 12;
}

}