#ifndef CONFLUO_SCHEMA_FIELD_H_
#define CONFLUO_SCHEMA_FIELD_H_

#include <cstdint>

#include "types/byte_string.h"
#include "types/data_type.h"
#include "types/immutable_value.h"

namespace confluo {

/**
 * Field class. Contains operations for operating on the data in the
 * schema.
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
  field_t(uint16_t idx, const data_type& type, void* data, bool indexed, uint16_t index_id, double index_bucket_size);

  /**
   * Returns the index of the field
   *
   * @return The index of the field
   */
  uint16_t idx() const;

  /**
   * Returns the data type of the field
   *
   * @return The data type of the fiedl
   */
  const data_type& type() const;

  /**
   * Gets the value of the field
   *
   * @return The immutable value of the field
   */
  const immutable_value& value() const;

  /**
   * Returns whether the field is indexed
   *
   * @return True if the field is indexed, false otherwise
   */
  bool is_indexed() const;

  /**
   * The identifier of the index
   *
   * @return The index id
   */
  uint16_t index_id() const;

  /**
   * Gets the key of the field
   *
   * @return A byte string that is the key
   */
  byte_string get_key() const;

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
  std::string to_string() const;

 private:
  uint16_t idx_;
  immutable_value value_;
  bool indexed_;
  double index_bucket_size_;
  uint16_t index_id_;

};

}

#endif /* CONFLUO_SCHEMA_FIELD_H_ */
