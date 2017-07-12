#ifndef DIALOG_BYTE_STRING_H_
#define DIALOG_BYTE_STRING_H_

#include <cstring>
#include <cstdio>

#include "byte_utils.h"
#include "byte_string.h"

using namespace utils;

namespace dialog {

class byte_string {
 public:
  byte_string()
      : size_(0),
        data_(nullptr) {
  }

  template<typename T, typename std::enable_if<
      std::is_integral<T>::value && !std::is_same<T, bool>::value
          && std::is_signed<T>::value, T>::type* = nullptr>
  byte_string(T val)
      : size_(sizeof(T)),
        data_(new uint8_t[size_]) {
    val ^= T(1) << (sizeof(T) * 8 - 1);
#if DIALOG_ENDIANNESS == DIALOG_BIG_ENDIAN
#elif DIALOG_ENDIANNESS == DIALOG_LITTLE_ENDIAN
    val = byte_utils::byte_swap(val);
#else
    val = byte_utils::is_big_endian() ? val : byte_utils::byte_swap(val);
#endif
    memcpy(data_, &val, size_);
  }

  template<typename T, typename std::enable_if<
      std::is_integral<T>::value && !std::is_same<T, bool>::value
          && !std::is_signed<T>::value, T>::type* = nullptr>
  byte_string(T val)
      : size_(sizeof(T)),
        data_(new uint8_t[size_]) {
#if DIALOG_ENDIANNESS == DIALOG_BIG_ENDIAN
#elif DIALOG_ENDIANNESS == DIALOG_LITTLE_ENDIAN
    val = byte_utils::byte_swap(val);
#else
    val = byte_utils::is_big_endian() ? val : byte_utils::byte_swap(val);
#endif
    memcpy(data_, &val, size_);
  }

  byte_string(const std::string& str)
      : size_(str.length()),
        data_(new uint8_t[size_]) {
    memcpy(data_, str.c_str(), str.length());
  }

  byte_string(const byte_string& other)
      : size_(other.size_),
        data_(new uint8_t[size_]) {
    memcpy(data_, other.data_, size_);
  }

  ~byte_string() {
    delete[] data_;
  }

  inline uint8_t operator[](size_t idx) {
    return data_[idx];
  }

  inline uint8_t operator[](size_t idx) const {
    return data_[idx];
  }

  inline bool operator<(const byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) < 0;
  }

  inline bool operator<=(const byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) <= 0;
  }

  inline bool operator>(const byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) > 0;
  }

  inline bool operator>=(const byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) >= 0;
  }

  inline bool operator==(const byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) == 0;
  }

  inline bool operator!=(const byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) != 0;
  }

 private:
  size_t size_;
  uint8_t* data_;
};

}

#endif /* LIBDIALOG_DIALOG_BYTE_STRING_H_ */
