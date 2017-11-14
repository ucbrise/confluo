#ifndef DIALOG_DATA_H_
#define DIALOG_DATA_H_

#include <string>

namespace dialog {

struct immutable_raw_data {
  const void* ptr;
  size_t size;

  immutable_raw_data(const void* _ptr, size_t _size)
      : ptr(_ptr),
        size(_size) {
  }

  template<typename T>
  inline T as() const {
    return *reinterpret_cast<const T*>(ptr);
  }
};

template<>
inline std::string immutable_raw_data::as<std::string>() const {
  return std::string(reinterpret_cast<const char *>(ptr), size);
}

struct mutable_raw_data {
 public:
  void* ptr;
  size_t size;

  mutable_raw_data()
      : ptr(nullptr),
        size(0) {
  }

  mutable_raw_data(size_t sz)
      : ptr(new uint8_t[sz]),
        size(sz) {
  }

  mutable_raw_data(const mutable_raw_data& other)
      : ptr(new uint8_t[other.size]),
        size(other.size) {
    memcpy(ptr, other.ptr, size);
  }

  mutable_raw_data(const immutable_raw_data& other)
      : ptr(new uint8_t[other.size]),
        size(other.size) {
    memcpy(ptr, other.ptr, size);
  }

  mutable_raw_data(mutable_raw_data&& other)
      : ptr(std::move(other.ptr)),
        size(std::move(other.size)) {
    other.ptr = nullptr;
    other.size = 0;
  }

  ~mutable_raw_data() {
    free();
  }

  template<typename T>
  inline T as() const {
    return *reinterpret_cast<const T*>(ptr);
  }

  template<typename T>
  inline mutable_raw_data& set(const T& value) {
    *reinterpret_cast<T*>(ptr) = value;
    return *this;
  }

  mutable_raw_data operator=(const mutable_raw_data& other) {
    if (size != other.size) {
      free();
      allocate(other.size);
    }
    memcpy(ptr, other.ptr, size);
    return *this;
  }

  mutable_raw_data operator=(const immutable_raw_data& other) {
    if (size != other.size) {
      free();
      allocate(other.size);
    }
    memcpy(ptr, other.ptr, size);
    return *this;
  }

  mutable_raw_data operator=(mutable_raw_data&& other) {
    free();
    ptr = other.ptr;
    size = other.size;
    other.ptr = nullptr;
    other.size = 0;
    return *this;
  }

  immutable_raw_data immutable() const {
    return immutable_raw_data(ptr, size);
  }

 private:
  void free() {
    if (ptr != nullptr) {
      delete[] reinterpret_cast<uint8_t*>(ptr);
      ptr = nullptr;
      size = 0;
    }
  }

  void allocate(size_t sz) {
    ptr = new uint8_t[sz];
    size = sz;
  }

};

template<>
inline std::string mutable_raw_data::as<std::string>() const {
  return std::string(reinterpret_cast<const char *>(ptr), size);
}

template<>
inline mutable_raw_data& mutable_raw_data::set<std::string>(const std::string& str) {
  memcpy(ptr, str.data(), size);
  return *this;
}

}

#endif /* DIALOG_DATA_H_ */
