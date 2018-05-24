#include "schema/index_state.h"

namespace confluo {

index_state_t::index_state_t()
    : state_(UNINDEXED),
      id_(UINT16_MAX),
      bucket_size_(1) {
}

index_state_t::index_state_t(const index_state_t &other)
    : state_(atomic::load(&other.state_)),
      id_(other.id_),
      bucket_size_(other.bucket_size_) {

}

uint16_t index_state_t::id() const {
  return id_;
}

double index_state_t::bucket_size() const {
  return bucket_size_;
}

index_state_t &index_state_t::operator=(const index_state_t &other) {
  atomic::init(&state_, atomic::load(&other.state_));
  id_ = other.id_;
  return *this;
}

bool index_state_t::is_indexed() const {
  return atomic::load(&state_) == INDEXED;
}

bool index_state_t::set_indexing() {
  uint8_t expected = UNINDEXED;
  return atomic::strong::cas(&state_, &expected, INDEXING);
}

void index_state_t::set_indexed(uint16_t index_id, double bucket_size) {
  id_ = index_id;
  bucket_size_ = bucket_size;
  atomic::store(&state_, INDEXED);
}

void index_state_t::set_unindexed() {
  atomic::store(&state_, UNINDEXED);
}

bool index_state_t::disable_indexing() {
  uint8_t expected = INDEXED;
  return atomic::strong::cas(&state_, &expected, UNINDEXED);
}

}