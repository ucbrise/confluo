#include "container/sketch/universal_sketch.h"

namespace confluo {
namespace sketch {

universal_sketch::universal_sketch(double epsilon, double gamma, size_t k, data_log *log, column_t column)
    : universal_sketch(8 * sizeof(column.type().size),
                       count_sketch<counter_t>::error_margin_to_width(epsilon),
                       count_sketch<counter_t>::perror_to_depth(gamma),
                       k,
                       log,
                       column) {
}

universal_sketch::universal_sketch(size_t l, size_t b, size_t t, size_t k, data_log *log, column_t column)
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

universal_sketch::universal_sketch(const universal_sketch &other)
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

universal_sketch &universal_sketch::operator=(const universal_sketch &other) {
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

bool universal_sketch::is_valid() {
  return atomic::load(&is_valid_);
}

bool universal_sketch::invalidate() {
  bool expected = true;
  return atomic::strong::cas(&is_valid_, &expected, false);
}

void universal_sketch::update(const record_t &r, size_t incr) {
  key_t key_hash = get_key_hash(r);
  auto offset = r.log_offset();
  // Update top layer
  counter_t old_count = substream_sketches_[0].update_and_estimate(key_hash, incr);
  update_heavy_hitters(0, key_hash, offset, old_count + incr);
  // Update rest
  for (size_t i = 1; i < num_layers_ && to_bool(substream_hashes_.hash(i - 1, key_hash)); i++) {
    old_count = substream_sketches_[i].update_and_estimate(key_hash, incr);
    update_heavy_hitters(i, key_hash, offset, old_count + incr);
  }
}

int64_t universal_sketch::estimate_frequency(const std::string &key) {
  key_t key_hash = str_to_key_hash(key);
  return substream_sketches_[0].estimate(key_hash);
}

universal_sketch::heavy_hitters_map_t universal_sketch::get_heavy_hitters(size_t num_layers) {
  heavy_hitters_map_t heavy_hitters;
  read_only_data_log_ptr ptr;
  for (size_t i = 0; i < num_layers; i++) {
    for (auto &hh : substream_heavy_hitters_[i]) {
      size_t record_offset = atomic::load(&hh);
      if (record_offset != zero()) {
        data_log_->cptr(record_offset, ptr);
        auto key = get_key_hash(ptr);
        auto str_rep = record_key_to_string(ptr);
        heavy_hitters[str_rep] = substream_sketches_[i].estimate(key);
      }
    }
  }
  return heavy_hitters;
}

size_t universal_sketch::storage_size() {
  size_t total_size = 0;
  for (size_t i = 0; i < num_layers_; i++) {
    total_size += (substream_sketches_[i].storage_size() + (substream_heavy_hitters_[i].size() * sizeof(key_t)));
  }
  return total_size;
}


universal_sketch::key_t universal_sketch::get_key_hash(const record_t &r) {
  return hash_util::hash(r.at(column_.idx()).value());
}

universal_sketch::key_t universal_sketch::get_key_hash(const read_only_data_log_ptr &ptr) {
  return hash_util::hash(column_.apply(ptr.decode().get()).value());
}

universal_sketch::key_t universal_sketch::str_to_key_hash(const std::string &str) {
  size_t data_size = column_.type().size;
  auto buf = std::unique_ptr<uint8_t[]>(new uint8_t[data_size]);
  column_.type().parse_op()(str, buf.get());
  key_t key_hash = hash_util::hash(buf.get(), data_size);
  return key_hash;
}

std::string universal_sketch::record_key_to_string(const read_only_data_log_ptr &ptr) {
  const void *fptr = reinterpret_cast<const uint8_t *>(ptr.decode().get()) + column_.offset();
  auto ftype = column_.type();
  return ftype.to_string_op()(immutable_raw_data(fptr, ftype.size));
}

void universal_sketch::update_heavy_hitters(size_t idx, key_t key_hash, size_t offset, counter_t count) {
  auto &heavy_hitters = substream_heavy_hitters_[idx];
  auto &sketch = substream_sketches_[idx];
  bool done = false;
  // TODO possibly use a different hash for each substream
  size_t hh_idx = hh_hash_.apply<key_t>(key_hash) % heavy_hitters.size();
  while (!done) {
    size_t prev_record_offset = atomic::load(&heavy_hitters[hh_idx]);
    if (prev_record_offset == 0) {
      done = atomic::strong::cas(&heavy_hitters[hh_idx], &prev_record_offset, offset);
    }
    else {
      read_only_data_log_ptr ptr;
      data_log_->cptr(prev_record_offset, ptr);
      auto prev_key = get_key_hash(ptr);
      auto prev_count = sketch.estimate(prev_key);
      done = (prev_count >= count) ? true : atomic::strong::cas(&heavy_hitters[hh_idx], &prev_record_offset, offset);
    }
  }
}

}
}
