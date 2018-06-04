#include "types/raw_data.h"

#include <cstdio>
#include <cstring>

namespace confluo {

immutable_raw_data::immutable_raw_data(const void *_ptr, size_t _size)
    : ptr(_ptr),
      size(_size) {
}

mutable_raw_data::mutable_raw_data()
    : ptr(nullptr),
      size(0) {
}

mutable_raw_data::mutable_raw_data(size_t sz)
    : ptr(sz ? new uint8_t[sz]() : nullptr),
      size(sz) {
}

mutable_raw_data::mutable_raw_data(const mutable_raw_data &other)
    : ptr(new uint8_t[other.size]),
      size(other.size) {
  memcpy(ptr, other.ptr, size);
}

mutable_raw_data::mutable_raw_data(const immutable_raw_data &other)
    : ptr(new uint8_t[other.size]),
      size(other.size) {
  memcpy(ptr, other.ptr, size);
}

mutable_raw_data::mutable_raw_data(mutable_raw_data &&other)
    : ptr(std::move(other.ptr)),
      size(std::move(other.size)) {
  other.ptr = nullptr;
  other.size = 0;
}

mutable_raw_data::~mutable_raw_data() {
  free();
}

mutable_raw_data mutable_raw_data::operator=(const mutable_raw_data &other) {
  if (size != other.size) {
    free();
    allocate(other.size);
  }
  memcpy(ptr, other.ptr, size);
  return *this;
}

mutable_raw_data mutable_raw_data::operator=(const immutable_raw_data &other) {
  if (size != other.size) {
    free();
    allocate(other.size);
  }
  memcpy(ptr, other.ptr, size);
  return *this;
}

mutable_raw_data mutable_raw_data::operator=(mutable_raw_data &&other) {
  free();
  ptr = other.ptr;
  size = other.size;
  other.ptr = nullptr;
  other.size = 0;
  return *this;
}

immutable_raw_data mutable_raw_data::immutable() const {
  return immutable_raw_data(ptr, size);
}

void mutable_raw_data::free() {
  if (ptr != nullptr) {
    delete[] reinterpret_cast<uint8_t *>(ptr);
    ptr = nullptr;
    size = 0;
  }
}

void mutable_raw_data::allocate(size_t sz) {
  ptr = new uint8_t[sz];
  size = sz;
}

/**
 * Converts the immutable raw data to a string
 *
 * @return The data in string form
 */
template<>
std::string immutable_raw_data::as<std::string>() const {
  const char *buf = reinterpret_cast<const char *>(ptr);
  return std::string(buf, size);
}

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
mutable_raw_data &mutable_raw_data::set<std::string>(const std::string &str) {
  memcpy(ptr, str.data(), str.length());
  return *this;
}

}