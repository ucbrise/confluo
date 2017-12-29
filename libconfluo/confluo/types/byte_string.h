#ifndef CONFLUO_TYPES_BYTE_STRING_H_
#define CONFLUO_TYPES_BYTE_STRING_H_

#include <cstring>
#include <cstdio>
#include <type_traits>

#include "byte_utils.h"

using namespace utils;

namespace confluo {

class byte_string;

class immutable_byte_string {
 public:
  immutable_byte_string(uint8_t* data, size_t size)
      : data_(data),
        size_(size) {
  }

  inline uint8_t operator[](size_t idx) const {
    return data_[idx];
  }

  inline bool operator<(const immutable_byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) < 0;
  }

  inline bool operator<=(const immutable_byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) <= 0;
  }

  inline bool operator>(const immutable_byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) > 0;
  }

  inline bool operator>=(const immutable_byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) >= 0;
  }

  inline bool operator==(const immutable_byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) == 0;
  }

  inline bool operator!=(const immutable_byte_string& other) const {
    return memcmp(data_, other.data_, std::min(size_, other.size_)) != 0;
  }

  immutable_byte_string& operator=(const immutable_byte_string& other) {
    data_ = other.data_;
    size_ = other.size_;
    return *this;
  }

  std::string to_string() const {
    std::string str = "{";
    size_t i;
    for (i = 0; i < size_ - 1; i++) {
      str += std::to_string((*this)[i]) + ", ";
    }
    str += std::to_string((*this)[i]) + "}";
    return str;
  }

 private:
  uint8_t* data_;
  size_t size_;

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
//  inline byte_string& operator++() {
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
//  inline byte_string& operator--() {
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

class byte_string {
 public:
  byte_string()
      : size_(0),
        data_(new uint8_t[size_]) {
  }

  byte_string(bool val)
      : size_(sizeof(bool)),
        data_(new uint8_t[size_]) {
    memcpy(data_, &val, size_);
  }

  template<typename T, typename std::enable_if<
      std::is_integral<T>::value && !std::is_same<T, bool>::value
          && std::is_signed<T>::value, T>::type* = nullptr>
  byte_string(T val)
      : size_(sizeof(T)),
        data_(new uint8_t[size_]) {
    typedef typename std::make_unsigned<T>::type UT;
    UT uval = val ^ (UT(1) << (sizeof(UT) * 8 - 1));
#if CONFLUO_ENDIANNESS == CONFLUO_BIG_ENDIAN
#elif CONFLUO_ENDIANNESS == CONFLUO_LITTLE_ENDIAN
    uval = byte_utils::byte_swap(uval);
#else
    uval = byte_utils::is_big_endian() ? uval : byte_utils::byte_swap(uval);
#endif
    memcpy(data_, &uval, size_);
  }

  template<typename T, typename std::enable_if<
      std::is_integral<T>::value && !std::is_same<T, bool>::value
          && !std::is_signed<T>::value, T>::type* = nullptr>
  byte_string(T val)
      : size_(sizeof(T)),
        data_(new uint8_t[size_]) {
#if CONFLUO_ENDIANNESS == CONFLUO_BIG_ENDIAN
#elif CONFLUO_ENDIANNESS == CONFLUO_LITTLE_ENDIAN
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

  byte_string(const std::string& str, size_t length)
      : size_(length),
        data_(new uint8_t[size_]) {
    memcpy(data_, str.c_str(), length);
  }

  byte_string(const immutable_byte_string& other)
      : size_(other.size_),
        data_(new uint8_t[size_]) {
    memcpy(data_, other.data_, size_);
  }

  byte_string(const byte_string& other)
      : size_(other.size_),
        data_(new uint8_t[size_]) {
    memcpy(data_, other.data_, size_);
  }

  byte_string(byte_string&& other)
      : size_(other.size_),
        data_(other.data_) {
    other.size_ = 0;
    other.data_ = nullptr;
  }

  ~byte_string() {
    delete[] data_;
  }

  inline uint8_t& operator[](size_t idx) {
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

  inline byte_string& operator++() {
    int64_t idx = size_ - 1;
    while (data_[idx] == UINT8_MAX) {
      data_[idx] = 0;
      idx--;
    }
    if (idx >= 0)
      data_[idx]++;
    return *this;
  }

  inline byte_string& operator--() {
    int64_t idx = size_ - 1;
    while (data_[idx] == 0) {
      data_[idx] = UINT8_MAX;
      idx--;
    }
    if (idx >= 0)
      data_[idx]--;
    return *this;
  }

  inline byte_string& operator=(const immutable_byte_string& other) {
    size_ = other.size_;
    data_ = new uint8_t[size_];
    memcpy(data_, other.data_, size_);
    return *this;
  }

  inline byte_string& operator=(const byte_string& other) {
    size_ = other.size_;
    data_ = new uint8_t[size_];
    memcpy(data_, other.data_, size_);
    return *this;
  }

  inline byte_string& operator=(byte_string&& other) {
    size_ = std::move(other.size_);
    data_ = std::move(other.data_);

    other.size_ = 0;
    other.data_ = nullptr;

    return *this;
  }

  immutable_byte_string copy() const {
    return immutable_byte_string(data_, size_);
  }

  std::string to_string() const {
    std::string str = "{";
    size_t i;
    for (i = 0; i < size_ - 1; i++) {
      str += std::to_string(data_[i]) + ", ";
    }
    str += std::to_string(data_[i]) + "}";
    return str;
  }

 private:
  size_t size_;
  uint8_t* data_;
};

}

#endif /* CONFLUO_TYPES_BYTE_STRING_H_ */
