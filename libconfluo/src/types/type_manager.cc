#include "types/type_manager.h"
#include "types/primitive_types.h"

namespace confluo {

size_t type_manager::register_type(const type_properties &type_def) {
  size_t id = data_type_properties::instance().size();
  data_type_properties::instance().push_back(type_def);
  return id;
}

data_type type_manager::get_type(const std::string &type_name, size_t size) {
  return data_type(type_name, size);
}

data_type type_manager::get_type(size_t id, size_t size) {
  if (id < data_type_properties::instance().size()) {
    return data_type(static_cast<uint16_t>(id), size);
  }
  return data_type();
}

}