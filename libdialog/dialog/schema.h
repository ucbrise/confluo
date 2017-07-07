#ifndef DIALOG_SCHEMA_H_
#define DIALOG_SCHEMA_H_

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <map>

#include "value.h"
#include "field.h"
#include "column.h"
#include "record.h"
#include "data_types.h"
#include "string_utils.h"

using namespace utils;

namespace dialog {

template<typename storage_mode>
class schema_t {
 public:
  schema_t()
      : num_columns_(nullptr),
        columns_(nullptr) {
  }

  schema_t(const std::string& path, const std::vector<column_t>& schema_in) {
    init(path, schema_in);
  }

  schema_t(const schema_t<storage_mode>& other) {
    num_columns_ = other.num_columns_;
    columns_ = other.columns_;
    name_map_ = other.name_map_;
  }

  void init(const std::string& path, const std::vector<column_t>& schema_in) {
    size_t file_size = sizeof(uint16_t) + schema_in.size() * sizeof(column_t);
    void* data = storage_mode::allocate(path + "/schema", file_size);
    num_columns_ = (uint16_t*) data;
    columns_ = (column_t*) (num_columns_ + 1);

    *num_columns_ = schema_in.size();
    memcpy(columns_, &schema_in[0], schema_in.size() * sizeof(column_t));
    storage_mode::flush(data, file_size);

    for (size_t i = 0; i < *num_columns_; i++)
      name_map_.insert(std::make_pair(columns_[i].name(), columns_[i].idx()));
  }

  column_t& operator[](size_t idx) {
    return columns_[idx];
  }

  column_t& operator[](const std::string& name) const {
    return columns_[name_map_.at(string_utils::to_upper(name))];
  }

  size_t size() {
    return *num_columns_;
  }

  record_t apply(size_t log_offset, const void* data, size_t len, uint64_t ts) {
    record_t r(ts, log_offset, data, len);
    r.reserve(*num_columns_);
    for (uint16_t i = 0; i < *num_columns_; i++)
      r.push_back(columns_[i].apply(data));
    return r;
  }

 private:
  uint16_t* num_columns_;
  column_t* columns_;
  std::map<std::string, uint16_t> name_map_;
};

class schema_builder {
 public:
  schema_builder()
      : offset_(0) {
  }

  schema_builder& add_column(const data_type& type, const std::string& name) {
    schema_.push_back(column_t(schema_.size(), offset_, type, name));
    offset_ += type.size;
    return *this;
  }

  std::vector<column_t> get_schema() const {
    return schema_;
  }

 private:
  uint16_t offset_;
  std::vector<column_t> schema_;
};

}

#endif /* DIALOG_SCHEMA_H_ */
