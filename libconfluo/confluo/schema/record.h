#ifndef CONFLUO_SCHEMA_RECORD_H_
#define CONFLUO_SCHEMA_RECORD_H_

#include <vector>
#include <cstdint>

#include "../storage/ptr.h"

namespace confluo {

/**
 * The record type. Contains functionality to iterate and modify data
 * stored in the record
 */
struct record_t {
 public:
  typedef std::vector<field_t>::iterator iterator;
  typedef std::vector<field_t>::const_iterator const_iterator;

  /**
   * Constructs an empty record
   */
  record_t()
      : timestamp_(0),
        log_offset_(0),
        data_(nullptr),
        ptr_(),
        size_(0),
        version_(0) {
  }

  /**
   * Constructor using a raw pointer
   * @param log_offset log offset
   * @param data raw pointer to data
   * @param size size of data
   */
  record_t(size_t log_offset, uint8_t* data, size_t size)
      : timestamp_(*reinterpret_cast<int64_t*>(data)),
        log_offset_(log_offset),
        data_(data),
        ptr_(),
        size_(size),
        version_(log_offset + size) {
  }

  /**
   * Constructor using a raw pointer
   * @param log_offset log offset
   * @param data raw pointer to data
   * @param size size of data
   */
  record_t(size_t log_offset, storage::read_only_ptr<uint8_t> data, size_t size)
      : timestamp_(*reinterpret_cast<int64_t*>(data.decode_ptr().get())),
        log_offset_(log_offset),
        data_(data.decode_ptr().get()), // TODO since unique_ptr, can't just use rvalue
        ptr_(data),
        size_(size),
        version_(log_offset + size) {
  }

  /**
   * Reserves n bytes for the record
   *
   * @param n The number of bytes to reserve
   */
  void reserve(size_t n) {
    fields_.reserve(n);
  }

  /**
   * Adds a value to the record
   *
   * @param val The value to add
   */
  void push_back(const field_t& val) {
    fields_.push_back(val);
  }

  /**
   * Adds an r value reference to the record
   *
   * @param val The r value reference to add
   */
  void push_back(field_t&& val) {
    fields_.push_back(val);
  }

  /**
   * Gets the field at a particular index
   *
   * @param idx The index of the desired field
   *
   * @return The field at that index
   */
  const field_t& operator[](uint16_t idx) const {
    return fields_.at(idx);
  }

  /**
   * Gets the field at a particular index
   *
   * @param idx The index of the desired field
   *
   * @return The field at that index
   */
  const field_t& at(uint16_t idx) const {
    return fields_.at(idx);
  }

  /**
   * Gets the timestamp of this record
   *
   * @return The timestamp of this record
   */
  uint64_t timestamp() const {
    return timestamp_;
  }

  /**
   * Gets the offset from the log for this record
   *
   * @return The log offset
   */
  size_t log_offset() const {
    return log_offset_;
  }

  /**
   * Gets the data that this record holds
   *
   * @return The data that's held in this record
   */
  uint8_t* data() const {
    return data_;
  }

  /**
   * The version of the log the record is in
   *
   * @return The log version
   */
  uint64_t version() const {
    return version_;
  }

  /**
   * Gets an iterator for the record fields
   *
   * @return The beginning of the iterator
   */
  iterator begin() {
    return fields_.begin();
  }

  /**
   * Gets an iterator for the record fields
   *
   * @return The end of the iterator
   */
  iterator end() {
    return fields_.end();
  }

  /**
   * Gets a constant iterator for the record fields
   *
   * @return The beginning of the constant iterator
   */
  const_iterator begin() const {
    return fields_.begin();
  }

  /**
   * Gets the constant iterator for the record fields
   *
   * @return The end of the constant iterator
   */
  const_iterator end() const {
    return fields_.end();
  }

  /**
   * Gets the size of the record
   *
   * @return The size of the record in bytes
   */
  size_t length() const {
    return size_;
  }

  /**
   * Gets a string representation of the record
   *
   * @return A string containing the contents of the record
   */
  std::string to_string() const {
    std::string str = "(";
    for (auto& f : *this) {
      str += f.to_string() + ", ";
    }
    str.pop_back();
    str += ")";
    return str;
  }

  /**
   * Checks whether this record is equal to the other record
   *
   * @param other The other record used for equality comparison
   *
   * @return True if this record is equal to the other record, false
   * otherwise
   */
  bool operator==(const record_t& other) const {
    return log_offset_ == other.log_offset_;
  }

 private:
  int64_t timestamp_;
  size_t log_offset_;
  uint8_t* data_;
  storage::read_only_ptr<uint8_t> ptr_;
  size_t size_;
  uint64_t version_;
  std::vector<field_t> fields_;
};

}

namespace std {

template<>
/**
 * Record hash
 */
struct hash<confluo::record_t> {
  /**
   * Computes a hash on the record
   * @param k The record to hash
   * @return The hash of the record, which in this case is just the
   * log offset
   */
  size_t operator()(const confluo::record_t& k) const {
    return k.log_offset();
  }
};

}

#endif /* CONFLUO_SCHEMA_RECORD_H_ */
