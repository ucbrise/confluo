#include "schema/field.h"

namespace confluo {

field_t::field_t(uint16_t idx,
                 const data_type &type,
                 void *data,
                 bool indexed,
                 uint16_t index_id,
                 double index_bucket_size)
    : idx_(idx),
      value_(type, data),
      indexed_(indexed),
      index_bucket_size_(index_bucket_size),
      index_id_(index_id) {
}

uint16_t field_t::idx() const {
  return idx_;
}

const data_type &field_t::type() const {
  return value_.type();
}

const immutable_value &field_t::value() const {
  return value_;
}

bool field_t::is_indexed() const {
  return indexed_;
}

uint16_t field_t::index_id() const {
  return index_id_;
}

byte_string field_t::get_key() const {
  return value_.to_key(index_bucket_size_);
}

std::string field_t::to_string() const {
  return value_.to_string();
}

}