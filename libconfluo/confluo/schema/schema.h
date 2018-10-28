#ifndef CONFLUO_SCHEMA_SCHEMA_H_
#define CONFLUO_SCHEMA_SCHEMA_H_

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
  schema_t();

  /**
   * Constructs a schema from the given columns
   *
   * @param columns The columns used in the schema
   */
  schema_t(const std::vector<column_t> &columns);

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
  size_t get_field_index(const std::string &name) const;

  /**
   * Gets the column at the specified index
   *
   * @param idx The index of the column to get
   *
   * @return The column at the index
   */
  column_t &operator[](size_t idx);

  /**
   * Gets an unmodifiable version of the column at the specified index
   *
   * @param idx The index to get the column at
   *
   * @return The column at the specified index
   */
  column_t const &operator[](size_t idx) const;

  /**
   * Gets the column from the column name
   *
   * @param name The name of the column
   * @throw invalid_operatoin_exception If the column does not exist
   *
   * @return The matching column
   */
  column_t &operator[](const std::string &name);

  /**
   * Gets the column matching the specified column name
   *
   * @param name The name of the column
   * @throw invalid_operation_exception If the column does not exist
   *
   * @return The column matching the name of the column passed in
   */
  column_t const &operator[](const std::string &name) const;

  /**
   * Gets the size of the record
   *
   * @return The size of the record in bytes
   */
  size_t record_size() const;

  /**
   * Gets the number of columns
   *
   * @return The number of columns in the schema
   */
  size_t size() const;

  /**
   * Applies the schema on raw data to get a record
   *
   * @param offset The offset of the record from the log
   * @param data The data the record contains
   *
   * @return Record containing the data
   */
  record_t apply(size_t offset, storage::read_only_encoded_ptr<uint8_t> &data) const;

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
  record_t apply_unsafe(size_t offset, void *data) const;

  /**
   * Gets a snapshot of the schema
   *
   * @return A snapshot of the schema
   */
  schema_snapshot snapshot() const;

  /**
   * Gets a vector of columns from the schema
   *
   * @return The columns that make up the schema
   */
  std::vector<column_t> &columns();

  /**
   * Gets a unmodifiable vector of columns from the schema
   *
   * @return A vector of columns from the schema
   */
  std::vector<column_t> const &columns() const;

  /**
   * Gets a string representation of the schema
   *
   * @return A string containing the contents of the schema
   */
  std::string to_string() const;

  /**
   * Converts the records into a pointer to the record data
   *
   * @param record The records used for conversion
   *
   * @return A pointer to the record data
   */
  void *json_string_to_data(const std::string json_record) const;

  /**
   * Converts the records into a pointer to the record data
   *
   * @param record The records used for conversion
   *
   * @return A pointer to the record data
   */
  void data_to_json_string(std::string &ret, const void *data) const;

  /**
   * Converts the pointer to record data to a json-formatted string
   *
   * @param data The data used for conversion
   *
   * @return The json-formatted string that is returned
   */
  std::string data_to_json_string(const void *data) const {
    std::string ret;
    data_to_json_string(ret, data);
    return ret;
  }

  /**
   * Converts the records into a pointer to the record data
   *
   * @param record The records used for conversion
   *
   * @return A pointer to the record data
   */
  void *record_vector_to_data(const std::vector<std::string> &record) const;

  /**
   * Converts the records into a string
   *
   * @param out The string containing the data of the records
   * @param record The records used for conversion
   */
  void record_vector_to_data(std::string &out, const std::vector<std::string> &record) const;

  /**
   * Converts the pointer to record data to a vector of records
   *
   * @param ret The vector of string records that is filled up
   * @param data The pointer to the record data
   */
  void data_to_record_vector(std::vector<std::string> &ret, const void *data) const;

  /**
   * Converts the pointer to record data to a vector of string records
   *
   * @param data The data used for conversion
   *
   * @return The vector of string records that is returned
   */
  std::vector<std::string> data_to_record_vector(const void *data) const {
    std::vector<std::string> ret;
    data_to_record_vector(ret, data);
    return ret;
  }

 private:
  size_t record_size_;  // TODO: Switch to dynamically sized records at some point
  std::vector<column_t> columns_;
  std::map<std::string, uint16_t> name_map_;
};

/**
 * A builder of the schema
 */
class schema_builder {
 public:
  /**
   * Constructs a default schema builder
   */
  schema_builder();

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
  schema_builder &add_column(const data_type &type, const std::string &name,
                             const mutable_value &min,
                             const mutable_value &max);

  /**
   * Adds a column to the schema builder using the specified attributes
   *
   * @param type The type of the column
   * @param name The name of the column
   *
   * @return This schema builder with the column added
   */
  schema_builder &add_column(const data_type &type, const std::string &name);

  /**
   * Gets the columns of this schema builder
   *
   * @return A vector of columns representing the schema
   */
  std::vector<column_t> get_columns() const;

  /**
   * Gets the timestamp
   *
   * @return True if the user provided the timsetamp, false otherwise
   */
  bool user_provided_ts() const;

 private:
  bool user_provided_ts_;
  uint16_t offset_;
  std::vector<column_t> columns_;
};

}

#endif /* CONFLUO_SCHEMA_SCHEMA_H_ */
