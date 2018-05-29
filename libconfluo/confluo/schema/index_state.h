#ifndef CONFLUO_SCHEMA_INDEX_STATE_H_
#define CONFLUO_SCHEMA_INDEX_STATE_H_

#include <cstdint>
#include "atomic.h"

namespace confluo {

/**
 * Possible index stages
 */
struct index_state_t {
  /** Not indexed */
  static const uint8_t UNINDEXED = 0;
  /** In the process of being indexed */
  static const uint8_t INDEXING = 1;
  /** Already indexed */
  static const uint8_t INDEXED = 2;

  /**
   * Constructs a default index state, which is to be unindexed
   */
  index_state_t();

  /**
   * Constructs an index state from another index state
   *
   * @param other The other index state to construct this index state from
   */
  index_state_t(const index_state_t &other);

  /**
   * Gets the id of this index state
   *
   * @return The identifier of this index state
   */
  uint16_t id() const;

  /**
   * Gets the bucket size of this index state
   *
   * @return The bucket size of this index state
   */
  double bucket_size() const;

  /**
   * Assigns the other index state to this index state
   *
   * @param other The other index state to assign to this index state
   *
   * @return This index state which has the contents of the other index
   * state
   */
  index_state_t &operator=(const index_state_t &other);

  /**
   * Checks whether this index state is in the indexed stage
   *
   * @return True if this index state is indexed, false otherwise
   */
  bool is_indexed() const;

  /**
   * Sets this index stage to be indexing
   *
   * @return True if this index stage is indexing, false otherwise
   */
  bool set_indexing();

  /**
   * Sets the index stage to be indexed
   *
   * @param index_id The identifier for the index
   * @param bucket_size The bucket size for lookup
   */
  void set_indexed(uint16_t index_id, double bucket_size);

  /**
   * Sets the index stage to be not indexed
   */
  void set_unindexed();

  /**
   * If the index stage is indexed, sets the stage to be not indexed
   *
   * @return True if this index stage is unindexed, false otherwise
   */
  bool disable_indexing();

 private:
  atomic::type<uint8_t> state_;
  uint16_t id_;
  double bucket_size_;
};

}

#endif /* CONFLUO_SCHEMA_INDEX_STATE_H_ */
