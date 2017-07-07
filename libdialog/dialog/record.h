#ifndef LIBDIALOG_DIALOG_RECORD_H_
#define LIBDIALOG_DIALOG_RECORD_H_

#include <vector>
#include <cstdint>

namespace dialog {

struct record_t {
 public:
  typedef std::vector<field_t>::iterator iterator;
  typedef std::vector<field_t>::const_iterator const_iterator;

  record_t(uint64_t timestamp, size_t log_offset, const void* data, size_t len)
      : timestamp_(timestamp),
        log_offset_(log_offset),
        data_(data),
        len_(len) {
  }

  void reserve(size_t n) {
    fields_.reserve(n);
  }

  void push_back(const field_t& val) {
    fields_.push_back(val);
  }

  void push_back(field_t&& val) {
    fields_.push_back(val);
  }

  const field_t& operator[](uint16_t idx) const {
    return fields_.at(idx);
  }

  const field_t& at(uint16_t idx) const {
    return fields_.at(idx);
  }

  uint64_t timestamp() const {
    return timestamp_;
  }

  size_t log_offset() const {
    return log_offset_;
  }

  const void* data() const {
    return data_;
  }

  size_t length() const {
    return len_;
  }

  iterator begin() {
    return fields_.begin();
  }

  iterator end() {
    return fields_.end();
  }

  const_iterator begin() const {
    return fields_.begin();
  }

  const_iterator end() const {
    return fields_.end();
  }

 private:
  uint64_t timestamp_;
  size_t log_offset_;
  const void* data_;
  size_t len_;
  std::vector<field_t> fields_;
};

}

#endif /* LIBDIALOG_DIALOG_RECORD_H_ */
