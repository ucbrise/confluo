#ifndef DIALOG_SCHEMA_H_
#define DIALOG_SCHEMA_H_

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <map>

#include "field.h"
#include "column.h"
#include "record.h"
#include "data_types.h"
#include "immutable_value.h"
#include "string_utils.h"

using namespace utils;

namespace dialog {

template<typename storage_mode>
class schema_t {
 public:
  schema_t()
      : record_size_(0) {
  }

  schema_t(const std::string& path, const std::vector<column_t>& columns)
      : columns_(columns) {
    record_size_ = 0;
    for (size_t i = 0; i < columns_.size(); i++) {
      name_map_.insert(std::make_pair(columns_[i].name(), columns_[i].idx()));
      record_size_ += sizeof(uint64_t) + columns_[i].type().size;
    }
  }

  schema_t(const schema_t<storage_mode>& other)
      : columns_(other.columns_),
        name_map_(other.name_map_),
        record_size_(other.record_size_) {
  }

  ~schema_t() = default;

  column_t& operator[](size_t idx) {
    return columns_[idx];
  }

  column_t const& operator[](size_t idx) const {
    return columns_[idx];
  }

  column_t& operator[](const std::string& name) {
    return columns_[name_map_.at(string_utils::to_upper(name))];
  }

  column_t const& operator[](const std::string& name) const {
    return columns_[name_map_.at(string_utils::to_upper(name))];
  }

  size_t record_size() const {
    return record_size_;
  }

  size_t size() const {
    return columns_.size();
  }

  record_t apply(size_t offset, void* data, uint64_t size) const {
    record_t r(offset, data, size);
    r.reserve(columns_.size());
    for (uint16_t i = 0; i < columns_.size(); i++)
      r.push_back(columns_[i].apply(r.data()));
    return r;
  }

 private:
  size_t record_size_;  // TODO: Switch to dynamically sized records at some point
  std::vector<column_t> columns_;
  std::map<std::string, uint16_t> name_map_;
};

class schema_builder {
 public:
  schema_builder()
      : offset_(0) {
  }

  schema_builder& add_column(const data_type& type, const std::string& name,
                             const mutable_value_t& min,
                             const mutable_value_t& max) {
    columns_.push_back(
        column_t(columns_.size(), offset_, type, name, min, max));
    offset_ += type.size;
    return *this;
  }

  inline schema_builder& add_column(const data_type& type,
                                    const std::string& name) {
    if (type.id == type_id::D_STRING)
      return add_column(type, name, mutable_value_t(), mutable_value_t());
    return add_column(type, name, mutable_value_t(type, type.min()),
                      mutable_value_t(type, type.max()));
  }

  std::vector<column_t> get_columns() const {
    return columns_;
  }

 private:
  uint16_t offset_;
  std::vector<column_t> columns_;
};

}

#endif /* DIALOG_SCHEMA_H_ */
