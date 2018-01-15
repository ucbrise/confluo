#ifndef CONFLUO_SCHEMA_INDEX_STATE_H_
#define CONFLUO_SCHEMA_INDEX_STATE_H_

#include <cstdint>
#include "atomic.h"

namespace confluo {

    /**
     * Possible index stages
     */
struct index_state_t {
  static const uint8_t UNINDEXED = 0;
  static const uint8_t INDEXING = 1;
  static const uint8_t INDEXED = 2;

  /**
   * index_state_t
   */
  index_state_t()
      : state_(UNINDEXED),
        id_(UINT16_MAX),
        bucket_size_(1) {
  }

  /**
   * index_state_t
   *
   * @param other The other
   */
  index_state_t(const index_state_t& other)
      : state_(atomic::load(&other.state_)),
        id_(other.id_),
        bucket_size_(other.bucket_size_) {

  }

  /**
   * id
   *
   * @return uint16_t
   */
  uint16_t id() const {
    return id_;
  }

  /**
   * bucket_size
   *
   * @return double
   */
  double bucket_size() const {
    return bucket_size_;
  }

  /**
   * operator=
   *
   * @param other The other
   *
   * @return index_state_t&
   */
  index_state_t& operator=(const index_state_t& other) {
    atomic::init(&state_, atomic::load(&other.state_));
    id_ = other.id_;
    return *this;
  }

  /**
   * is_indexed
   *
   * @return bool
   */
  bool is_indexed() const {
    return atomic::load(&state_) == INDEXED;
  }

  /**
   * set_indexing
   *
   * @return bool
   */
  bool set_indexing() {
    uint8_t expected = UNINDEXED;
    return atomic::strong::cas(&state_, &expected, INDEXING);
  }

  /**
   * set_indexed
   *
   * @param index_id The index_id
   * @param bucket_size The bucket_size
   */
  void set_indexed(uint16_t index_id, double bucket_size) {
    id_ = index_id;
    bucket_size_ = bucket_size;
    atomic::store(&state_, INDEXED);
  }

  /**
   * set_unindexed
   */
  void set_unindexed() {
    atomic::store(&state_, UNINDEXED);
  }

  /**
   * disable_indexing
   *
   * @return bool
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
