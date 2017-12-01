#ifndef CONFLUO_SCHEMA_SCHEMA_H_
#define CONFLUO_SCHEMA_SCHEMA_H_

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <map>

#include "exceptions.h"
#include "types/immutable_value.h"
#include "schema/column.h"
#include "schema/field.h"
#include "schema/record.h"
#include "schema_snapshot.h"
#include "string_utils.h"
#include "types/data_type.h"

using namespace utils;

// FIXME: Add storage mode
// FIXME: Nullable records
// FIXME: Timestamp handling
// FIXME: Handle non-unique field names

namespace confluo {

/**
 * The schema for the atomic multilog. Contains operations applied to
 * data that it contains.
 */
class schema_t {
 public:
  /**
   * Constructs an empty schema
   */
  schema_t()
      : record_size_(0) {
  }

  /**
   * Constructs a schema from the given columns
   *
   * @param columns The columns used in the schema
   */
  schema_t(const std::vector<column_t>& columns)
      : columns_(columns) {
    record_size_ = 0;
    for (size_t i = 0; i < columns_.size(); i++) {
      name_map_.insert(std::make_pair(columns_[i].name(), columns_[i].idx()));
      record_size_ += columns_[i].type().size;
    }
  }

  /**
   * Default destructor for the schema
   */
  ~schema_t() = default;

  /**
   * Gets the index of the field from the given field name
   *
   * @param name The name of the field
   *
   * @return The index of the field
   */
  size_t get_field_index(const std::string& name) const {
    return name_map_.at(string_utils::to_upper(name));
  }

  /**
   * Gets the column at the specified index
   *
   * @param idx The index of the column to get
   *
   * @return The column at the index
   */
  column_t& operator[](size_t idx) {
    return columns_[idx];
  }

  /**
   * Gets an unmodifiable version of the column at the specified index
   *
   * @param idx The index to get the column at
   *
   * @return The column at the specified index
   */
  column_t const& operator[](size_t idx) const {
    return columns_[idx];
  }

  /**
   * Gets the column from the column name
   *
   * @param name The name of the column
   * @throw invalid_operatoin_exception If the column does not exist
   *
   * @return The matching column
   */
  column_t& operator[](const std::string& name) {
    try {
      return columns_[name_map_.at(string_utils::to_upper(name))];
    } catch (std::exception& e) {
      THROW(invalid_operation_exception,
            "No such attribute " + name + ": " + e.what());
    }
  }

  /**
   * Gets the column matching the specified column name
   *
   * @param name The name of the column
   * @throw invalid_operation_exception If the column does not exist
   *
   * @return The column matching the name of the column passed in
   */
  column_t const& operator[](const std::string& name) const {
    try {
      return columns_[name_map_.at(string_utils::to_upper(name))];
    } catch (std::exception& e) {
      THROW(invalid_operation_exception,
            "No such attribute " + name + ": " + e.what());
    }
  }

  /**
   * Gets the size of the record
   *
   * @return The size of the record in bytes
   */
  size_t record_size() const {
    return record_size_;
  }

  /**
   * Gets the number of columns
   *
   * @return The number of columns in the schema
   */
  size_t size() const {
    return columns_.size();
  }

  /**
   * Applies the schema on raw data to get a record
   *
   * @param offset The offset of the record from the log
   * @param data The data the record contains
   *
   * @return Record containing the data
   */
  record_t apply(size_t offset, storage::read_only_encoded_ptr<uint8_t>& data) const {
    record_t r(offset, data, record_size_);
    r.reserve(columns_.size());
    for (uint16_t i = 0; i < columns_.size(); i++)
      r.push_back(columns_[i].apply(r.data()));
    return r;
  }

  /**
   * Applies the schema on raw data to get a record.
   * Note that usage of the record relies on the lifetime of
   * the data buffer.
   *
   * @param offset The offset of the data
   * @param data The data to be added to the schema
   *
   * @return Record containing the data
   */
  record_t apply_unsafe(size_t offset, void* data) const {
    record_t r(offset, reinterpret_cast<uint8_t*>(data), record_size_);
    r.reserve(columns_.size());
    for (uint16_t i = 0; i < columns_.size(); i++)
      r.push_back(columns_[i].apply(r.data()));
    return r;
  }

  /**
   * Gets a snapshot of the schema
   *
   * @return A snapshot of the schema
   */
  schema_snapshot snapshot() const {
    schema_snapshot snap;
    for (const column_t& col : columns_) {
      snap.add_column(col.snapshot());
    }
    return snap;
  }

  /**
   * Gets a vector of columns from the schema
   *
   * @return The columns that make up the schema
   */
  std::vector<column_t>& columns() {
    return columns_;
  }

  /**
   * Gets a unmodifiable vector of columns from the schema
   *
   * @return A vector of columns from the schema
   */
  std::vector<column_t> const& columns() const {
    return columns_;
  }

  /**
   * Gets a string representation of the schema
   *
   * @return A string containing the contents of the schema
   */
  std::string to_string() const {
    std::string str = "{\n";
    for (auto col : columns_) {
      str += ("\t" + col.name() + ": " + col.type().name() + ",\n");
    }
    str += "}";
    return str;
  }

  /**
   * Converts the records into a pointer to the record data
   *
   * @param record The records used for conversion
   *
   * @return A pointer to the record data
   */
  void* record_vector_to_data(const std::vector<std::string>& record) const {
    if (record.size() == columns_.size()) {
      // Timestamp is provided
      void* buf = new uint8_t[record_size_]();
      for (size_t i = 0; i < columns_.size(); i++) {
        void* fptr = reinterpret_cast<uint8_t*>(buf) + columns_[i].offset();
        columns_[i].type().parse_op()(record.at(i), fptr);
      }
      return buf;
    } else if (record.size() == columns_.size() - 1) {
      // Timestamp is not provided -- generate one
      void* buf = new uint8_t[record_size_]();
      uint64_t ts = time_utils::cur_ns();
      memcpy(buf, &ts, sizeof(uint64_t));
      for (size_t i = 1; i < columns_.size(); i++) {
        void* fptr = reinterpret_cast<uint8_t*>(buf) + columns_[i].offset();
        columns_[i].type().parse_op()(record.at(i - 1), fptr);
      }
      return buf;
    } else {
      THROW(invalid_operation_exception, "Record does not match schema");
    }
  }

  /**
   * Converts the records into a string
   *
   * @param out The string containing the data of the records
   * @param record The records used for conversion
   */
  void record_vector_to_data(std::string& out,
                             const std::vector<std::string>& record) const {
    if (record.size() == columns_.size()) {
      // Timestamp is provided
      out.resize(record_size_);
      for (size_t i = 0; i < columns_.size(); i++) {
        void* fptr = reinterpret_cast<uint8_t*>(&out[0]) + columns_[i].offset();
        columns_[i].type().parse_op()(record.at(i), fptr);
      }
    } else if (record.size() == columns_.size() - 1) {
      // Timestamp is not provided -- generate one
      out.resize(record_size_);
      uint64_t ts = time_utils::cur_ns();
      memcpy(&out[0], &ts, sizeof(uint64_t));
      for (size_t i = 1; i < columns_.size(); i++) {
        void* fptr = reinterpret_cast<uint8_t*>(&out[0]) + columns_[i].offset();
        columns_[i].type().parse_op()(record.at(i - 1), fptr);
      }
    } else {
      THROW(invalid_operation_exception, "Record does not match schema");
    }
  }

  /**
   * Converts the pointer to record data to a vector of records
   *
   * @param ret The vector of string records that is filled up
   * @param data The pointer to the record data
   */
  void data_to_record_vector(std::vector<std::string>& ret,
                             const void* data) const {
    for (size_t i = 0; i < size(); i++) {
      const void* fptr = reinterpret_cast<const uint8_t*>(data)
          + columns_[i].offset();
      data_type ftype = columns_[i].type();
      ret.push_back(ftype.to_string_op()(immutable_raw_data(fptr, ftype.size)));
    }
  }

  /**
   * Converts the pointer to record data to a vector of string records
   *
   * @param data The data used for conversion
   *
   * @return The vector of string records that is returned
   */
  std::vector<std::string> data_to_record_vector(const void* data) const {
    std::vector<std::string> ret;
    data_to_record_vector(ret, data);
    return ret;
  }

 private:
  size_t record_size_;  // TODO: Switch to dynamically sized records at some point
  std::vector<column_t> columns_;
  std::map<std::string, uint16_t> name_map_;
}
;

/**
 * A builder of the schema
 */
class schema_builder {
 public:
  /**
   * Constructs a default schema builder
   */
  schema_builder()
      : user_provided_ts_(false),
        offset_(0) {
    // Every schema must have timestamp
    // TODO: Replace this with a new timestamp type
    mutable_value min(ULONG_TYPE, ULONG_TYPE.min());
    mutable_value max(ULONG_TYPE, ULONG_TYPE.max());
    columns_.push_back(column_t(0, 0, ULONG_TYPE, "TIMESTAMP", min, max));
    offset_ += ULONG_TYPE.size;
  }

  /**
   * Adds a column to the schema builder
   *
   * @param type The type of the column
   * @param name The name of the column
   * @param min The minimum value of the column
   * @param max The maximum value of the column
   *
   * @return A reference to this schema builder
   */
  schema_builder& add_column(const data_type& type, const std::string& name,
                             const mutable_value& min,
                             const mutable_value& max) {
    if (utils::string_utils::to_upper(name) == "TIMESTAMP") {
      user_provided_ts_ = true;
      if (type != ULONG_TYPE) {
        THROW(invalid_operation_exception, "TIMESTAMP must be of ULONG_TYPE");
      }
      return *this;
    }

    columns_.push_back(
        column_t(columns_.size(), offset_, type, name, min, max));
    offset_ += type.size;
    return *this;
  }

  /**
   * Adds a column to the schema builder using the specified attributes
   *
   * @param type The type of the column
   * @param name The name of the column
   *
   * @return This schema builder with the column added
   */
  inline schema_builder& add_column(const data_type& type,
                                    const std::string& name) {
    return add_column(type, name, mutable_value(type, type.min()),
                      mutable_value(type, type.max()));
  }

  /**
   * Gets the columns of this schema builder
   *
   * @return A vector of columns representing the schema
   */
  std::vector<column_t> get_columns() const {
    return columns_;
  }

  /**
   * Gets the timestamp
   *
   * @return True if the user provided the timsetamp, false otherwise
   */
  bool user_provided_ts() const {
    return user_provided_ts_;
  }

 private:
  bool user_provided_ts_;
  uint16_t offset_;
  std::vector<column_t> columns_;
};

}

#endif /* CONFLUO_SCHEMA_SCHEMA_H_ */
