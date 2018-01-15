#ifndef CONFLUO_SCHEMA_FIELD_H_
#define CONFLUO_SCHEMA_FIELD_H_

#include <cstdint>

#include "types/byte_string.h"
#include "types/data_type.h"
#include "types/immutable_value.h"

namespace confluo {

    /**
     * @brief Field definition
     */
struct field_t {
    /**
     * field_t
     *
     * @param idx The idx
     * @param type The type
     * @param data The data
     * @param indexed The indexed
     * @param index_id The index_id
     * @param index_bucket_size The index_bucket_size
     */
  field_t(uint16_t idx, const data_type& type, void* data, bool indexed,
          uint16_t index_id, double index_bucket_size)
      : idx_(idx),
        value_(type, data),
        indexed_(indexed),
        index_bucket_size_(index_bucket_size),
        index_id_(index_id) {
  }

  /**
   * idx
   *
   * @return uint16_t
   */
  inline uint16_t idx() const {
    return idx_;
  }

  /**
   * type
   *
   * @return data_type&
   */
  inline const data_type& type() const {
    return value_.type();
  }

  /**
   * value
   *
   * @return immutable_value&
   */
  inline const immutable_value& value() const {
    return value_;
  }

  /**
   * is_indexed
   *
   * @return bool
   */
  inline bool is_indexed() const {
    return indexed_;
  }

  /**
   * index_id
   *
   * @return uint16_t
   */
  inline uint16_t index_id() const {
    return index_id_;
  }

  /**
   * get_key
   *
   * @return byte_string
   */
  inline byte_string get_key() const {
    return value_.to_key(index_bucket_size_);
  }

  /**
   * as
   *
   * @tparam T
   *
   * @return T
   */
  template<typename T>
  inline T as() const {
    return value_.as<T>();
  }

  /**
   * to_string
   *
   * @return std::string
   */
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
