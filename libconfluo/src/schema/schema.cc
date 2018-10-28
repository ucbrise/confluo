#include "schema/schema.h"

namespace confluo {

schema_t::schema_t()
    : record_size_(0) {
}

schema_t::schema_t(const std::vector<column_t> &columns)
    : columns_(columns) {
  record_size_ = 0;
  for (size_t i = 0; i < columns_.size(); i++) {
    name_map_.insert(std::make_pair(columns_[i].name(), columns_[i].idx()));
    record_size_ += columns_[i].type().size;
  }
}

size_t schema_t::get_field_index(const std::string &name) const {
  return name_map_.at(string_utils::to_upper(name));
}

column_t &schema_t::operator[](size_t idx) {
  return columns_[idx];
}

column_t const &schema_t::operator[](size_t idx) const {
  return columns_[idx];
}

column_t &schema_t::operator[](const std::string &name) {
  try {
    return columns_[name_map_.at(string_utils::to_upper(name))];
  } catch (std::exception &e) {
    THROW(invalid_operation_exception,
          "No such attribute " + name + ": " + e.what());
  }
}

column_t const &schema_t::operator[](const std::string &name) const {
  try {
    return columns_[name_map_.at(string_utils::to_upper(name))];
  } catch (std::exception &e) {
    THROW(invalid_operation_exception,
          "No such attribute " + name + ": " + e.what());
  }
}

size_t schema_t::record_size() const {
  return record_size_;
}

size_t schema_t::size() const {
  return columns_.size();
}

record_t schema_t::apply(size_t offset, storage::read_only_encoded_ptr<uint8_t> &data) const {
  record_t r(offset, data, record_size_);
  r.reserve(columns_.size());
  for (uint16_t i = 0; i < columns_.size(); i++)
    r.push_back(columns_[i].apply(r.data()));
  return r;
}

record_t schema_t::apply_unsafe(size_t offset, void *data) const {
  record_t r(offset, reinterpret_cast<uint8_t *>(data), record_size_);
  r.reserve(columns_.size());
  for (uint16_t i = 0; i < columns_.size(); i++)
    r.push_back(columns_[i].apply(r.data()));
  return r;
}

schema_snapshot schema_t::snapshot() const {
  schema_snapshot snap;
  for (const column_t &col : columns_) {
    snap.add_column(col.snapshot());
  }
  return snap;
}

std::vector<column_t> &schema_t::columns() {
  return columns_;
}

std::vector<column_t> const &schema_t::columns() const {
  return columns_;
}

std::string schema_t::to_string() const {
  std::string str = "{\n";
  for (auto col : columns_) {
    str += ("\t" + col.name() + ": " + col.type().name() + ",\n");
  }
  str += "}";
  return str;
}

void *schema_t::json_string_to_data(const std::string json_record) const {
  // need to convert json_data into a record vector
  std::stringstream ss;
  // putting the json data into a stream
  ss << json_record;

  // initializing the boost property tree
  namespace pt = boost::property_tree;
  pt::ptree root;
  // parsing the json data
  pt::read_json(ss, root);

  // this will store our record
  std::vector<std::string> record;

  // iterator over the boost property tree values
  for (pt::ptree::value_type &pair : root)
  {
    // get the content of the node
    std::string value = pair.second.data();
    // put the content of the node into the record
    record.push_back(value);
  }

  void *buf = record_vector_to_data(record);
  return buf;
}

void schema_t::data_to_json_string(std::string &ret, const void *data) const {
  namespace pt = boost::property_tree;
  pt::ptree root;

  for (size_t i = 0; i < size(); i++) {
    const void *fptr = reinterpret_cast<const uint8_t *>(data) + columns_[i].offset();
    data_type ftype = columns_[i].type();
    std::string column_name = columns_[i].name();
    root.put(column_name, ftype.to_string_op()(immutable_raw_data(fptr, ftype.size)));
  }

  std::stringstream ss;
  pt::write_json(ss, root);
  ret = ss.str();
}

void *schema_t::record_vector_to_data(const std::vector<std::string> &record) const {
  if (record.size() == columns_.size()) {
    // Timestamp is provided
    void *buf = new uint8_t[record_size_]();
    for (size_t i = 0; i < columns_.size(); i++) {
      void *fptr = reinterpret_cast<uint8_t *>(buf) + columns_[i].offset();
      columns_[i].type().parse_op()(record.at(i), fptr);
    }
    return buf;
  } else if (record.size() == columns_.size() - 1) {
    // Timestamp is not provided -- generate one
    void *buf = new uint8_t[record_size_]();
    uint64_t ts = time_utils::cur_ns();
    memcpy(buf, &ts, sizeof(uint64_t));
    for (size_t i = 1; i < columns_.size(); i++) {
      void *fptr = reinterpret_cast<uint8_t *>(buf) + columns_[i].offset();
      columns_[i].type().parse_op()(record.at(i - 1), fptr);
    }
    return buf;
  } else {
    THROW(invalid_operation_exception, "Record does not match schema");
  }
}

void schema_t::record_vector_to_data(std::string &out, const std::vector<std::string> &record) const {
  if (record.size() == columns_.size()) {
    // Timestamp is provided
    out.resize(record_size_);
    for (size_t i = 0; i < columns_.size(); i++) {
      void *fptr = reinterpret_cast<uint8_t *>(&out[0]) + columns_[i].offset();
      columns_[i].type().parse_op()(record.at(i), fptr);
    }
  } else if (record.size() == columns_.size() - 1) {
    // Timestamp is not provided -- generate one
    out.resize(record_size_);
    uint64_t ts = time_utils::cur_ns();
    memcpy(&out[0], &ts, sizeof(uint64_t));
    for (size_t i = 1; i < columns_.size(); i++) {
      void *fptr = reinterpret_cast<uint8_t *>(&out[0]) + columns_[i].offset();
      columns_[i].type().parse_op()(record.at(i - 1), fptr);
    }
  } else {
    THROW(invalid_operation_exception, "Record does not match schema");
  }
}

void schema_t::data_to_record_vector(std::vector<std::string> &ret, const void *data) const {
  for (size_t i = 0; i < size(); i++) {
    const void *fptr = reinterpret_cast<const uint8_t *>(data) + columns_[i].offset();
    data_type ftype = columns_[i].type();
    ret.push_back(ftype.to_string_op()(immutable_raw_data(fptr, ftype.size)));
  }
}

schema_builder::schema_builder()
    : user_provided_ts_(false),
      offset_(0) {
  // Every schema must have timestamp
  // TODO: Replace this with a new timestamp type
  mutable_value min(primitive_types::ULONG_TYPE(), primitive_types::ULONG_TYPE().min());
  mutable_value max(primitive_types::ULONG_TYPE(), primitive_types::ULONG_TYPE().max());
  columns_.push_back(column_t(0, 0, primitive_types::ULONG_TYPE(), "TIMESTAMP", min, max));
  offset_ += primitive_types::ULONG_TYPE().size;
}

schema_builder &schema_builder::add_column(const data_type &type,
                                           const std::string &name,
                                           const mutable_value &min,
                                           const mutable_value &max) {
  if (utils::string_utils::to_upper(name) == "TIMESTAMP") {
    user_provided_ts_ = true;
    if (type != primitive_types::ULONG_TYPE()) {
      THROW(invalid_operation_exception, "TIMESTAMP must be of ULONG_TYPE");
    }
    return *this;
  }

  columns_.push_back(column_t(static_cast<uint16_t>(columns_.size()), offset_, type, name, min, max));
  offset_ += type.size;
  return *this;
}

schema_builder &schema_builder::add_column(const data_type &type, const std::string &name) {
  return add_column(type, name, mutable_value(type, type.min()), mutable_value(type, type.max()));
}

std::vector<column_t> schema_builder::get_columns() const {
  return columns_;
}

bool schema_builder::user_provided_ts() const {
  return user_provided_ts_;
}

}