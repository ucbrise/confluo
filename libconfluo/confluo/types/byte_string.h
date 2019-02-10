#ifndef CONFLUO_TYPES_BYTE_STRING_H_
#define CONFLUO_TYPES_BYTE_STRING_H_

#include <cstring>
#include <cstdio>
#include <type_traits>
#include <cstdint>
#include <algorithm>
#include <string>

#include "byte_utils.h"

namespace confluo {

class byte_string;

/**
 * Unchangeable sequence of bytes
 */
class immutable_byte_string {
 public:
  /**
   * Constructs an immutable_byte_string containing the specified data
   * and size
   * @param data The sequence of bytes
   * @param size The number of bytes the data contains
   */
  immutable_byte_string(uint8_t *data, size_t size);

  /**
   * Selects a particular byte of the string
   * @param idx The index of the byte string where the desired byte is
   * located
   * @return The byte at the desired index
   */
  uint8_t operator[](size_t idx) const;

  /**
   * Performs a less than comparison of two immutable_byte_strings 
   * based on the size of the byte_strings
   * @param other A reference to the other immutable_byte_string for
   * comparison
   * @return True if this immutable_byte string is less than the other
   * immutable_byte_string, false otherwise
   */
  bool operator<(const immutable_byte_string &other) const;

  /**
   * Performs a less than or equal to comparison of two 
   * immutable_byte_strings based on size
   * @param other A reference to other immutable_byte_string for 
   * comparison
   * @return True if this immutable_byte_string is less than or equal
   * to the other immutable_byte_string, false otherwise
   */
  bool operator<=(const immutable_byte_string &other) const;

  /**
   * Performs a greater than comparison of two 
   * immutable_byte_strings based on size
   * @param other A reference to other immutable_byte_string for 
   * comparison
   * @return True if this immutable_byte_string is greater than the other 
   * immutable_byte_string, false otherwise
   */
  bool operator>(const immutable_byte_string &other) const;

  /**
   * Performs a greater than or equal to comparison of two 
   * immutable_byte_strings based on size
   * @param other A reference to other immutable_byte_string for 
   * comparison
   * @return True if this immutable_byte_string is greater than or equal
   * to the other immutable_byte_string, false otherwise
   */
  bool operator>=(const immutable_byte_string &other) const;

  /**
   * Performs an equality comparison of two 
   * immutable_byte_strings based on size
   * @param other A reference to other immutable_byte_string for 
   * comparison
   * @return True if this immutable_byte_string is equal
   * to the other immutable_byte_string, false otherwise
   */
  bool operator==(const immutable_byte_string &other) const;

  /**
   * Performs a not equal to comparison of two 
   * immutable_byte_strings based on size
   * @param other A reference to other immutable_byte_string for 
   * comparison
   * @return True if this immutable_byte_string is not equal
   * to the other immutable_byte_string, false otherwise
   */
  bool operator!=(const immutable_byte_string &other) const;

  /**
   * Assigns the contents of another immutable_byte_string to this
   * immutable_byte_string
   * @param other A reference to the other immutable_byte_string
   * @return A pointer to this immutable_byte_string
   */
  immutable_byte_string &operator=(const immutable_byte_string &other);

  /**
   * Constructs a string representation of the immutable_byte_string
   * @return A formatted string containing the contents of the 
   * immutable_byte_string
   */
  std::string to_string() const;

 private:
  uint8_t *data_;
  size_t size_;

  /** The byte string class */
  friend class byte_string;
};

//class byte_string : public std::basic_string<uint8_t> {
// public:
//  typedef std::basic_string<uint8_t> parent_t;
//
//  byte_string()
//      : std::basic_string<uint8_t>() {
//  }
//
//  byte_string(bool val)
//      : parent_t(reinterpret_cast<const uint8_t*>(&val), sizeof(bool)) {
//  }
//
//  template<typename T, typename std::enable_if<
//      std::is_integral<T>::value && !std::is_same<T, bool>::value
//          && std::is_signed<T>::value, T>::type* = nullptr>
//  byte_string(T val)
//      : parent_t() {
//    val ^= T(1) << (sizeof(T) * 8 - 1);
//#if CONFLUO_ENDIANNESS == CONFLUO_BIG_ENDIAN
//#elif CONFLUO_ENDIANNESS == CONFLUO_LITTLE_ENDIAN
//    val = byte_utils::byte_swap(val);
//#else
//    val = byte_utils::is_big_endian() ? val : byte_utils::byte_swap(val);
//#endif
//    assign(reinterpret_cast<const uint8_t*>(&val), sizeof(T));
//  }
//
//  template<typename T, typename std::enable_if<
//      std::is_integral<T>::value && !std::is_same<T, bool>::value
//          && !std::is_signed<T>::value, T>::type* = nullptr>
//  byte_string(T val)
//      : parent_t() {
//#if CONFLUO_ENDIANNESS == CONFLUO_BIG_ENDIAN
//#elif CONFLUO_ENDIANNESS == CONFLUO_LITTLE_ENDIAN
//    val = byte_utils::byte_swap(val);
//#else
//    val = byte_utils::is_big_endian() ? val : byte_utils::byte_swap(val);
//#endif
//    assign(reinterpret_cast<const uint8_t*>(&val), sizeof(T));
//  }
//
//  byte_string(const std::string& str)
//      : parent_t(reinterpret_cast<const uint8_t*>(str.c_str()), str.length()) {
//  }
//
//  byte_string(const std::string& str, size_t len)
//      : parent_t(reinterpret_cast<const uint8_t*>(str.c_str()),
//                 std::min(len, str.length())) {
//  }
//
//  byte_string(const byte_string& other)
//      : parent_t(other) {
//  }
//
//  byte_string(byte_string&& other)
//      : parent_t(std::move(other)) {
//  }
//
//  byte_string& operator++() {
//    int64_t idx = length() - 1;
//    while ((*this)[idx] == UINT8_MAX) {
//      (*this)[idx] = 0;
//      idx--;
//    }
//    if (idx >= 0)
//      (*this)[idx]++;
//    return *this;
//  }
//
//  byte_string& operator--() {
//    int64_t idx = length() - 1;
//    while ((*this)[idx] == 0) {
//      (*this)[idx] = UINT8_MAX;
//      idx--;
//    }
//    if (idx >= 0)
//      (*this)[idx]--;
//    return *this;
//  }
//
//  byte_string& operator=(const byte_string& other) {
//    parent_t::operator=(other);
//    return *this;
//  }
//
//  byte_string& operator=(byte_string&& other) {
//    parent_t::operator=(std::move(other));
//    return *this;
//  }
//
//  std::string to_string() const {
//    std::string str = "{";
//    size_t i;
//    for (i = 0; i < length() - 1; i++) {
//      str += std::to_string((*this)[i]) + ", ";
//    }
//    str += std::to_string((*this)[i]) + "}";
//    return str;
//  }
//};

/**
 * A sequence of bytes that is mutable
 */
class byte_string {
 public:
  /**
   * Constructs an empty byte_string
   */
  byte_string();

  /**
   * Constructs a one character byte_string from the provided value
   * @param val The value to be encoded by the byte_string
   */
  byte_string(bool val);

  /**
   * Constructs a byte_string from a generic type
   * @param val The representation of the bytes to convert into a
   * byte_string, assumes val is signed
   */
  template<typename T, typename std::enable_if<
      std::is_integral<T>::value && !std::is_same<T, bool>::value
          && std::is_signed<T>::value, T>::type * = nullptr>
  byte_string(T val)
      : size_(sizeof(T)),
        data_(new uint8_t[size_]) {
    typedef typename std::make_unsigned<T>::type UT;
    UT uval = val ^(UT(1) << (sizeof(UT) * 8 - 1));
#if CONFLUO_ENDIANNESS == CONFLUO_BIG_ENDIAN
#elif CONFLUO_ENDIANNESS == CONFLUO_LITTLE_ENDIAN
    uval = utils::byte_utils::byte_swap(uval);
#else
    uval = utils::byte_utils::is_big_endian() ? uval : utils::byte_utils::byte_swap(uval);
#endif
    memcpy(data_, &uval, size_);
  }

  /**
   * Constructs a byte_string from a generic type
   * @param val The representation of the bytes to convert into a 
   * byte_string, assumes val is unsigned
   */
  template<typename T, typename std::enable_if<
      std::is_integral<T>::value && !std::is_same<T, bool>::value
          && !std::is_signed<T>::value, T>::type * = nullptr>
  byte_string(T val)
      : size_(sizeof(T)),
        data_(new uint8_t[size_]) {
#if CONFLUO_ENDIANNESS == CONFLUO_BIG_ENDIAN
#elif CONFLUO_ENDIANNESS == CONFLUO_LITTLE_ENDIAN
    val = utils::byte_utils::byte_swap(val);
#else
    val = utils::byte_utils::is_big_endian() ? val : utils::byte_utils::byte_swap(val);
#endif
    memcpy(data_, &val, size_);
  }

  /**
   * Constructs a byte_string from another byte_string
   * @param str Reference to the string to copy from
   */
  byte_string(const std::string &str);

  /**
   * Constructs a byte_string from copying a certain number of characters
   * from another byte_string
   * @param str The reference string to copy from
   * @param length The number of characters to copy
   */
  byte_string(const std::string &str, size_t length);

  /**
   * Constructs a byte_string from an immutable_byte_string
   * @param other A reference to the immutable_byte_string that is
   * copied from
   */
  byte_string(const immutable_byte_string &other);

  /**
   * Constructs a byte_string from another byte_string
   * @param other A reference to the other byte_string to copy from
   */
  byte_string(const byte_string &other);

  /**
   * Initializes a given byte_string
   * @param other A double referenced byte_string to be initialized
   */
  byte_string(byte_string &&other);

  /**
   * Deallocates the data for the byte_string
   */
  ~byte_string();

  /**
   * Accesses the byte at the specified index
   * @param idx The index of the desired byte in the byte_string
   * @return A reference to the byte at the desired index
   */
  uint8_t &operator[](size_t idx);

  /**
   * Access the byte at the specified index
   * @param idx The index of the desired byte in the byte_string
   * @return The byte at the desired index
   */
  uint8_t operator[](size_t idx) const;

  /**
   * Performs a less than comparison of two byte_strings 
   * based on the size of the byte_strings
   * @param other A reference to the other byte_string for
   * comparison
   * @return True if this byte_string is less than the other
   * byte_string, false otherwise
   */
  bool operator<(const byte_string &other) const;

  /**
   * Performs a less than or equal to comparison of two byte_strings 
   * based on the size of the byte_strings
   * @param other A reference to the other byte_string for
   * comparison
   * @return True if this byte_string is less than or equal to the other
   * byte_string, false otherwise
   */
  bool operator<=(const byte_string &other) const;

  /**
   * Performs a greater than comparison of two byte_strings 
   * based on the size of the byte_strings
   * @param other A reference to the other byte_string for
   * comparison
   * @return True if this byte_string is greater than the other
   * byte_string, false otherwise
   */
  bool operator>(const byte_string &other) const;

  /**
   * Performs a greater than or equal to comparison of two byte_strings 
   * based on the size of the byte_strings
   * @param other A reference to the other byte_string for
   * comparison
   * @return True if this byte_string is greater than or equal to the other
   * byte_string, false otherwise
   */
  bool operator>=(const byte_string &other) const;

  /**
   * Performs an equality comparison of two byte_strings 
   * based on the size of the byte_strings
   * @param other A reference to the other byte_string for
   * comparison
   * @return True if this byte_string is equal to the other
   * byte_string, false otherwise
   */
  bool operator==(const byte_string &other) const;

  /**
   * Performs a not equal comparison of two byte_strings 
   * based on the size of the byte_strings
   * @param other A reference to the other byte_string for
   * comparison
   * @return True if this byte_string is not equal to the other
   * byte_string, false otherwise
   */
  bool operator!=(const byte_string &other) const;

  /**
   * Increments the value of the byte_string
   * @return A reference to the incremented byte_string
   */
  byte_string &operator++();

  /**
   * Decrements the value of the byte_string
   * @return A reference to the decremented byte_string
   */
  byte_string &operator--();

  /**
   * Assigns the value of an immutable_byte_string to a byte_string
   * @param other A constant reference to an immutable_byte_string used
   * for assignment
   * @return A byte_string with the copied data
   */
  byte_string &operator=(const immutable_byte_string &other);

  /**
   * Assigns the contents of another byte_string to this byte_string
   * @param other Reference to the byte_string to copy from
   * @return The byte_string with the copied data
   */
  byte_string &operator=(const byte_string &other);

  /**
   * Assigns a double referenced byte_string to this byte_string
   * efficiently
   * @param other The double referenced byte_string to use
   * @return A byte string reference with the data of the other
   * byte_string
   */
  byte_string &operator=(byte_string &&other);

  template<typename T>
  inline T as() const {
    T val;
#if CONFLUO_ENDIANNESS == CONFLUO_BIG_ENDIAN
    val = *reinterpret_cast<T*>(data_);
#elif CONFLUO_ENDIANNESS == CONFLUO_LITTLE_ENDIAN
    val = utils::byte_utils::reverse_as<T>(data_, size_);
#else
    if (utils::byte_utils::is_big_endian()) {
      val = *reinterpret_cast<T*>(data_);
    } else {
      val = utils::byte_utils::reverse_as<T>(data_, size_);
    }
#endif
    return val;
  }

  /**
   * Copies the byte_string data into an immutable form
   * @return An immutable_byte_string with the same data
   */
  immutable_byte_string copy() const;

  uint8_t *data() const;

  size_t size() const;

  /**
   * Formats the data into a readable form
   * @return A string representation of the data
   */
  std::string to_string() const;

 private:
  size_t size_;
  uint8_t *data_;
};

}

#endif /* CONFLUO_TYPES_BYTE_STRING_H_ */
