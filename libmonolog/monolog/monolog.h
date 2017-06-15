#ifndef MONOLOG_MONOLOG_H_
#define MONOLOG_MONOLOG_H_

#include <array>
#include <vector>
#include <fstream>

#include "atomic.h"
#include "bit_utils.h"
#include "mmap_utils.h"

using namespace utils;

namespace monolog {

/**
 * Iterator for monologs.
 */

template<typename monolog_impl>
class monolog_iterator : public std::iterator<std::input_iterator_tag,
    typename monolog_impl::value_type, typename monolog_impl::difference_type,
    typename monolog_impl::pointer, typename monolog_impl::reference> {
 public:
  typedef typename monolog_impl::value_type value_type;
  typedef typename monolog_impl::difference_type difference_type;
  typedef typename monolog_impl::pointer pointer;
  typedef typename monolog_impl::reference reference;

  monolog_iterator(monolog_impl& impl, size_t pos)
      : impl_(impl),
        pos_(pos) {
  }

  reference operator*() const {
    return impl_.get(pos_);
  }

  monolog_iterator& operator++() {
    pos_++;
    return *this;
  }

  monolog_iterator operator++(int) {
    monolog_iterator it = *this;
    ++(*this);
    return it;
  }

  bool operator==(monolog_iterator other) const {
    return (impl_ == other.impl_) && (pos_ == other.pos_);
  }

  bool operator!=(monolog_iterator other) const {
    return !(*this == other);
  }

 private:
  monolog_impl& impl_;
  size_t pos_;
};

/**
 * The base class for Monotonic Log (MonoLog).
 *
 * Implements get/set/multiget/multiset functionalities,
 * but does not maintain read or write tails and does not
 * provide any atomicity/consistency guarantees by itself.
 *
 */
template<class T, size_t NBUCKETS = 32>
class monolog_base {
 public:
  static const size_t FBS = 16;
  static const size_t FBS_HIBIT = 4;

  typedef atomic::type<T*> __atomic_bucket_ref;

  monolog_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      atomic::init(&x, null_ptr);
    }
    atomic::init(&buckets_[0], new T[FBS]);
  }

  ~monolog_base() {
    for (auto& x : buckets_) {
      delete[] atomic::load(&x);
    }
  }

  void ensure_alloc(size_t start_idx, size_t end_idx) {
    size_t pos1 = start_idx + FBS;
    size_t pos2 = end_idx + FBS;
    size_t hibit1 = bit_utils::highest_bit(pos1);
    size_t hibit2 = bit_utils::highest_bit(pos2);
    size_t bucket_idx1 = hibit1 - FBS_HIBIT;
    size_t bucket_idx2 = hibit2 - FBS_HIBIT;
    for (size_t i = bucket_idx1; i <= bucket_idx2; i++) {
      if (atomic::load(&buckets_[i]) == NULL) {
        try_allocate_bucket(i);
      }
    }
  }

  // Sets the data at index idx to val. Allocates memory if necessary.
  void set(size_t idx, const T val) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    T* bucket;
    if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
      bucket = try_allocate_bucket(bucket_idx);
    }
    bucket[bucket_off] = val;
  }

  // Sets the data at index idx to val. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void set_unsafe(size_t idx, const T val) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    atomic::load(&buckets_[bucket_idx])[bucket_off] = val;
  }

  // Sets a contiguous region of the MonoLog base to the provided data.
  void set(size_t idx, const T* data, size_t len) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    while (data_remaining) {
      T* bucket;
      if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
        bucket = try_allocate_bucket(bucket_idx);
      }
      size_t bucket_remaining = ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off)
          * sizeof(T);
      size_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      memcpy(bucket + bucket_off, data + data_off, bytes_to_write);
      bucket_idx++;
      bucket_off = 0;
    }
  }

  // Sets a contiguous region of the MonoLog base to the provided data. Does
  // NOT allocate memory -- ensure memory is allocated before calling this
  // function.
  void set_unsafe(size_t idx, const T* data, size_t len) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    while (data_remaining) {
      size_t bucket_remaining = ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off)
          * sizeof(T);
      size_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      memcpy(atomic::load(&buckets_[bucket_idx]) + bucket_off, data + data_off,
             bytes_to_write);
      bucket_idx++;
      bucket_off = 0;
    }
  }

  // Gets the data at index idx.
  const T& get(size_t idx) const {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    return atomic::load(&buckets_[bucket_idx])[bucket_off];
  }

  T& operator[](size_t idx) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    T* bucket;
    if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
      bucket = try_allocate_bucket(bucket_idx);
    }
    return bucket[bucket_off];
  }

  // Copies a contiguous region of the MonoLog base into the provided buffer.
  // The buffer should have sufficient space to hold the data requested, otherwise
  // undefined behavior may result.
  void get(T* data, size_t idx, size_t len) const {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    while (data_remaining) {
      size_t bucket_remaining = ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off)
          * sizeof(T);
      size_t bytes_to_read = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_read;
      data_off += bytes_to_read;
      memcpy(data + data_off, atomic::load(&buckets_[bucket_idx]) + bucket_off,
             bytes_to_read);
      bucket_idx++;
      bucket_off = 0;
    }
  }

  size_t storage_size() const {
    size_t bucket_size = buckets_.size() * sizeof(__atomic_bucket_ref );
    size_t data_size = 0;
    for (size_t i = 0; i < buckets_.size(); i++) {
      if (atomic::load(&buckets_[i]) != NULL) {
        data_size += ((1U << (i + FBS_HIBIT)) * sizeof(T));
      }
    }
    return bucket_size + data_size;
  }

 protected:
  // Tries to allocate the specifies bucket. If another thread has already
  // succeeded in allocating the bucket, the current thread deallocates and
  // returns.
  T* try_allocate_bucket(size_t bucket_idx) {
    size_t size = (1U << (bucket_idx + FBS_HIBIT));
    T* new_bucket = new T[size];
    T* expected = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (!atomic::strong::cas(&buckets_[bucket_idx], &expected, new_bucket)) {
      // All other threads will deallocate the newly allocated bucket.
      delete[] new_bucket;
      return expected;
    }

    return new_bucket;
  }

  std::array<__atomic_bucket_ref, NBUCKETS> buckets_;  // Stores the pointers to the buckets for MonoLog.
};

template<class T, size_t NBUCKETS = 1024, size_t BLOCK_SIZE = 1073741824UL>
class monolog_linear_base {
 public:
  typedef atomic::type<T*> __atomic_bucket_ref;
  static const size_t BUFFER_SIZE = 1048576;  // 1KB buffer size

  monolog_linear_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      atomic::init(&x, null_ptr);
    }
    atomic::init(&buckets_[0], new T[BLOCK_SIZE + BUFFER_SIZE]);
  }

  ~monolog_linear_base() {
    for (auto& x : buckets_) {
      delete[] atomic::load(&x);
    }
  }

  void ensure_alloc(size_t idx1, size_t idx2) {
    size_t bucket_idx1 = idx1 / BLOCK_SIZE;
    size_t bucket_idx2 = idx2 / BLOCK_SIZE;
    for (size_t i = bucket_idx1; i <= bucket_idx2; i++) {
      if (atomic::load(&buckets_[i]) == NULL) {
        try_allocate_bucket(i);
      }
    }
  }

  // Sets the data at index idx to val. Allocates memory if necessary.
  void set(size_t idx, const T& val) {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    T* bucket;
    if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
      bucket = try_allocate_bucket(bucket_idx);
    }
    bucket[bucket_off] = val;
  }

  // Sets the data at index idx to val. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void set_unsafe(size_t idx, const T val) {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    atomic::load(&buckets_[bucket_idx])[bucket_off] = val;
  }

  // Write len bytes of data at offset.
  // Allocates memory if necessary.
  void write(size_t offset, const T* data, size_t len) {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = offset / BLOCK_SIZE;
      size_t bucket_off = offset % BLOCK_SIZE;
      size_t bucket_len = std::min(BLOCK_SIZE - bucket_off, remaining);
      T* bucket;
      if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
        bucket = try_allocate_bucket(bucket_idx);
      }
      memcpy(bucket + bucket_off, data + len - remaining,
             bucket_len * sizeof(T));
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
      memcpy(atomic::load(&buckets_[bucket_idx]) + bucket_off,
             data + len - remaining, bucket_len * sizeof(T));
      offset += bucket_len;
      remaining -= bucket_len;
    }
  }

  // Gets the data at index idx.
  const T& get(size_t idx) const {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    return atomic::load(&buckets_[bucket_idx])[bucket_off];
  }

  // Get len bytes of data at offset.
  void read(size_t offset, T* data, size_t len) const {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = offset / BLOCK_SIZE;
      size_t bucket_off = offset % BLOCK_SIZE;
      size_t bucket_len = std::min(BLOCK_SIZE - bucket_off, remaining);
      memcpy(data + len - remaining,
             atomic::load(&buckets_[bucket_idx]) + bucket_off,
             bucket_len * sizeof(T));
      offset += bucket_len;
      remaining -= bucket_len;
    }
  }

  T& operator[](size_t idx) {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    T* bucket;
    if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
      bucket = try_allocate_bucket(bucket_idx);
    }
    return bucket[bucket_off];
  }

  void* ptr(size_t offset) {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    T *bucket;
    if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
      bucket = try_allocate_bucket(bucket_idx);
    }
    return (void*) (bucket + bucket_off);
  }

  const void* cptr(size_t offset) const {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    return (void*) (atomic::load(&buckets_[bucket_idx]) + bucket_off);
  }

  size_t storage_size() const {
    size_t bucket_size = buckets_.size() * sizeof(__atomic_bucket_ref );
    size_t data_size = 0;
    for (size_t i = 0; i < buckets_.size(); i++) {
      if (atomic::load(&buckets_[i]) != NULL) {
        data_size += ((BLOCK_SIZE + BUFFER_SIZE) * sizeof(T));
      }
    }
    return bucket_size + data_size;
  }

 protected:
// Tries to allocate the specifies bucket. If another thread has already
// succeeded in allocating the bucket, the current thread deallocates and
// returns.
  T* try_allocate_bucket(size_t bucket_idx) {
    T* bucket = new T[BLOCK_SIZE + BUFFER_SIZE]();
    T* expected = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (!atomic::strong::cas(&buckets_[bucket_idx], &expected, bucket)) {
      // All other threads will deallocate the newly allocated bucket.
      delete[] bucket;
      return expected;
    }
    return bucket;
  }

  std::array<__atomic_bucket_ref, NBUCKETS> buckets_;  // Stores the pointers to the buckets for MonoLog.
};

typedef bool block_state;

template<typename T, size_t BLOCK_SIZE = 33554432>
class mmapped_block {
 public:
  static const block_state uninit = false;
  static const block_state init = true;

  static size_t const BUFFER_SIZE = 1048576;

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
    memcpy(ptr + offset, data, len * sizeof(T));
  }

  void write_unsafe(size_t offset, const T* data, size_t len) {
    memcpy(atomic::load(&data_) + offset, data, len * sizeof(T));
  }

  void flush(size_t offset, size_t len) {
    utils::mmap_utils::mmap_flush(atomic::load(&data_) + offset,
                                  len * sizeof(T));
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
    if ((data = atomic::load(&data_)) == NULL) {
      data = try_allocate();
    }
    return (void*) (data + offset);
  }

  const void* cptr(size_t offset) const {
    return (void*) (atomic::load(&data_) + offset);
  }

  mmapped_block& operator=(const mmapped_block& other) {
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
    block_state state = uninit;
    if (atomic::strong::cas(&state_, &state, init)) {
      size_t file_size = (BLOCK_SIZE + BUFFER_SIZE) * sizeof(T);
      T* data = (T*) utils::mmap_utils::mmap_rw_init(path_, file_size, 0xFF);
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
  atomic::type<block_state> state_;
  atomic::type<T*> data_;
};

template<typename T, size_t MAX_BLOCKS = 4096, size_t BLOCK_SIZE = 33554432>
class mmap_monolog_base {
 public:

  mmap_monolog_base() = default;

  mmap_monolog_base(const std::string& name, const std::string& data_path) {
    init(name, data_path);
  }

  void init(const std::string& name, const std::string& data_path) {
    name_ = name;
    data_path_ = data_path;
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
      std::string block_path = data_path + "/" + name + "." + std::to_string(i);
      blocks_[i].set_path(block_path);
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
    size_t bucket_size = blocks_.size() * sizeof(mmapped_block<T, BLOCK_SIZE> );
    size_t data_size = 0;
    for (size_t i = 0; i < blocks_.size(); i++)
      data_size += blocks_[i].storage_size();
    return bucket_size + data_size;
  }

 protected:
  std::string name_;
  std::string data_path_;
  std::array<mmapped_block<T, BLOCK_SIZE>, MAX_BLOCKS> blocks_;
};

/**
 * Write stalled, linearizable implementation for the MonoLog.
 *
 * Maintains a read and write tail to ensure:
 * - Read operations can only access data for completed writes; conversely,
 *   read operations can access data for all completed writes.
 * - Completion times for write operations are strictly ordered by their
 *   start times.
 */
template<class T, size_t NBUCKETS = 32>
class monolog_write_stalled : public monolog_base<T, NBUCKETS> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<monolog_write_stalled<T, NBUCKETS>> iterator;

  monolog_write_stalled()
      : write_tail_(0),
        read_tail_(0) {
  }

  // Append an entry at the end of the MonoLog
  size_t push_back(const T& val) {
    size_t idx = atomic::faa(&write_tail_, 1UL);
    this->set(idx, val);

    size_t expected = idx;
    while (!atomic::weak::cas(&read_tail_, &expected, idx + 1))
      expected = idx;
    return idx;
  }

  size_t push_back_range(const T& start, const T& end) {
    size_t cnt = (end - start + 1);
    size_t idx = atomic::faa(&write_tail_, cnt);
    for (size_t i = 0; i < cnt; i++)
      this->set(idx + i, start + i);

    size_t expected = idx;
    while (!atomic::weak::cas(&read_tail_, &expected, idx + cnt))
      expected = idx;

    return idx;
  }

  // Get the entry at the specified index `idx'.
  const T& at(size_t idx) const {
    return this->get(idx);
  }

  // Get the size of the MonoLog (i.e., number of completely written entries)
  size_t size() const {
    return atomic::load(&read_tail_);
  }

  iterator begin() const {
    return iterator(this, 0);
  }

  iterator end() const {
    return iterator(this, size());
  }

 private:
  atomic::type<size_t> write_tail_;
  atomic::type<size_t> read_tail_;
};

/**
 * Relaxed (i.e., not linearizable) implementation for the MonoLog.
 *
 * Maintains a single tail that ensures:
 * - Write operations are atomic
 */
template<class T, size_t NBUCKETS = 32>
class monolog_relaxed : public monolog_base<T, NBUCKETS> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<monolog_relaxed<T, NBUCKETS>> iterator;

  monolog_relaxed()
      : tail_(0) {
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

template<class T, size_t NBUCKETS = 32, size_t BLOCK_SIZE = 1073741824UL>
class monolog_relaxed_linear : public monolog_linear_base<T, NBUCKETS,
    BLOCK_SIZE> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<monolog_relaxed_linear<T, NBUCKETS>> iterator;

  monolog_relaxed_linear()
      : tail_(0) {
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

template<typename T, size_t MAX_BLOCKS = 4096, size_t BLOCK_SIZE = 33554432>
class mmap_monolog_relaxed : public mmap_monolog_base<T, MAX_BLOCKS, BLOCK_SIZE> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<mmap_monolog_relaxed<T, MAX_BLOCKS, BLOCK_SIZE>> iterator;

  mmap_monolog_relaxed()
      : mmap_monolog_base<T, MAX_BLOCKS, BLOCK_SIZE>(),
        tail_(0UL) {
  }

  mmap_monolog_relaxed(const std::string& name, const std::string& data_path)
      : mmap_monolog_base<T, MAX_BLOCKS, BLOCK_SIZE>(name, data_path),
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

template<typename monolog_type = monolog_base<atomic::type<uint64_t>, 1024>>
class monolog_bitvector {
 public:
  monolog_bitvector() = default;

  void ensure_alloc(uint64_t start, uint64_t end) {
    bits_.ensure_alloc(start / 64, end / 64);
  }

  bool get_bit(uint64_t i) const {
    return utils::bit_utils::get_bit(bits_.get(i / 64), i % 64);
  }

  void set_bit(uint64_t i) {
    uint64_t block = bits_[i / 64];
    while (!atomic::weak::cas(&bits_[i / 64], &block,
                              utils::bit_utils::set_bit(block, i % 64))) {
    }
  }

  void set_bits(uint64_t i, uint64_t count) {
    uint64_t bidx = i / 64;
    uint64_t bidx_max = (i + count) / 64;
    uint64_t boff = i % 64;
    uint64_t rem = (bidx != bidx_max) ? 64 - boff : 64 - boff - count;

    if (boff == 0 && rem == 64) {
      atomic::store(&bits_[bidx], UINT64_C(0xFFFFFFFFFFFFFFFF));
    } else {
      uint64_t block = bits_[bidx];
      while (!atomic::weak::cas(&bits_[bidx], &block,
                                utils::bit_utils::set_bits(block, boff, rem))) {
      }
    }

    if (bidx == bidx_max)
      return;

    bidx++;
    while (bidx != bidx_max) {
      atomic::store(&bits_[bidx], UINT64_C(0xFFFFFFFFFFFFFFFF));
      bidx++;
    }

    if ((rem = (i + count) % 64)) {
      uint64_t block = bits_[bidx];
      while (!atomic::weak::cas(&bits_[bidx], &block,
                                utils::bit_utils::set_bits(block, 0, rem))) {
      }
    }
  }

 private:
  monolog_type bits_;
};

}

#endif /* MONOLOG_MONOLOG_H_ */
