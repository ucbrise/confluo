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
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include "exceptions.h"
#include "types/immutable_value.h"
#include "schema/column.h"
#include "schema/field.h"
#include "schema/record.h"
#include "schema_snapshot.h"
#include "string_utils.h"
#include "types/data_type.h"

using namespace utils;
namespace pt = boost::property_tree;

// FIXME: Add storage mode
// FIXME: Nullable records
// FIXME: Timestamp handling
// FIXME: Handle non-unique field names

namespace confluo {

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

  record_t apply(size_t offset, storage::read_only_ptr<uint8_t>& data) const {
    record_t r(offset, data, record_size_);
    r.reserve(columns_.size());
    for (uint16_t i = 0; i < columns_.size(); i++)
      r.push_back(columns_[i].apply(r.data()));
    return r;
  }

  record_t apply_unsafe(size_t offset, void* data) const {
    record_t r(offset, reinterpret_cast<uint8_t*>(data), record_size_);
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

  std::string to_string() const {
    std::string str = "{\n";
    for (auto col : columns_) {
      str += ("\t" + col.name() + ": " + col.type().name() + ",\n");
    }
    str += "}";
    return str;
  }

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

  void json_to_ptree(pt::ptree& out, const std::string& json) const {
    try {
      boost::iostreams::stream<boost::iostreams::array_source> stream(json.c_str(), json.size());
      pt::read_json(stream, out);
    } catch (pt::json_parser_error& e) {
      THROW(invalid_operation_exception, e.what());
    } catch (...) {
      THROW(invalid_operation_exception, "JSON format failed to read for some reason");
    }
  }

  void* json_to_data(const std::string& json) const {
    pt::ptree tree;
    json_to_ptree(tree, json);

    if (tree.size() == columns_.size()) {
      // Timestamp is provided
      void* buf = new uint8_t[record_size_]();
      for (auto col: columns_) {
        void* fptr = reinterpret_cast<uint8_t*>(buf) + col.offset();
        col.type().parse_op()(tree.get<std::string>(col.name()), fptr);
      }
      return buf;
    } else if (tree.size() == columns_.size() - 1) {
      // Timestamp is not provided so generate one
      void* buf = new uint8_t[record_size_]();
      uint64_t ts = time_utils::cur_ns();
      memcpy(buf, &ts, sizeof(uint64_t));
      for (size_t i = 1; i < columns_.size(); i++) {
        column_t col = columns_[i];
        void* fptr = reinterpret_cast<uint8_t*>(buf) + col.offset();
        col.type().parse_op()(tree.get<std::string>(col.name()), fptr);
      }
      return buf;
    } else {
      THROW(invalid_operation_exception, "Record does not match schema");
    }
  }

  void data_to_record_vector(std::vector<std::string>& ret,
                             const void* data) const {
    for (size_t i = 0; i < size(); i++) {
      const void* fptr = reinterpret_cast<const uint8_t*>(data)
          + columns_[i].offset();
      data_type ftype = columns_[i].type();
      ret.push_back(ftype.to_string_op()(immutable_raw_data(fptr, ftype.size)));
    }
  }

  std::vector<std::string> data_to_record_vector(const void* data) const {
    std::vector<std::string> ret;
    data_to_record_vector(ret, data);
    return ret;
  }

  void data_to_json(std::string& ret, const void* data) const {
    pt::ptree pt;
    for (size_t i = 0; i < size(); i++) {
      const void* fptr = reinterpret_cast<const uint8_t*>(data)
          + columns_[i].offset();
      data_type ftype = columns_[i].type();
      std::string val = ftype.to_string_op()(immutable_raw_data(fptr, ftype.size));
      pt.put(columns_[i].name(), val);
    }
    std::stringstream ss;
    pt::json_parser::write_json(ss, pt);
    ret = ss.str();
  }

  std::string data_to_json(const void* data) const {
    std::string ret;
    data_to_json(ret, data);
    return ret;
  }

 private:
  size_t record_size_;  // TODO: Switch to dynamically sized records at some point
  std::vector<column_t> columns_;
  std::map<std::string, uint16_t> name_map_;
}
;

class schema_builder {
 public:
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

  inline schema_builder& add_column(const data_type& type,
                                    const std::string& name) {
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

#endif /* CONFLUO_SCHEMA_SCHEMA_H_ */
