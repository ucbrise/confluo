#include "schema/record.h"

namespace confluo {

record_t::record_t()
    : timestamp_(0),
      log_offset_(0),
      data_(nullptr),
      ptr_(),
      size_(0),
      version_(0) {
}

record_t::record_t(size_t log_offset, uint8_t *data, size_t size)
    : timestamp_(*reinterpret_cast<int64_t *>(data)),
      log_offset_(log_offset),
      data_(data),
      ptr_(),
      size_(size),
      version_(log_offset + size) {
}

record_t::record_t(size_t log_offset, storage::read_only_encoded_ptr<uint8_t> data, size_t size)
    : timestamp_(0),
      log_offset_(log_offset),
      data_(nullptr),
      ptr_(data),
      size_(size),
      version_(log_offset + size) {
  storage::decoded_ptr<uint8_t> ptr = data.decode();
  timestamp_ = *reinterpret_cast<int64_t *>(ptr.get());
  data_ = ptr.get();
}

void record_t::reserve(size_t n) {
  fields_.reserve(n);
}

void record_t::push_back(const field_t &val) {
  fields_.push_back(val);
}

void record_t::push_back(field_t &&val) {
  fields_.push_back(val);
}

const field_t &record_t::operator[](uint16_t idx) const {
  return fields_.at(idx);
}

const field_t &record_t::at(uint16_t idx) const {
  return fields_.at(idx);
}

uint64_t record_t::timestamp() const {
  return timestamp_;
}

size_t record_t::log_offset() const {
  return log_offset_;
}

uint8_t *record_t::data() const {
  return data_;
}

uint64_t record_t::version() const {
  return version_;
}

record_t::iterator record_t::begin() {
  return fields_.begin();
}

record_t::iterator record_t::end() {
  return fields_.end();
}

record_t::const_iterator record_t::begin() const {
  return fields_.begin();
}

record_t::const_iterator record_t::end() const {
  return fields_.end();
}

size_t record_t::length() const {
  return size_;
}

std::string record_t::to_string() const {
  std::string str = "(";
  for (auto &f : *this) {
    str += f.to_string() + ", ";
  }
  if (str != "(") {
    str.pop_back();
  }
  str += ")";
  return str;
}

bool record_t::operator==(const record_t &other) const {
  return log_offset_ == other.log_offset_;
}

}