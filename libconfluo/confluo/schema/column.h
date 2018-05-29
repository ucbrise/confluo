#ifndef CONFLUO_SCHEMA_COLUMN_H_
#define CONFLUO_SCHEMA_COLUMN_H_

#include <string>
#include <cstdint>

#include "field.h"
#include "index_state.h"
#include "schema/column_snapshot.h"
#include "types/mutable_value.h"
#include "string_utils.h"

namespace confluo {

/**
 * Column class. Contains operations for operating on columns in the schema.
 */
class column_t {
 public:
  /**
   * Default constructor for a column
   */
  column_t();

  /**
   * Constructor that initializes the fields of the column
   * @param idx The index of the column
   * @param offset The offset for the column
   * @param type The type of data the column has
   * @param name The name of the column
   * @param min The min value of the column
   * @param max The max value of the column
   */
  column_t(uint16_t idx, uint16_t offset, const data_type &type,
           const std::string &name, const mutable_value &min,
           const mutable_value &max);

  /**
   * Gets name
   * @return The name of the column
   */
  std::string name() const;

  /**
   * Gets type
   * @return The type of the data the column has
   */
  const data_type &type() const;

  /**
   * Gets offset
   * @return The offset of the column data
   */
  uint16_t offset() const;

  /**
   * Gets the index
   * @return The index of the column
   */
  uint16_t idx() const;

  /**
   * Gets the min value
   * @return The minimum value the column can hold
   */
  mutable_value min() const;

  /**
   * Gets the maximum value
   * @return The maximum value the column can hold
   */
  mutable_value max() const;

  /**
   * Gets the id of the column index
   * @return The index id
   */
  uint16_t index_id() const;

  /**
   * Gets the size of the bucket at the index
   * @return The bucket size
   */
  double index_bucket_size() const;

  /**
   * Whether the column is indexed
   * @return True if the column is indexed, false otherwise
   */
  bool is_indexed() const;

  /**
   * Index the column
   * @return True if the column was successfully indexed, false otherwise
   */
  bool set_indexing();

  /**
   * Sets index
   * @param index_id The id of the index
   * @param bucket_size The size of the bucket
   */
  void set_indexed(uint16_t index_id, double bucket_size);

  /**
   * Unindexes the column
   */
  void set_unindexed();

  /**
   * Disables indexing for the column
   * @return True if disabled false otherwise
   */
  bool disable_indexing();

  /**
   * Creates column from data
   * @param data The data that the column stores
   * @return The new column
   */
  field_t apply(void *data) const;

  /**
   * Takes a snapshot of the column data
   *
   * @return A new column that is the snapshot
   */
  column_snapshot snapshot() const;

 private:
  uint16_t idx_;
  data_type type_;
  uint16_t offset_;
  std::string name_;
  mutable_value min_;
  mutable_value max_;
  index_state_t idx_state_;
};

}

#endif /* CONFLUO_SCHEMA_COLUMN_H_ */
