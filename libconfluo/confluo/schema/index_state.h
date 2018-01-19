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
  index_state_t()
      : state_(UNINDEXED),
        id_(UINT16_MAX),
        bucket_size_(1) {
  }

  /**
   * Constructs an index state from another index state
   *
   * @param other The other index state to construct this index state from
   */
  index_state_t(const index_state_t& other)
      : state_(atomic::load(&other.state_)),
        id_(other.id_),
        bucket_size_(other.bucket_size_) {

  }

  /**
   * Gets the id of this index state
   *
   * @return The identifier of this index state
   */
  uint16_t id() const {
    return id_;
  }

  /**
   * Gets the bucket size of this index state
   *
   * @return The bucket size of this index state
   */
  double bucket_size() const {
    return bucket_size_;
  }

  /**
   * Assigns the other index state to this index state
   *
   * @param other The other index state to assign to this index state
   *
   * @return This index state which has the contents of the other index
   * state
   */
  index_state_t& operator=(const index_state_t& other) {
    atomic::init(&state_, atomic::load(&other.state_));
    id_ = other.id_;
    return *this;
  }

  /**
   * Checks whether this index state is in the indexed stage
   *
   * @return True if this index state is indexed, false otherwise
   */
  bool is_indexed() const {
    return atomic::load(&state_) == INDEXED;
  }

  /**
   * Sets this index stage to be indexing
   *
   * @return True if this index stage is indexing, false otherwise
   */
  bool set_indexing() {
    uint8_t expected = UNINDEXED;
    return atomic::strong::cas(&state_, &expected, INDEXING);
  }

  /**
   * Sets the index stage to be indexed
   *
   * @param index_id The identifier for the index
   * @param bucket_size The bucket size for lookup
   */
  void set_indexed(uint16_t index_id, double bucket_size) {
    id_ = index_id;
    bucket_size_ = bucket_size;
    atomic::store(&state_, INDEXED);
  }

  /**
   * Sets the index stage to be not indexed
   */
  void set_unindexed() {
    atomic::store(&state_, UNINDEXED);
  }

  /**
   * If the index stage is indexed, sets the stage to be not indexed
   *
   * @return True if this index stage is unindexed, false otherwise
   */
  bool disable_indexing() {
    uint8_t expected = INDEXED;
    return atomic::strong::cas(&state_, &expected, UNINDEXED);
  }

 private:
  atomic::type<uint8_t> state_;
  uint16_t id_;
  double bucket_size_;
};

const uint8_t index_state_t::UNINDEXED;
const uint8_t index_state_t::INDEXING;
const uint8_t index_state_t::INDEXED;

}

#endif /* CONFLUO_SCHEMA_INDEX_STATE_H_ */
