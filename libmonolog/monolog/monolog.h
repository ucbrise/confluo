#ifndef MONOLOG_FACLOG_H_
#define MONOLOG_FACLOG_H_

#include <array>
#include <vector>
#include <atomic>
#include <fstream>

#include "utils.h"

namespace monolog {

template<typename monolog_impl>
class monolog_iterator :
  public std::iterator<std::input_iterator_tag,
  typename monolog_impl::value_type,
  typename monolog_impl::difference_type,
  typename monolog_impl::pointer,
  typename monolog_impl::reference> {
 public:
  typedef typename monolog_impl::value_type value_type;
  typedef typename monolog_impl::difference_type difference_type;
  typedef typename monolog_impl::pointer pointer;
  typedef typename monolog_impl::reference reference;

  monolog_iterator(monolog_impl& impl, size_t pos)
    : impl_(impl), pos_(pos) {
    pos_ = 0U;
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
class __monolog_base {
 public:
  static const size_t FBS = 16;
  static const size_t FBS_HIBIT = 4;

  typedef std::atomic<T*> __atomic_bucket_ref;

  __monolog_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      x.store(null_ptr, std::memory_order_release);
    }
    buckets_[0].store(new T[FBS], std::memory_order_release);
  }

  ~__monolog_base() {
    for (auto& x : buckets_) {
      delete[] x.load(std::memory_order_acquire);
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
      if (buckets_[i].load(std::memory_order_acquire) == NULL) {
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
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off] = val;
  }

  // Sets the data at index idx to val. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void set_unsafe(size_t idx, const T val) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off] = val;
  }

  // Sets a contiguous region of the MonoLog base to the provided data.
  void set(size_t idx, const T* data, const size_t len) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    while (data_remaining) {
      if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
        try_allocate_bucket(bucket_idx);
      }
      size_t bucket_remaining =
        ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off) * sizeof(T);
      size_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      memcpy(buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
             data + data_off, bytes_to_write);
      bucket_idx++;
      bucket_off = 0;
    }
  }

  // Sets a contiguous region of the MonoLog base to the provided data. Does
  // NOT allocate memory -- ensure memory is allocated before calling this
  // function.
  void set_unsafe(size_t idx, const T* data, const size_t len) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    while (data_remaining) {
      size_t bucket_remaining =
        ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off) * sizeof(T);
      size_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      memcpy(buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
             data + data_off, bytes_to_write);
      bucket_idx++;
      bucket_off = 0;
    }
  }

  // Gets the data at index idx.
  T get(const size_t idx) const {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off];
  }

  T& operator[](const size_t idx) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off];
  }

  // Copies a contiguous region of the MonoLog base into the provided buffer.
  // The buffer should have sufficient space to hold the data requested, otherwise
  // undefined behavior may result.
  void get(T* data, const size_t idx, const size_t len) const {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    while (data_remaining) {
      size_t bucket_remaining =
        ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off) * sizeof(T);
      size_t bytes_to_read = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_read;
      data_off += bytes_to_read;
      memcpy(data + data_off,
             buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
             bytes_to_read);
      bucket_idx++;
      bucket_off = 0;
    }
  }

  size_t storage_size() const {
    size_t bucket_size = buckets_.size() * sizeof(__atomic_bucket_ref );
    size_t data_size = 0;
    for (size_t i = 0; i < buckets_.size(); i++) {
      if (buckets_[i].load(std::memory_order_acquire) != NULL) {
        data_size += ((1U << (i + FBS_HIBIT)) * sizeof(T));
      }
    }
    return bucket_size + data_size;
  }

 protected:
  // Tries to allocate the specifies bucket. If another thread has already
  // succeeded in allocating the bucket, the current thread deallocates and
  // returns.
  size_t try_allocate_bucket(size_t bucket_idx) {
    size_t size = (1U << (bucket_idx + FBS_HIBIT));
    T* new_bucket = new T[size];
    T* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (!std::atomic_compare_exchange_strong_explicit(
          &buckets_[bucket_idx], &null_ptr, new_bucket, std::memory_order_release,
          std::memory_order_acquire)) {
      // All other threads will deallocate the newly allocated bucket.
      delete[] new_bucket;
    }

    return size;
  }

  std::array<__atomic_bucket_ref, NBUCKETS> buckets_;  // Stores the pointers to the buckets for MonoLog.
};

template<class T, size_t NBUCKETS = 1024, size_t BLOCK_SIZE = 1073741824UL>
class __monolog_linear_base {
 public:
  typedef std::atomic<T*> __atomic_bucket_ref;
  static const size_t BUFFER_SIZE = 1024; // 1KB buffer size

  __monolog_linear_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      x = null_ptr;
    }
    buckets_[0] = new T[BLOCK_SIZE + BUFFER_SIZE];
  }

  ~__monolog_linear_base() {
    for (auto& x : buckets_) {
      delete[] x.load(std::memory_order_acquire);
    }
  }

  void ensure_alloc(size_t idx1, size_t idx2) {
    size_t bucket_idx1 = idx1 / BLOCK_SIZE;
    size_t bucket_idx2 = idx2 / BLOCK_SIZE;
    for (size_t i = bucket_idx1; i <= bucket_idx2; i++) {
      if (buckets_[i].load(std::memory_order_acquire) == NULL) {
        try_allocate_bucket(i);
      }
    }
  }

  // Sets the data at index idx to val. Allocates memory if necessary.
  void set(size_t idx, const T val) {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off] = val;
  }

  // Sets the data at index idx to val. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void set_unsafe(size_t idx, const T val) {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off] = val;
  }

  // Write len bytes of data at offset.
  // Allocates memory if necessary.
  void write(const size_t offset, const T* data, const size_t len) {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    memcpy(buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
           data, len);
  }

  // Write len bytes of data at offset. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void write_unsafe(const size_t offset, const T* data, const size_t len) {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    memcpy(buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
           data, len);
  }

  // Gets the data at index idx.
  T get(const size_t idx) const {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off];
  }

  // Get len bytes of data at offset.
  void read(const size_t offset, T* data, const size_t len) const {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    memcpy(data,
           buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
           len);
  }

  T& operator[](const size_t idx) {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off];
  }

  void* ptr(const size_t offset) {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    return (void*)(buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off);
  }

  size_t storage_size() const {
    size_t bucket_size = buckets_.size() * sizeof(__atomic_bucket_ref );
    size_t data_size = 0;
    for (size_t i = 0; i < buckets_.size(); i++) {
      if (buckets_[i].load(std::memory_order_acquire) != NULL) {
        data_size += (BLOCK_SIZE * sizeof(T));
      }
    }
    return bucket_size + data_size;
  }

 protected:
  // Tries to allocate the specifies bucket. If another thread has already
  // succeeded in allocating the bucket, the current thread deallocates and
  // returns.
  void try_allocate_bucket(size_t bucket_idx) {
    T* bucket = new T[BLOCK_SIZE + BUFFER_SIZE];
    T* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (!std::atomic_compare_exchange_strong_explicit(
          &buckets_[bucket_idx], &null_ptr, bucket, std::memory_order_release,
          std::memory_order_acquire)) {
      // All other threads will deallocate the newly allocated bucket.
      delete[] bucket;
    }
  }

  std::array<__atomic_bucket_ref, NBUCKETS> buckets_;  // Stores the pointers to the buckets for MonoLog.
};

template<class T, size_t NBUCKETS = 32>
class __atomic_monolog_base {

  static_assert(std::is_fundamental<T>::value, "Type for atomic monolog must be primitive.");
 public:
  static const size_t FBS = 16;
  static const size_t FBS_HIBIT = 4;

  typedef std::atomic<T> __atomic_ref;
  typedef std::atomic<__atomic_ref *> __atomic_bucket_ref;

  __atomic_monolog_base() {
    __atomic_ref* null_ptr = NULL;
    for (auto& x : buckets_) {
      x.store(null_ptr, std::memory_order_release);
    }
    buckets_[0].store(new __atomic_ref[FBS], std::memory_order_release);
  }

  ~__atomic_monolog_base() {
    for (auto& x : buckets_) {
      delete[] x.load(std::memory_order_acquire);
    }
  }

  void alloc(size_t idx) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
  }

  void set(size_t idx, const T val) {
    store(idx, val);
  }

  // Atomically store value at index idx.
  // Allocates memory if necessary.
  void store(size_t idx, const T val) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off].store(val, std::memory_order_release);
  }

  __atomic_ref& operator[](const size_t idx) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off].load(std::memory_order_acquire);
  }

  T get(const size_t idx) const {
    return load(idx);
  }

  // Atomically loads the data at index idx.
  T load(const size_t idx) const {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off].load(std::memory_order_acquire);
  }

  bool cas(const size_t idx, T& expected, T replacement) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off]
           .atomic_compare_exchange_strong_explicit(expected, replacement,
               std::memory_order_release, std::memory_order_acquire);
  }

 protected:
  // Tries to allocate the specifies bucket. If another thread has already
  // succeeded in allocating the bucket, the current thread deallocates and
  // returns.
  size_t try_allocate_bucket(size_t bucket_idx) {
    size_t size = (1U << (bucket_idx + FBS_HIBIT));
    __atomic_ref* bucket = new_bucket(size);
    __atomic_ref* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (!std::atomic_compare_exchange_strong_explicit(&buckets_[bucket_idx], &null_ptr,
        bucket, std::memory_order_release, std::memory_order_acquire)) {
      // All other threads will deallocate the newly allocated bucket.
      delete[] bucket;
    }

    return size;
  }

  __atomic_ref* new_bucket(size_t size) {
    __atomic_ref* bucket = new __atomic_ref[size];
    for (size_t i = 0; i < size; i++) {
      bucket[i].store(T(0), std::memory_order_release);
    }
    return bucket;
  }

  std::array<__atomic_bucket_ref, NBUCKETS> buckets_;  // Stores the pointers to the buckets for MonoLog.
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
class monolog_write_stalled : public __monolog_base<T, NBUCKETS> {
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
  size_t push_back(const T val) {
    size_t idx = write_tail_.fetch_add(1UL, std::memory_order_release);
    this->set(idx, val);

    size_t expected = idx;
    while (!std::atomic_compare_exchange_weak_explicit(&read_tail_, &expected,
           idx + 1, std::memory_order_release, std::memory_order_acquire))
      expected = idx;
    return idx;
  }

  size_t push_back_range(const T start, const T end) {
    size_t cnt = (end - start + 1);
    size_t idx = write_tail_.fetch_add(cnt, std::memory_order_release);
    for (size_t i = 0; i < cnt; i++)
      this->set(idx + i, start + i);

    size_t expected = idx;
    while (!std::atomic_compare_exchange_weak_explicit(&read_tail_, &expected,
           idx + cnt, std::memory_order_release, std::memory_order_acquire))
      expected = idx;

    return idx;
  }

  // Get the entry at the specified index `idx'.
  T at(const size_t idx) const {
    return this->get(idx);
  }

  // Get the size of the MonoLog (i.e., number of completely written entries)
  size_t size() const {
    return read_tail_.load(std::memory_order_acquire);
  }

  iterator begin() const {
    return iterator(this, 0);
  }

  iterator end() const {
    return iterator(this, size());
  }

 private:
  std::atomic<size_t> write_tail_;
  std::atomic<size_t> read_tail_;
};

/**
 * Relaxed (i.e., not linearizable) implementation for the MonoLog.
 *
 * Maintains a single tail that ensures:
 * - Write operations are atomic
 */
template<class T, size_t NBUCKETS = 32>
class monolog_relaxed : public __monolog_base<T, NBUCKETS> {
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

  size_t push_back(const T val) {
    size_t idx = tail_.fetch_add(1U, std::memory_order_release);
    this->set(idx, val);
    return idx;
  }

  size_t push_back_range(const T start, const T end) {
    size_t cnt = (end - start + 1);
    size_t idx = tail_.fetch_add(cnt, std::memory_order_release);
    for (size_t i = 0; i < cnt; i++)
      this->set(idx + i, start + i);
    return idx;
  }

  T at(const size_t idx) const {
    return this->get(idx);
  }

  size_t size() const {
    return tail_.load(std::memory_order_acquire);
  }

  iterator begin() const {
    return iterator(this, 0);
  }

  iterator end() const {
    return iterator(this, size());
  }

 private:
  std::atomic<size_t> tail_;
};

}

#endif /* MONOLOG_MONOLOG_H_ */
