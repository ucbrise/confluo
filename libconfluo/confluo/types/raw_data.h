#ifndef CONFLUO_TYPES_RAW_DATA_H_
#define CONFLUO_TYPES_RAW_DATA_H_

#include <string>

namespace confluo {

/**
 * Raw data that cannot be changed
 */
struct immutable_raw_data {
  /** Pointer to data that is unmodifiable */
  const void* ptr;
  /** Size of the data in bytes */
  size_t size;

  /**
   * Constructs immutable raw data from a specified pointer and size
   *
   * @param _ptr The pointer to the unmodifiable data
   * @param _size The size of the data in bytes
   */
  immutable_raw_data(const void* _ptr, size_t _size)
      : ptr(_ptr),
        size(_size) {
  }

  /**
   * Casts the data to the specified type T
   *
   * @tparam T The desired type
   *
   * @return The data of type T
   */
  template<typename T>
  inline T as() const {
    return *reinterpret_cast<const T*>(ptr);
  }
};

/**
 * Converts the immutable raw data to a string
 *
 * @return The data in string form
 */
template<>
inline std::string immutable_raw_data::as<std::string>() const {
  const char* buf = reinterpret_cast<const char *>(ptr);
  return std::string(buf, size);
}

/**
 * Raw data that is modifiable
 */
struct mutable_raw_data {
 public:
  /**
   * The pointer to the raw data
   */
  void* ptr;
  /**
   * The size of the data in bytes
   */
  size_t size;

  /**
   * Initializes empty mutable raw data
   */
  mutable_raw_data()
      : ptr(nullptr),
        size(0) {
  }

  /**
   * Initializes mutable raw data of a given size
   *
   * @param sz The size of the mutable raw data
   */
  mutable_raw_data(size_t sz)
      : ptr(sz ? new uint8_t[sz]() : nullptr),
        size(sz) {
  }

  /**
   * Constructs mutable raw data from another mutable raw data
   *
   * @param other The other mutable raw data to copy from
   */
  mutable_raw_data(const mutable_raw_data& other)
      : ptr(new uint8_t[other.size]),
        size(other.size) {
    memcpy(ptr, other.ptr, size);
  }

  /**
   * Constructs mutable raw data from an immutable raw data
   *
   * @param other The immutable raw data to copy from
   */
  mutable_raw_data(const immutable_raw_data& other)
      : ptr(new uint8_t[other.size]),
        size(other.size) {
    memcpy(ptr, other.ptr, size);
  }

  /**
   * Initializes the other mutable raw data to be empty
   *
   * @param other Double reference to a mutable raw data to initialize
   */
  mutable_raw_data(mutable_raw_data&& other)
      : ptr(std::move(other.ptr)),
        size(std::move(other.size)) {
    other.ptr = nullptr;
    other.size = 0;
  }

  /**
   * Deallocates the mutable raw data
   */
  ~mutable_raw_data() {
    free();
  }

  /**
   * Converts mutable raw data of to a value of type T
   *
   * @tparam T The specified type of conversion
   *
   * @return The data of type T
   */
  template<typename T>
  inline T as() const {
    return *reinterpret_cast<const T*>(ptr);
  }

  /**
   * Sets the mutable raw data to a value of type T
   *
   * @tparam T The type of specified value
   * @param value The value to set the mutable raw data to
   *
   * @return A reference to this raw mutable data
   */
  template<typename T>
  inline mutable_raw_data& set(const T& value) {
    *reinterpret_cast<T*>(ptr) = value;
    return *this;
  }

  /**
   * Assigns another raw mutable data to this raw mutable data
   *
   * @param other The other raw mutable data
   *
   * @return This raw mutable data that is a copy of the other raw mutable
   * data
   */
  mutable_raw_data operator=(const mutable_raw_data& other) {
    if (size != other.size) {
      free();
      allocate(other.size);
    }
    memcpy(ptr, other.ptr, size);
    return *this;
  }

  /**
   * Assigns raw immutable data to this raw mutable data
   *
   * @param other The immutable raw data to copy from
   *
   * @return This raw mutable data that's a copy of the raw immutable
   * data
   */
  mutable_raw_data operator=(const immutable_raw_data& other) {
    if (size != other.size) {
      free();
      allocate(other.size);
    }
    memcpy(ptr, other.ptr, size);
    return *this;
  }

  /**
   * Initializes the other raw mutable data to be empty and assigns it
   * to this raw mutable data
   *
   * @param other The other raw mutable data to initialize
   *
   * @return This raw mutable data that's intialized to empty data
   */
  mutable_raw_data operator=(mutable_raw_data&& other) {
    free();
    ptr = other.ptr;
    size = other.size;
    other.ptr = nullptr;
    other.size = 0;
    return *this;
  }

  /**
   * Converts to raw immutable data
   *
   * @return The raw immutable data form of the raw mutable data
   */
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

/**
 * Converts the raw mutable data to be in string form
 *
 * @return String representation of the raw mutable data
 */
template<>
inline std::string mutable_raw_data::as<std::string>() const {
  return std::string(reinterpret_cast<const char *>(ptr), size);
}

/**
 * Sets the data of the raw mutable value to a specified string value
 *
 * @param str The string data to turn into a raw mutable value
 *
 * @return A reference to the mutable raw data that stores the string value
 */
template<>
inline mutable_raw_data& mutable_raw_data::set<std::string>(const std::string& str) {
  memcpy(ptr, str.data(), str.length());
  return *this;
}

}

#endif /* CONFLUO_TYPES_RAW_DATA_H_ */
