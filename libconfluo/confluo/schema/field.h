#ifndef CONFLUO_SCHEMA_FIELD_H_
#define CONFLUO_SCHEMA_FIELD_H_

#include <cstdint>

#include "types/byte_string.h"
#include "types/data_type.h"
#include "types/immutable_value.h"

namespace confluo {

/**
 * Field definition
 */
struct field_t {
  /**
   * Constructs a field from the specified components
   *
   * @param idx The index of the field
   * @param type The data type of the field
   * @param data The data the field contains
   * @param indexed Whether the field is indexed
   * @param index_id The id of the index
   * @param index_bucket_size The index bucket size
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
   * Returns the index of the field
   *
   * @return The index of the field
   */
  inline uint16_t idx() const {
    return idx_;
  }

  /**
   * Returns the data type of the field
   *
   * @return The data type of the fiedl
   */
  inline const data_type& type() const {
    return value_.type();
  }

  /**
   * Gets the value of the field
   *
   * @return The immutable value of the field
   */
  inline const immutable_value& value() const {
    return value_;
  }

  /**
   * Returns whether the field is indexed
   *
   * @return True if the field is indexed, false otherwise
   */
  inline bool is_indexed() const {
    return indexed_;
  }

  /**
   * The identifier of the index
   *
   * @return The index id
   */
  inline uint16_t index_id() const {
    return index_id_;
  }

  /**
   * Gets the key of the field
   *
   * @return A byte string that is the key
   */
  inline byte_string get_key() const {
    return value_.to_key(index_bucket_size_);
  }

  /**
   * Casts the field to the specified type
   *
   * @tparam T The type to cast to
   *
   * @return The value casted as type T
   */
  template<typename T>
  inline T as() const {
    return value_.as<T>();
  }

  /**
   * Gets a string representation of the field
   *
   * @return A string containing the field data
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
