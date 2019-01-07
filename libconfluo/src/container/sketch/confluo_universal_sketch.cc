#include "container/sketch/confluo_universal_sketch.h"

namespace confluo {
namespace sketch {

confluo_universal_sketch::confluo_universal_sketch(double epsilon,
                                                   double gamma,
                                                   size_t k,
                                                   data_log *log,
                                                   column_t column)
    : confluo_universal_sketch(8 * sizeof(column.type().size),
                               count_sketch<counter_t>::error_margin_to_width(epsilon),
                               count_sketch<counter_t>::perror_to_depth(gamma),
                               k,
                               log,
                               column) {
}

confluo_universal_sketch::confluo_universal_sketch(size_t l,
                                                   size_t b,
                                                   size_t t,
                                                   size_t k,
                                                   data_log *log,
                                                   column_t column)
    : substream_sketches_(l),
      substream_heavy_hitters_(l),
      substream_hashes_(l - 1),
      hh_hash_(pairwise_indep_hash::generate_random()),
      num_layers_(l),
      data_log_(log),
      column_(std::move(column)),
      is_valid_(true) {
  substream_hashes_.guarantee_initialized(l - 1);
  for (size_t i = 0; i < l; i++) {
    substream_sketches_[i] = sketch_t(b, t);
    substream_heavy_hitters_[i] = heavy_hitters_t(k);
  }
}

confluo_universal_sketch::confluo_universal_sketch(const confluo_universal_sketch &other)
    : substream_sketches_(other.substream_sketches_),
      substream_heavy_hitters_(other.substream_heavy_hitters_.size()),
      substream_hashes_(other.substream_hashes_),
      hh_hash_(other.hh_hash_),
      num_layers_(other.num_layers_),
      data_log_(other.data_log_),
      column_(other.column_),
      is_valid_(atomic::load(&other.is_valid_)) {
  for (size_t i = 0; i < other.substream_heavy_hitters_.size(); i++) {
    for (size_t j = 0; j < other.substream_heavy_hitters_[i].size(); j++)
      atomic::store(&substream_heavy_hitters_[i][j], atomic::load(&other.substream_heavy_hitters_[i][j]));
  }
}

confluo_universal_sketch &confluo_universal_sketch::operator=(const confluo_universal_sketch &other) {
  substream_sketches_ = other.substream_sketches_;
  substream_heavy_hitters_ = std::vector<heavy_hitters_t>(other.substream_heavy_hitters_.size());
  substream_hashes_ = other.substream_hashes_;
  num_layers_ = other.num_layers_;
  hh_hash_ = other.hh_hash_;
  data_log_ = other.data_log_;
  column_ = other.column_;
  is_valid_ = atomic::load(&other.is_valid_);
  for (size_t i = 0; i < other.substream_heavy_hitters_.size(); i++) {
    for (size_t j = 0; j < other.substream_heavy_hitters_[i].size(); j++)
      atomic::store(&substream_heavy_hitters_[i][j], atomic::load(&other.substream_heavy_hitters_[i][j]));
  }
  return *this;
}

bool confluo_universal_sketch::is_valid() {
  return atomic::load(&is_valid_);
}

bool confluo_universal_sketch::invalidate() {
  bool expected = true;
  return atomic::strong::cas(&is_valid_, &expected, false);
}

void confluo_universal_sketch::update(const record_t &r) {
  key_t key = record_to_key(r);
  auto offset = r.log_offset();
  // Update top layer
  counter_t old_count = substream_sketches_[0].update_and_estimate(key);
  update_heavy_hitters(0, key, offset, old_count + 1);
  // Update rest
  for (size_t i = 1; i < num_layers_ && to_bool(substream_hashes_.hash(i - 1, key)); i++) {
    old_count = substream_sketches_[i].update_and_estimate(key);
    update_heavy_hitters(i, key, offset, old_count + 1);
  }
}

std::unordered_map<std::string, confluo_universal_sketch::counter_t> confluo_universal_sketch::get_heavy_hitters() {
  std::unordered_map<std::string, counter_t> heavy_hitters;
  read_only_data_log_ptr ptr;
  for (auto &hh : substream_heavy_hitters_[0]) {
    size_t record_offset = atomic::load(&hh);
    if (record_offset != zero()) {
      data_log_->cptr(record_offset, ptr);
      auto key = record_ptr_to_key(ptr);
      auto str_rep = record_ptr_to_string(ptr);
      heavy_hitters[str_rep] = substream_sketches_[0].estimate(key);
    }
  }
  return heavy_hitters;
}

size_t confluo_universal_sketch::storage_size() {
  size_t total_size = 0;
  for (size_t i = 0; i < num_layers_; i++) {
    total_size += (substream_sketches_[i].storage_size() + (substream_heavy_hitters_[i].size() * sizeof(key_t)));
  }
  return total_size;
}


confluo_universal_sketch::key_t confluo_universal_sketch::record_to_key(const record_t &r) {
  return r.at(column_.idx()).get_key().hash();
}

confluo_universal_sketch::key_t confluo_universal_sketch::record_ptr_to_key(const read_only_data_log_ptr &ptr) {
  auto field_value = column_.apply(ptr.decode().get());
  return field_value.get_key().hash();
}

std::string confluo_universal_sketch::record_ptr_to_string(const read_only_data_log_ptr &ptr) {
  const void *fptr = reinterpret_cast<const uint8_t *>(ptr.decode().get()) + column_.offset();
  data_type ftype = column_.type();
  return ftype.to_string_op()(immutable_raw_data(fptr, ftype.size));
}

void confluo_universal_sketch::update_heavy_hitters(size_t idx,
                                                    confluo_universal_sketch::key_t key,
                                                    size_t offset,
                                                    confluo_universal_sketch::counter_t count) {
  auto &heavy_hitters = substream_heavy_hitters_[idx];
  auto &sketch = substream_sketches_[idx];

  bool done = false;
  size_t hh_idx = hh_hash_.apply<key_t>(key) % heavy_hitters.size();
  while (!done) {
    size_t prev_record_offset = atomic::load(&heavy_hitters[hh_idx]);
    if (prev_record_offset == 0) {
      done = atomic::strong::cas(&heavy_hitters[hh_idx], &prev_record_offset, offset);
    }
    else {
      read_only_data_log_ptr ptr;
      data_log_->cptr(prev_record_offset, ptr);
      auto prev_key = record_ptr_to_key(ptr);
      auto prev_count = sketch.estimate(prev_key);
      done = (prev_count > count) ? true : atomic::strong::cas(&heavy_hitters[hh_idx], &prev_record_offset, offset);
    }
  }
}

}
}
