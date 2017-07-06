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

    for (size_t i = 0; i < *num_columns_; i++) {
      auto entry = std::make_pair(std::string(columns_[i].name),
                                  columns_[i].idx);
      name_map_.insert(entry);
    }
  }

  column_t& get_column(size_t idx) {
    return columns_[idx];
  }

  size_t num_columns() {
    return *num_columns_;
  }

  column_t lookup(const std::string& name) const {
    return columns_[name_map_.at(string_utils::to_upper(name))];
  }

  record_t apply(size_t log_offset, const void* data, size_t len, uint64_t ts) {
    record_t r;
    r.timestamp = ts;
    r.data = data;
    r.len = len;
    r.log_offset = log_offset;
    r.fields.reserve(*num_columns_);
    for (uint16_t i = 0; i < *num_columns_; i++)
      r.fields.push_back(columns_[i].apply(data));
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
    column_t col;
    col.idx = schema_.size();
    col.type = type;
    col.offset = offset_;
    col.set_name(name);
    schema_.push_back(col);
    offset_ += type.size;
    return *this;
  }

  template<class storage_mode>
  void init_schema(schema_t<storage_mode>& schema, const std::string& path) {
    schema.init(path, schema_);
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
