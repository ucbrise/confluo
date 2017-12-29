#ifndef CONFLUO_SCHEMA_FIELD_H_
#define CONFLUO_SCHEMA_FIELD_H_

#include <cstdint>

#include "types/byte_string.h"
#include "types/data_type.h"
#include "types/immutable_value.h"

namespace confluo {

struct field_t {
  field_t(uint16_t idx, const data_type& type, void* data, bool indexed,
          uint16_t index_id, double index_bucket_size)
      : idx_(idx),
        value_(type, data),
        indexed_(indexed),
        index_bucket_size_(index_bucket_size),
        index_id_(index_id) {
  }

  inline uint16_t idx() const {
    return idx_;
  }

  inline const data_type& type() const {
    return value_.type();
  }

  inline const immutable_value& value() const {
    return value_;
  }

  inline bool is_indexed() const {
    return indexed_;
  }

  inline uint16_t index_id() const {
    return index_id_;
  }

  inline byte_string get_key() const {
    return value_.to_key(index_bucket_size_);
  }

  template<typename T>
  inline T as() const {
    return value_.as<T>();
  }

  std::string to_string() const {
    return value_.to_string();
  }

 private:
  uint16_t idx_;
  immutable_value value_;
  bool indexed_;
  double index_bucket_size_;
  uint16_t index_id_;

};

}

#endif /* CONFLUO_SCHEMA_FIELD_H_ */
