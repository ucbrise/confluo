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
  immutable_raw_data(const void* _ptr, size_t _size);

  /**
   * Casts the data to the specified type T
   *
   * @tparam T The desired type
   *
   * @return The data of type T
   */
  template<typename T>
  T as() const {
    return *reinterpret_cast<const T*>(ptr);
  }
};

/**
 * Converts the immutable raw data to a string
 *
 * @return The data in string form
 */
template<>
std::string immutable_raw_data::as<std::string>() const {
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
  mutable_raw_data();

  /**
   * Initializes mutable raw data of a given size
   *
   * @param sz The size of the mutable raw data
   */
  mutable_raw_data(size_t sz);

  /**
   * Constructs mutable raw data from another mutable raw data
   *
   * @param other The other mutable raw data to copy from
   */
  mutable_raw_data(const mutable_raw_data& other);

  /**
   * Constructs mutable raw data from an immutable raw data
   *
   * @param other The immutable raw data to copy from
   */
  mutable_raw_data(const immutable_raw_data& other);

  /**
   * Initializes the other mutable raw data to be empty
   *
   * @param other Double reference to a mutable raw data to initialize
   */
  mutable_raw_data(mutable_raw_data&& other);

  /**
   * Deallocates the mutable raw data
   */
  ~mutable_raw_data();

  /**
   * Converts mutable raw data of to a value of type T
   *
   * @tparam T The specified type of conversion
   *
   * @return The data of type T
   */
  template<typename T>
  T as() const {
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
  mutable_raw_data& set(const T& value) {
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
  mutable_raw_data operator=(const mutable_raw_data& other);

  /**
   * Assigns raw immutable data to this raw mutable data
   *
   * @param other The immutable raw data to copy from
   *
   * @return This raw mutable data that's a copy of the raw immutable
   * data
   */
  mutable_raw_data operator=(const immutable_raw_data& other);

  /**
   * Initializes the other raw mutable data to be empty and assigns it
   * to this raw mutable data
   *
   * @param other The other raw mutable data to initialize
   *
   * @return This raw mutable data that's intialized to empty data
   */
  mutable_raw_data operator=(mutable_raw_data&& other);

  /**
   * Converts to raw immutable data
   *
   * @return The raw immutable data form of the raw mutable data
   */
  immutable_raw_data immutable() const;

 private:
  void free();

  void allocate(size_t sz);

};

/**
 * Converts the raw mutable data to be in string form
 *
 * @return String representation of the raw mutable data
 */
template<>
std::string mutable_raw_data::as<std::string>() const {
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
mutable_raw_data& mutable_raw_data::set<std::string>(const std::string& str) {
  memcpy(ptr, str.data(), str.length());
  return *this;
}

}

#endif /* CONFLUO_TYPES_RAW_DATA_H_ */
