#ifndef CONFLUO_SCHEMA_SCHEMA_SNAPSHOT_H_
#define CONFLUO_SCHEMA_SCHEMA_SNAPSHOT_H_

#include "schema/column_snapshot.h"
#include "types/byte_string.h"
#include "types/immutable_value.h"
#include "types/raw_data.h"

namespace confluo {

    /**
     * Snapshot of the schema
     */
class schema_snapshot {
 public:
     /**
      * schema_snapshot
      */
  schema_snapshot() = default;

  /**
   * add_column
   *
   * @param snap The snap
   */
  void add_column(const column_snapshot& snap) {
    snapshot_.push_back(snap);
  }

  /**
   * add_column
   *
   * @param snap The snap
   */
  void add_column(column_snapshot&& snap) {
    snapshot_.push_back(std::move(snap));
  }

  /**
   * get
   *
   * @param data The data
   * @param i The i
   *
   * @return immutable_value
   */
  immutable_value get(void* data, uint32_t i) const {
    return immutable_value(
        snapshot_[i].type,
        reinterpret_cast<uint8_t*>(data) + snapshot_[i].offset);
  }

  /**
   * time_key
   *
   * @param time_block The time_block
   *
   * @return byte_string
   */
  byte_string time_key(int64_t time_block) const {
    return LONG_TYPE.key_transform()(
        immutable_raw_data(reinterpret_cast<uint8_t*>(&time_block), LONG_TYPE.size), 1.0);
  }

  /**
   * get_key
   *
   * @param ptr The ptr
   * @param i The i
   *
   * @return byte_string
   */
  byte_string get_key(void* ptr, uint32_t i) const {
    return snapshot_[i].type.key_transform()(
        immutable_raw_data(reinterpret_cast<uint8_t*>(ptr) + snapshot_[i].offset,
             snapshot_[i].type.size),
        snapshot_[i].index_bucket_size);
  }

  /**
   * get_timestamp
   *
   * @param ptr The ptr
   *
   * @return int64_t
   */
  int64_t get_timestamp(void* ptr) const {
    return *reinterpret_cast<int64_t*>(ptr);
  }

  /**
   * is_indexed
   *
   * @param i The i
   *
   * @return bool
   */
  bool is_indexed(size_t i) const {
    return snapshot_[i].indexed;
  }

  /**
   * index_id
   *
   * @param i The i
   *
   * @return uint32_t
   */
  uint32_t index_id(size_t i) const {
    return snapshot_[i].index_id;
  }

  /**
   * index_bucket_size
   *
   * @param i The i
   *
   * @return double
   */
  double index_bucket_size(size_t i) const {
    return snapshot_[i].index_bucket_size;
  }

  /**
   * num_columns
   *
   * @return size_t
   */
  size_t num_columns() const {
    return snapshot_.size();
  }

 private:
  std::vector<column_snapshot> snapshot_;
};

}

#endif /* CONFLUO_SCHEMA_SCHEMA_SNAPSHOT_H_ */
