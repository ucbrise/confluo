#include "schema/schema_snapshot.h"

namespace confluo {

void schema_snapshot::add_column(const column_snapshot &snap) {
  snapshot_.push_back(snap);
}

void schema_snapshot::add_column(column_snapshot &&snap) {
  snapshot_.push_back(std::move(snap));
}

immutable_value schema_snapshot::get(void *data, uint32_t i) const {
  return immutable_value(snapshot_[i].type, reinterpret_cast<uint8_t*>(data) + snapshot_[i].offset);
}

byte_string schema_snapshot::time_key(int64_t time_block) const {
  return LONG_TYPE.key_transform()(immutable_raw_data(reinterpret_cast<uint8_t*>(&time_block), LONG_TYPE.size), 1.0);
}

byte_string schema_snapshot::get_key(void *ptr, uint32_t i) const {
  return snapshot_[i].type.key_transform()(
      immutable_raw_data(reinterpret_cast<uint8_t*>(ptr) + snapshot_[i].offset, snapshot_[i].type.size),
      snapshot_[i].index_bucket_size);
}

int64_t schema_snapshot::get_timestamp(void *ptr) const {
  return *reinterpret_cast<int64_t*>(ptr);
}

bool schema_snapshot::is_indexed(size_t i) const {
  return snapshot_[i].indexed;
}

uint32_t schema_snapshot::index_id(size_t i) const {
  return snapshot_[i].index_id;
}

double schema_snapshot::index_bucket_size(size_t i) const {
  return snapshot_[i].index_bucket_size;
}

size_t schema_snapshot::num_columns() const {
  return snapshot_.size();
}

}