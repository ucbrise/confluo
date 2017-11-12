#ifndef DIALOG_DATA_H_
#define DIALOG_DATA_H_

#include <string>

namespace dialog {

struct data {
  const void* ptr;
  size_t size;

  data(const void* _ptr, size_t _size)
      : ptr(_ptr),
        size(_size) {
  }

  template<typename T>
  inline T as() const {
    return *reinterpret_cast<const T*>(ptr);
  }
};

template<>
inline std::string data::as<std::string>() const {
  return std::string(reinterpret_cast<const char *>(ptr), size);
}

}

#endif /* DIALOG_DATA_H_ */
