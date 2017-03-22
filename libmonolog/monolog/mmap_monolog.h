#ifndef MMAP_MONOLOG_H_
#define MMAP_MONOLOG_H_

#include "monolog.h"
#include "mmap_utils.h"

namespace monolog {

typedef bool block_state;

template<typename T, size_t BLOCK_SIZE = 33554432>
class mmapped_block {
 public:
  static const block_state uninit = false;
  static const block_state init = true;

  static const size_t BUFFER_SIZE = 1024;

  mmapped_block()
      : path_(""),
        state_(uninit),
        data_(nullptr) {
  }

  mmapped_block(const std::string& path)
      : path_(path),
        state_(uninit),
        data_(nullptr) {
  }

  mmapped_block(const mmapped_block& other)
      : path_(other.path_),
        state_(other.state_),
        data_(other.data_) {
  }

  void set_path(const std::string& path) {
    path_ = path;
  }

  size_t storage_size() const {
    if (atomic::load(&data_) != nullptr)
      return (BLOCK_SIZE + BUFFER_SIZE) * sizeof(T);
    return 0;
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
    memcpy(ptr + offset, data, len);
  }

  void write_unsafe(size_t offset, const T* data, size_t len) {
    memcpy(atomic::load(&data_) + offset, data, len);
  }

  T at(size_t i) const {
    return atomic::load(&data_)[i];
  }

  void read(size_t offset, T* data, size_t len) const {
    memcpy(data, atomic::load(&data_) + offset, len);
  }

  T& operator[](size_t i) {
    T* ptr;
    if ((ptr = atomic::load(&data_)) == nullptr)
      ptr = try_allocate();
    return ptr[i];
  }

  void* ptr(size_t offset) const {
    return (void*) (atomic::load(&data_) + offset);
  }

  mmapped_block& operator=(const mmapped_block& other) {
    path_ = other.path_;
    std::atomic_init(&state_, atomic::load(&other.state_));
    std::atomic_init(&data_, atomic::load(&other.data_));
    return *this;
  }

  void ensure_alloc() {
    if (atomic::load(&data_) == nullptr)
      try_allocate();
  }

 private:
  T* try_allocate() {
    block_state state = uninit;
    if (atomic::strong::cas(&state_, &state, init)) {
      size_t file_size = (BLOCK_SIZE + BUFFER_SIZE) * sizeof(T);
      T* data = (T*) utils::mmap_utils::mmap_rw(path_, file_size);
      atomic::store(&data_, data);
      return data;
    }

    // Stall until initialized
    T* data;
    while ((data = atomic::load(&data_)) == nullptr)
      ;

    return data;
  }

  std::string path_;
  std::atomic<block_state> state_;
  std::atomic<T*> data_;
};

template<typename T, size_t MAX_BLOCKS = 4096, size_t BLOCK_SIZE = 33554432>
class mmap_monolog {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<mmap_monolog<T, MAX_BLOCKS, BLOCK_SIZE>> iterator;

  mmap_monolog(const std::string& name, const std::string& data_path)
      : name_(name),
        tail_(0) {
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
      std::string block_path = data_path + "/" + name + "." + std::to_string(i);
      blocks_[i].set_path(block_path);
    }
    blocks_[0].ensure_alloc();
  }

  std::string name() const {
    return name_;
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
  void write(size_t offset, const T* data, const size_t len) {
    blocks_[offset / BLOCK_SIZE].write(offset % BLOCK_SIZE, data, len);
  }

  // Write len bytes of data at offset. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void write_unsafe(size_t offset, const T* data, const size_t len) {
    blocks_[offset / BLOCK_SIZE].write_unsafe(offset % BLOCK_SIZE, data, len);
  }

  // Gets the data at index idx.
  T get(size_t idx) const {
    return blocks_[idx / BLOCK_SIZE].at(idx % BLOCK_SIZE);
  }

  // Get len bytes of data at offset.
  void read(size_t offset, T* data, size_t len) const {
    blocks_[offset / BLOCK_SIZE].read(offset % BLOCK_SIZE, data, len);
  }

  T& operator[](const size_t idx) {
    return blocks_[idx / BLOCK_SIZE][idx % BLOCK_SIZE];
  }

  void* ptr(const size_t offset) {
    return blocks_[offset / BLOCK_SIZE].ptr(offset % BLOCK_SIZE);
  }

  size_t storage_size() const {
    size_t bucket_size = blocks_.size() * sizeof(mmapped_block<T, BLOCK_SIZE> );
    size_t data_size = 0;
    for (size_t i = 0; i < blocks_.size(); i++)
      data_size += blocks_[i].storage_size();
    return bucket_size + data_size;
  }

  size_t push_back(const T val) {
    size_t idx = atomic::faa(&tail_, 1UL);
    set(idx, val);
    return idx;
  }

  size_t push_back_range(const T start, const T end) {
    size_t cnt = (end - start + 1);
    size_t idx = atomic::faa(&tail_, cnt);
    for (size_t i = 0; i < cnt; i++)
      set(idx + i, start + i);
    return idx;
  }

  T at(const size_t idx) const {
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
  const std::string name_;
  const std::string data_path_;
  std::atomic<size_t> tail_;
  std::array<mmapped_block<T, BLOCK_SIZE>, MAX_BLOCKS> blocks_;
};

}

#endif /* MMAP_MONOLOG_H_ */
