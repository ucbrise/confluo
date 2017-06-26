#ifndef MONOLOG_MONOLOG_LINEAR_H_
#define MONOLOG_MONOLOG_LINEAR_H_

#include <array>
#include <vector>

#include "storage.h"
#include "atomic.h"

// TODO: Add documentation

namespace dialog {
namespace monolog {

template<typename T, size_t BUFFER_SIZE = 1048576,
    class storage_mode = storage::in_memory>
class monolog_block {
 public:
  typedef bool block_state;

  static const block_state UNINIT = false;
  static const block_state INIT = true;

  monolog_block()
      : path_(""),
        state_(UNINIT),
        data_(nullptr),
        size_(0) {
  }

  monolog_block(const std::string& path, size_t size)
      : path_(path),
        state_(UNINIT),
        data_(nullptr),
        size_(size) {
  }

  monolog_block(const monolog_block& other)
      : path_(other.path_),
        state_(other.state_),
        data_(other.data_),
        size_(other.size_) {
  }

  void init(const std::string& path, const size_t size) {
    path_ = path;
    size_ = size;
  }

  size_t storage_size() const {
    if (atomic::load(&data_) != nullptr)
      return (size_ + BUFFER_SIZE) * sizeof(T);
    return 0;
  }

  void flush(size_t offset, size_t len) {
    storage_mode::flush(atomic::load(&data_) + offset, len * sizeof(T));
  }

  void set(size_t i, const T& val) {
    T* ptr;
    if ((ptr = atomic::load(&data_)) == nullptr)
      ptr = try_allocate();
    ptr[i] = val;
  }

  void set_unsafe(size_t i, const T& val) {
    atomic::load(&data_)[i] = val;
  }

  void write(size_t offset, const T* data, size_t len) {
    T* ptr;
    if ((ptr = atomic::load(&data_)) == nullptr)
      ptr = try_allocate();
    memcpy(ptr + offset, data, len * sizeof(T));
  }

  void write_unsafe(size_t offset, const T* data, size_t len) {
    memcpy(atomic::load(&data_) + offset, data, len * sizeof(T));
  }

  const T& at(size_t i) const {
    return atomic::load(&data_)[i];
  }

  void read(size_t offset, T* data, size_t len) const {
    memcpy(data, atomic::load(&data_) + offset, len * sizeof(T));
  }

  T& operator[](size_t i) {
    T* ptr;
    if ((ptr = atomic::load(&data_)) == nullptr)
      ptr = try_allocate();
    return ptr[i];
  }

  void* ptr(size_t offset) {
    T *data;
    if ((data = atomic::load(&data_)) == nullptr) {
      data = try_allocate();
    }
    return (void*) (data + offset);
  }

  const void* cptr(size_t offset) const {
    return (void*) (atomic::load(&data_) + offset);
  }

  monolog_block& operator=(const monolog_block& other) {
    path_ = other.path_;
    atomic::init(&state_, atomic::load(&other.state_));
    atomic::init(&data_, atomic::load(&other.data_));
    return *this;
  }

  void ensure_alloc() {
    if (atomic::load(&data_) == nullptr)
      try_allocate();
  }

 private:
  T* try_allocate() {
    block_state state = UNINIT;
    if (atomic::strong::cas(&state_, &state, INIT)) {
      size_t file_size = (size_ + BUFFER_SIZE) * sizeof(T);
      T* data = (T*) storage_mode::allocate(path_, file_size);
      atomic::store(&data_, data);
      return data;
    }

    // Someone else is initializing, stall until initialized
    T* data;
    while ((data = atomic::load(&data_)) == nullptr)
      ;

    return data;
  }

  std::string path_;
  atomic::type<block_state> state_;
  atomic::type<T*> data_;
  size_t size_;
};

template<typename T, size_t MAX_BLOCKS = 4096, size_t BLOCK_SIZE = 268435456,
    size_t BUFFER_SIZE = 1048576, class storage_mode = storage::in_memory>
class monolog_linear_base {
 public:
  monolog_linear_base() = default;

  monolog_linear_base(const std::string& name, const std::string& data_path) {
    init(name, data_path);
  }

  void init(const std::string& name, const std::string& data_path) {
    name_ = name;
    data_path_ = data_path;
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
      std::string block_path = data_path + "/" + name + "_" + std::to_string(i)
          + ".dat";
      blocks_[i].init(block_path, BLOCK_SIZE);
    }
    blocks_[0].ensure_alloc();
  }

  std::string name() const {
    return name_;
  }

  std::string data_path() const {
    return data_path_;
  }

  void ensure_alloc(size_t idx1, size_t idx2) {
    size_t bucket_idx1 = idx1 / BLOCK_SIZE;
    size_t bucket_idx2 = idx2 / BLOCK_SIZE;
    for (size_t i = bucket_idx1; i <= bucket_idx2; i++)
      blocks_[i].ensure_alloc();
  }

  // Sets the data at index idx to val. Allocates memory if necessary.
  void set(size_t idx, const T& val) {
    blocks_[idx / BLOCK_SIZE].set(idx % BLOCK_SIZE, val);
  }

  // Sets the data at index idx to val. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void set_unsafe(size_t idx, const T val) {
    blocks_[idx / BLOCK_SIZE].set_unsafe(idx % BLOCK_SIZE, val);
  }

  // Write len bytes of data at offset.
  // Allocates memory if necessary.
  void write(size_t offset, const T* data, size_t len) {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = offset / BLOCK_SIZE;
      size_t bucket_off = offset % BLOCK_SIZE;
      size_t bucket_len = std::min(BLOCK_SIZE - bucket_off, remaining);
      blocks_[bucket_idx].write(bucket_off, data + len - remaining, bucket_len);
      offset += bucket_len;
      remaining -= bucket_len;
    }
  }

  // Write len bytes of data at offset. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void write_unsafe(size_t offset, const T* data, size_t len) {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = offset / BLOCK_SIZE;
      size_t bucket_off = offset % BLOCK_SIZE;
      size_t bucket_len = std::min(BLOCK_SIZE - bucket_off, remaining);
      blocks_[bucket_idx].write_unsafe(bucket_off, data + len - remaining,
                                       bucket_len);
      offset += bucket_len;
      remaining -= bucket_len;
    }
  }

  void flush(size_t offset, size_t len) {
    blocks_[offset / BLOCK_SIZE].flush(offset % BLOCK_SIZE, len);
  }

  // Gets the data at index idx.
  const T& get(size_t idx) const {
    return blocks_[idx / BLOCK_SIZE].at(idx % BLOCK_SIZE);
  }

  // Get len bytes of data at offset.
  void read(size_t offset, T* data, size_t len) const {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = offset / BLOCK_SIZE;
      size_t bucket_off = offset % BLOCK_SIZE;
      size_t bucket_len = std::min(BLOCK_SIZE - bucket_off, remaining);
      blocks_[bucket_idx].read(bucket_off, data + len - remaining, bucket_len);
      offset += bucket_len;
      remaining -= bucket_len;
    }
  }

  T& operator[](size_t idx) {
    return blocks_[idx / BLOCK_SIZE][idx % BLOCK_SIZE];
  }

  void* ptr(size_t offset) {
    return blocks_[offset / BLOCK_SIZE].ptr(offset % BLOCK_SIZE);
  }

  const void* cptr(size_t offset) const {
    return blocks_[offset / BLOCK_SIZE].cptr(offset % BLOCK_SIZE);
  }

  size_t storage_size() const {
    size_t bucket_size = blocks_.size() * sizeof(monolog_block<T> );
    size_t data_size = 0;
    for (size_t i = 0; i < blocks_.size(); i++)
      data_size += blocks_[i].storage_size();
    return bucket_size + data_size;
  }

 protected:
  std::string name_;
  std::string data_path_;
  std::array<monolog_block<T, BUFFER_SIZE, storage_mode>, MAX_BLOCKS> blocks_;
};

template<typename T, size_t MAX_BLOCKS = 4096, size_t BLOCK_SIZE = 268435456,
    size_t BUFFER_SIZE = 1048576, class storage_mode = storage::in_memory>
class monolog_linear : public monolog_linear_base<T, MAX_BLOCKS, BLOCK_SIZE,
    BUFFER_SIZE, storage_mode> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<
      monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE, storage_mode>> iterator;

  monolog_linear()
      : monolog_linear_base<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE, storage_mode>(),
        tail_(0UL) {
  }

  monolog_linear(const std::string& name, const std::string& data_path)
      : monolog_linear_base<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE, storage_mode>(
            name, data_path),
        tail_(0UL) {
  }

  size_t push_back(const T& val) {
    size_t idx = atomic::faa(&tail_, 1UL);
    this->set(idx, val);
    return idx;
  }

  size_t push_back_range(const T& start, const T& end) {
    size_t cnt = (end - start + 1);
    size_t idx = atomic::faa(&tail_, cnt);
    for (size_t i = 0; i < cnt; i++)
      this->set(idx + i, start + i);
    return idx;
  }

  size_t append(const T* data, size_t len) {
    size_t offset = atomic::faa(&tail_, len);
    this->write(offset, data, len);
    return offset;
  }

  size_t reserve(size_t len) {
    return atomic::faa(&tail_, len);
  }

  const T& at(size_t idx) const {
    return this->get(idx);
  }

  size_t size() const {
    return atomic::load(&tail_);
  }

  iterator begin() const {
    return iterator(this, 0);
  }

  iterator end() const {
    return iterator(this, size());
  }

 private:
  atomic::type<size_t> tail_;
};

}
}

#endif /* MONOLOG_MONOLOG_LINEAR_H_ */
