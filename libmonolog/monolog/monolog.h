#ifndef MONOLOG_MONOLOG_H_
#define MONOLOG_MONOLOG_H_

#include <array>
#include <vector>
#include <fstream>

#include "atomic.h"
#include "utils.h"

namespace monolog {

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
class __monolog_base {
 public:
  static const size_t FBS = 16;
  static const size_t FBS_HIBIT = 4;

  typedef std::atomic<T*> __atomic_bucket_ref;

  __monolog_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      atomic::init(&x, null_ptr);
    }
    atomic::init(&buckets_[0], new T[FBS]);
  }

  ~__monolog_base() {
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
  void set(size_t idx, const T* data, const size_t len) {
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
  void set_unsafe(size_t idx, const T* data, const size_t len) {
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
  T get(const size_t idx) const {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    return atomic::load(&buckets_[bucket_idx])[bucket_off];
  }

  T& operator[](const size_t idx) {
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
  void get(T* data, const size_t idx, const size_t len) const {
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
class __monolog_linear_base {
 public:
  typedef std::atomic<T*> __atomic_bucket_ref;
  static const size_t BUFFER_SIZE = 1024;  // 1KB buffer size

  __monolog_linear_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      atomic::init(&x, null_ptr);
    }
    atomic::init(&buckets_[0], new T[BLOCK_SIZE + BUFFER_SIZE]);
  }

  ~__monolog_linear_base() {
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
  void set(size_t idx, const T val) {
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
  void write(const size_t offset, const T* data, const size_t len) {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    T* bucket;
    if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
      bucket = try_allocate_bucket(bucket_idx);
    }
    memcpy(bucket + bucket_off, data, len);
  }

  // Write len bytes of data at offset. Does NOT allocate memory -- ensure
  // memory is allocated before calling this function.
  void write_unsafe(const size_t offset, const T* data, const size_t len) {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    memcpy(atomic::load(&buckets_[bucket_idx]) + bucket_off, data, len);
  }

  // Gets the data at index idx.
  T get(const size_t idx) const {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    return atomic::load(&buckets_[bucket_idx])[bucket_off];
  }

  // Get len bytes of data at offset.
  void read(const size_t offset, T* data, const size_t len) const {
    size_t bucket_idx = offset / BLOCK_SIZE;
    size_t bucket_off = offset % BLOCK_SIZE;
    memcpy(data, atomic::load(&buckets_[bucket_idx]) + bucket_off, len);
  }

  T& operator[](const size_t idx) {
    size_t bucket_idx = idx / BLOCK_SIZE;
    size_t bucket_off = idx % BLOCK_SIZE;
    T* bucket;
    if ((bucket = atomic::load(&buckets_[bucket_idx])) == NULL) {
      bucket = try_allocate_bucket(bucket_idx);
    }
    return bucket[bucket_off];
  }

  void* ptr(const size_t offset) {
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
    T* bucket = new T[BLOCK_SIZE + BUFFER_SIZE];
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
    size_t idx = atomic::faa(&write_tail_, 1UL);
    this->set(idx, val);

    size_t expected = idx;
    while (!atomic::weak::cas(&read_tail_, &expected, idx + 1))
      expected = idx;
    return idx;
  }

  size_t push_back_range(const T start, const T end) {
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
  T at(const size_t idx) const {
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
    size_t idx = atomic::faa(&tail_, 1UL);
    this->set(idx, val);
    return idx;
  }

  size_t push_back_range(const T start, const T end) {
    size_t cnt = (end - start + 1);
    size_t idx = atomic::faa(&tail_, cnt);
    for (size_t i = 0; i < cnt; i++)
      this->set(idx + i, start + i);
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
  std::atomic<size_t> tail_;
};

template<class T, size_t NBUCKETS = 32, size_t BLOCK_SIZE = 1073741824UL>
class monolog_relaxed_linear : public __monolog_linear_base<T, NBUCKETS,
    BLOCK_SIZE> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<monolog_relaxed<T, NBUCKETS>> iterator;

  monolog_relaxed_linear()
      : tail_(0) {
  }

  size_t push_back(const T val) {
    size_t idx = atomic::faa(&tail_, 1UL);
    this->set(idx, val);
    return idx;
  }

  size_t push_back_range(const T start, const T end) {
    size_t cnt = (end - start + 1);
    size_t idx = atomic::faa(&tail_, cnt);
    for (size_t i = 0; i < cnt; i++)
      this->set(idx + i, start + i);
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
  std::atomic<size_t> tail_;
};

}

#endif /* MONOLOG_MONOLOG_H_ */
