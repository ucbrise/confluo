#include "types/byte_string.h"

namespace confluo {

immutable_byte_string::immutable_byte_string(uint8_t *data, size_t size)
    : data_(data),
      size_(size) {
}

uint8_t immutable_byte_string::operator[](size_t idx) const {
  return data_[idx];
}

bool immutable_byte_string::operator<(const immutable_byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) < 0;
}

bool immutable_byte_string::operator<=(const immutable_byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) <= 0;
}

bool immutable_byte_string::operator>(const immutable_byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) > 0;
}

bool immutable_byte_string::operator>=(const immutable_byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) >= 0;
}

bool immutable_byte_string::operator==(const immutable_byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) == 0;
}

bool immutable_byte_string::operator!=(const immutable_byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) != 0;
}

immutable_byte_string &immutable_byte_string::operator=(const immutable_byte_string &other) {
  data_ = other.data_;
  size_ = other.size_;
  return *this;
}

std::string immutable_byte_string::to_string() const {
  std::string str = "{";
  size_t i;
  for (i = 0; i < size_ - 1; i++) {
    str += std::to_string((*this)[i]) + ", ";
  }
  str += std::to_string((*this)[i]) + "}";
  return str;
}

byte_string::byte_string()
    : size_(0),
      data_(new uint8_t[size_]) {
}

byte_string::byte_string(bool val)
    : size_(sizeof(bool)),
      data_(new uint8_t[size_]) {
  memcpy(data_, &val, size_);
}

byte_string::byte_string(const std::string &str)
    : size_(str.length()),
      data_(new uint8_t[size_]) {
  memcpy(data_, str.c_str(), str.length());
}

byte_string::byte_string(const std::string &str, size_t length)
    : size_(length),
      data_(new uint8_t[size_]) {
  memcpy(data_, str.c_str(), length);
}

byte_string::byte_string(const immutable_byte_string &other)
    : size_(other.size_),
      data_(new uint8_t[size_]) {
  memcpy(data_, other.data_, size_);
}

byte_string::byte_string(const byte_string &other)
    : size_(other.size_),
      data_(new uint8_t[size_]) {
  memcpy(data_, other.data_, size_);
}

byte_string::byte_string(byte_string &&other)
    : size_(other.size_),
      data_(other.data_) {
  other.size_ = 0;
  other.data_ = nullptr;
}

byte_string::~byte_string() {
  if (data_ != nullptr) {
    delete[] data_;
  }
}

uint8_t &byte_string::operator[](size_t idx) {
  return data_[idx];
}

uint8_t byte_string::operator[](size_t idx) const {
  return data_[idx];
}

bool byte_string::operator<(const byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) < 0;
}

bool byte_string::operator<=(const byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) <= 0;
}

bool byte_string::operator>(const byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) > 0;
}

bool byte_string::operator>=(const byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) >= 0;
}

bool byte_string::operator==(const byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) == 0;
}

bool byte_string::operator!=(const byte_string &other) const {
  return memcmp(data_, other.data_, std::min(size_, other.size_)) != 0;
}

byte_string &byte_string::operator++() {
  int64_t idx = static_cast<int64_t>(size_ - 1);
  while (data_[idx] == UINT8_MAX) {
    data_[idx] = 0;
    idx--;
  }
  if (idx >= 0)
    data_[idx]++;
  return *this;
}

byte_string &byte_string::operator--() {
  int64_t idx = static_cast<int64_t>(size_ - 1);
  while (data_[idx] == 0) {
    data_[idx] = UINT8_MAX;
    idx--;
  }
  if (idx >= 0)
    data_[idx]--;
  return *this;
}

byte_string &byte_string::operator=(const immutable_byte_string &other) {
  size_ = other.size_;
  data_ = new uint8_t[size_];
  memcpy(data_, other.data_, size_);
  return *this;
}

byte_string &byte_string::operator=(const byte_string &other) {
  size_ = other.size_;
  data_ = new uint8_t[size_];
  memcpy(data_, other.data_, size_);
  return *this;
}

immutable_byte_string byte_string::copy() const {
  return immutable_byte_string(data_, size_);
}

uint8_t *byte_string::data() const {
  return data_;
}

size_t byte_string::size() const {
  return size_;
}

std::string byte_string::to_string() const {
  std::string str = "{";
  size_t i;
  for (i = 0; i < size_ - 1; i++) {
    str += std::to_string(data_[i]) + ", ";
  }
  str += std::to_string(data_[i]) + "}";
  return str;
}

byte_string &byte_string::operator=(byte_string &&other) {
  size_ = other.size_;
  data_ = other.data_;
  other.size_ = 0;
  other.data_ = nullptr;
  return *this;
}

}