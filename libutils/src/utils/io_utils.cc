#include "io_utils.h"

namespace utils {

template<>
void io_utils::write<std::string>(std::ostream& out, const std::string& value) {
  size_t size = value.length();
  out.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
  out.write(value.c_str(), value.length());
}

template<>
std::string io_utils::read<std::string>(std::istream& in) {
  size_t size;
  in.read(reinterpret_cast<char*>(&size), sizeof(size_t));
  std::string value;
  value.resize(size);
  in.read(&value[0], size);
  return value;
}

std::string io_utils::read(std::istream &in, size_t length) {
  std::string value;
  value.resize(length);
  in.read(&value[0], length);
  return value;
}

void io_utils::flush(std::ostream &out) {
  out.flush();
}

}