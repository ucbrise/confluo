#include "types/raw_data.h"

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
    delete[] reinterpret_cast<uint8_t*>(ptr);
    ptr = nullptr;
    size = 0;
  }
}

void mutable_raw_data::allocate(size_t sz) {
  ptr = new uint8_t[sz];
  size = sz;
}

}