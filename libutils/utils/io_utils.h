#ifndef UTILS_IO_UTILS_H_
#define UTILS_IO_UTILS_H_

namespace utils {

class io_utils {
 public:
  template<typename T>
  static void write(std::ostream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
  }

  template<typename T>
  static void write(std::ostream& out, const T* values, size_t length) {
    out.write(reinterpret_cast<const char*>(values), length * sizeof(T));
  }

  template<typename T>
  static T read(std::istream& in) {
    T val;
    in.read(reinterpret_cast<char*>(&val), sizeof(T));
    return val;
  }

  static void flush(std::ostream& out) {
    out.flush();
  }

};

template<>
void io_utils::write<std::string>(std::ostream& out,
                                         const std::string& value) {
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

}

#endif /* UTILS_IO_UTILS_H_ */
