#ifndef UTILS_IO_UTILS_H_
#define UTILS_IO_UTILS_H_

#include <iostream>
#include <string>

namespace utils {

class io_utils {
 public:
  template<typename T>
  static void write(std::ostream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof(T));
  }

  template<typename T>
  static void write(std::ostream &out, const T *values, size_t length) {
    out.write(reinterpret_cast<const char *>(values), length * sizeof(T));
  }

  template<typename T>
  static T read(std::istream &in) {
    T val;
    in.read(reinterpret_cast<char *>(&val), sizeof(T));
    return val;
  }

  static std::string read(std::istream &in, size_t length);

  static void flush(std::ostream &out);
};

template<>
void io_utils::write<std::string>(std::ostream &out, const std::string &value);

template<>
std::string io_utils::read<std::string>(std::istream &in);

}

#endif /* UTILS_IO_UTILS_H_ */
