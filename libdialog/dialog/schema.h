#ifndef DIALOG_SCHEMA_H_
#define DIALOG_SCHEMA_H_

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <map>

#include "schema_snapshot.h"
#include "exceptions.h"
#include "field.h"
#include "column.h"
#include "record.h"
#include "data_types.h"
#include "immutable_value.h"
#include "string_utils.h"

using namespace utils;

// FIXME: Add storage mode
// FIXME: Nullable records
// FIXME: Timestamp handling
// FIXME: Handle non-unique field names

namespace dialog {

class schema_t {
 public:
  schema_t()
      : record_size_(0) {
  }

  schema_t(const std::vector<column_t>& columns)
      : columns_(columns) {
    record_size_ = 0;
    for (size_t i = 0; i < columns_.size(); i++) {
      name_map_.insert(std::make_pair(columns_[i].name(), columns_[i].idx()));
      record_size_ += columns_[i].type().size;
    }
  }

  schema_t(const schema_t& other)
      : record_size_(other.record_size_),
        columns_(other.columns_),
        name_map_(other.name_map_) {
  }

  ~schema_t() = default;

  size_t get_field_index(const std::string& name) const {
    return name_map_.at(string_utils::to_upper(name));
  }

  column_t& operator[](size_t idx) {
    return columns_[idx];
  }

  column_t const& operator[](size_t idx) const {
    return columns_[idx];
  }

  column_t& operator[](const std::string& name) {
    try {
      return columns_[name_map_.at(string_utils::to_upper(name))];
    } catch (std::exception& e) {
      THROW(invalid_operation_exception,
            "No such attribute " + name + ": " + e.what());
    }
  }

  column_t const& operator[](const std::string& name) const {
    try {
      return columns_[name_map_.at(string_utils::to_upper(name))];
    } catch (std::exception& e) {
      THROW(invalid_operation_exception,
            "No such attribute " + name + ": " + e.what());
    }
  }

  size_t record_size() const {
    return record_size_;
  }

  size_t size() const {
    return columns_.size();
  }

  record_t apply(size_t offset, void* data) const {
    record_t r(offset, data, record_size_);
    r.reserve(columns_.size());
    for (uint16_t i = 0; i < columns_.size(); i++)
      r.push_back(columns_[i].apply(r.data()));
    return r;
  }

  schema_snapshot snapshot() const {
    schema_snapshot snap;
    for (const column_t& col : columns_) {
      snap.add_column(col.snapshot());
    }
    return snap;
  }

  std::vector<column_t>& columns() {
    return columns_;
  }

  std::vector<column_t> const& columns() const {
    return columns_;
  }

 private:
  size_t record_size_;  // TODO: Switch to dynamically sized records at some point
  std::vector<column_t> columns_;
  std::map<std::string, uint16_t> name_map_;
};

class schema_builder {
 public:
  schema_builder()
      : user_provided_ts_(false),
        offset_(0) {
    // Every schema must have timestamp
    columns_.push_back(
        column_t(0, 0, LONG_TYPE, "TIMESTAMP", LONG_TYPE.zero(),
                 LONG_TYPE.max()));
    offset_ += LONG_TYPE.size;
  }

  schema_builder& add_column(const data_type& type, const std::string& name,
                             const mutable_value& min,
                             const mutable_value& max) {
    if (utils::string_utils::to_upper(name) == "TIMESTAMP") {
      user_provided_ts_ = true;
      if (type != LONG_TYPE) {
        THROW(invalid_operation_exception, "TIMESTAMP must be of LONG_TYPE");
      }
      return *this;
    }

    columns_.push_back(
        column_t(columns_.size(), offset_, type, name, min, max));
    offset_ += type.size;
    return *this;
  }

  inline schema_builder& add_column(const data_type& type,
                                    const std::string& name) {
    if (type.id == type_id::D_STRING)
      return add_column(type, name, mutable_value(), mutable_value());
    return add_column(type, name, mutable_value(type, type.min()),
                      mutable_value(type, type.max()));
  }

  std::vector<column_t> get_columns() const {
    return columns_;
  }

  bool user_provided_ts() const {
    return user_provided_ts_;
  }

 private:
  bool user_provided_ts_;
  uint16_t offset_;
  std::vector<column_t> columns_;
};

}

#endif /* DIALOG_SCHEMA_H_ */
