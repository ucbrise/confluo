#ifndef SLOG_FACLOG_H_
#define SLOG_FACLOG_H_

#include <array>
#include <vector>
#include <atomic>
#include <fstream>

#include "utils.h"

namespace slog {

/**
 * The base class for Monotonic Log (MonoLog).
 *
 * Implements get/set/multiget/multiset functionalities,
 * but does not maintain read or write tails and does not
 * provide any atomicity/consistency guarantees by itself.
 *
 */
template<class T, uint32_t NBUCKETS = 32>
class __monolog_base {
 public:
  static const uint32_t FBS = 16;
  static const uint32_t FBS_HIBIT = 4;

  typedef std::atomic<T*> __atomic_bucket_ref;

  __monolog_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      x.store(null_ptr);
    }
    buckets_[0].store(new T[FBS], std::memory_order_release);
  }

  ~__monolog_base() {
    for (auto& x : buckets_) {
      delete[] x.load(std::memory_order_acquire);
    }
  }

  void ensure_alloc(uint32_t start_idx, uint32_t end_idx) {
    uint32_t pos1 = start_idx + FBS;
    uint32_t pos2 = end_idx + FBS;
    uint32_t hibit1 = bit_utils::highest_bit(pos1);
    uint32_t hibit2 = bit_utils::highest_bit(pos2);
    uint32_t bucket_idx1 = hibit1 - FBS_HIBIT;
    uint32_t bucket_idx2 = hibit2 - FBS_HIBIT;
    for (uint32_t i = bucket_idx1; i <= bucket_idx2; i++) {
      if (buckets_[i].load(std::memory_order_acquire) == NULL) {
        try_allocate_bucket(i);
      }
    }
  }

  // Sets the data at index idx to val. Allocates memory if necessary.
  void set(uint32_t idx, const T val) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off] = val;
  }

  // Sets a contiguous region of the MonoLog base to the provided data.
  void set(uint32_t idx, const T* data, const uint32_t len) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    uint32_t data_remaining = len * sizeof(T);
    uint32_t data_off = 0;
    while (data_remaining) {
      if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
        try_allocate_bucket(bucket_idx);
      }
      uint32_t bucket_remaining =
          ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off) * sizeof(T);
      uint32_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      memcpy(buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
             data + data_off, bytes_to_write);
      bucket_idx++;
      bucket_off = 0;
    }
  }

  // Gets the data at index idx.
  T get(const uint32_t idx) const {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off];
  }

  T& operator[](const uint32_t idx) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off];
  }

  // Copies a contiguous region of the MonoLog base into the provided buffer.
  // The buffer should have sufficient space to hold the data requested, otherwise
  // undefined behavior may result.
  void get(T* data, const uint32_t idx, const uint32_t len) const {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    uint32_t data_remaining = len * sizeof(T);
    uint32_t data_off = 0;
    while (data_remaining) {
      uint32_t bucket_remaining =
          ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off) * sizeof(T);
      uint32_t bytes_to_read = std::min(bucket_remaining, data_remaining);
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
    for (uint32_t i = 0; i < buckets_.size(); i++) {
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
  uint32_t try_allocate_bucket(uint32_t bucket_idx) {
    uint32_t size = (1U << (bucket_idx + FBS_HIBIT));
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

template<class T, uint32_t NBUCKETS = 1024, uint32_t BLOCK_SIZE = 268435456U>
class __monolog_linear_base {
 public:
  typedef std::atomic<T*> __atomic_bucket_ref;

  __monolog_linear_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      x = null_ptr;
    }
    buckets_[0] = new T[BLOCK_SIZE];
  }

  ~__monolog_linear_base() {
    for (auto& x : buckets_) {
      delete[] x.load();
    }
  }

  // Write len bytes of data at offset.
  // Allocates memory if necessary.
  void write(const uint64_t offset, const T* data, const uint32_t len) {
    uint32_t bucket_idx = offset / BLOCK_SIZE;
    uint32_t bucket_off = offset % BLOCK_SIZE;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    memcpy(buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
           data, len);
  }

  // Get len bytes of data at offset.
  void read(const uint64_t offset, T* data, const uint32_t len) const {
    uint32_t bucket_idx = offset / BLOCK_SIZE;
    uint32_t bucket_off = offset % BLOCK_SIZE;
    memcpy(data,
           buckets_[bucket_idx].load(std::memory_order_acquire) + bucket_off,
           len);
  }

  size_t storage_size() const {
    size_t bucket_size = buckets_.size() * sizeof(__atomic_bucket_ref );
    size_t data_size = 0;
    for (uint32_t i = 0; i < buckets_.size(); i++) {
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
  void try_allocate_bucket(uint32_t bucket_idx) {
    T* bucket = new T[BLOCK_SIZE];
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

template<class T, uint32_t NBUCKETS = 32>
class __atomic_monolog_base {

  static_assert(std::is_fundamental<T>::value, "Type for atomic monolog must be primitive.");
 public:
  static const uint32_t FBS = 16;
  static const uint32_t FBS_HIBIT = 4;

  typedef std::atomic<T> __atomic_ref;
  typedef std::atomic<__atomic_ref *> __atomic_bucket_ref;

  __atomic_monolog_base() {
    __atomic_ref* null_ptr = NULL;
    for (auto& x : buckets_) {
      x = null_ptr;
    }
    buckets_[0].store(new __atomic_ref[FBS]);
  }

  ~__atomic_monolog_base() {
    for (auto& x : buckets_) {
      delete[] x.load();
    }
  }

  void alloc(uint32_t idx) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
  }

  void set(uint32_t idx, const T val) {
    store(idx, val);
  }

  // Atomically store value at index idx.
  // Allocates memory if necessary.
  void store(uint32_t idx, const T val) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off].store(val);
  }

  __atomic_ref& operator[](const uint32_t idx) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx].load(std::memory_order_acquire) == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off].load();
  }

  T get(const uint32_t idx) const {
    return load(idx);
  }

  // Atomically loads the data at index idx.
  T load(const uint32_t idx) const {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off].load();
  }

  bool cas(const uint32_t idx, T& expected, T replacement) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx].load(std::memory_order_acquire)[bucket_off].compare_exchange_strong(expected,
        replacement);
  }

protected:
  // Tries to allocate the specifies bucket. If another thread has already
  // succeeded in allocating the bucket, the current thread deallocates and
  // returns.
  uint32_t try_allocate_bucket(uint32_t bucket_idx) {
    uint32_t size = (1U << (bucket_idx + FBS_HIBIT));
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

  __atomic_ref* new_bucket(uint32_t size) {
    __atomic_ref* bucket = new __atomic_ref[size];
    for(uint32_t i = 0; i < size; i++) {
      bucket[i].store(T(0));
    }
    return bucket;
  }

  std::array<__atomic_bucket_ref, NBUCKETS> buckets_;  // Stores the pointers to the buckets for MonoLog.
};

/**
 * Linearizable implementation for the MonoLog.
 *
 * Maintains a read and write tail to ensure:
 * - Read operations can only access data for completed writes; conversely,
 *   read operations can access data for all completed writes.
 * - Completion times for write operations are strictly ordered by their
 *   start times.
 */
template<class T, uint32_t NBUCKETS = 32>
class monolog_linearizable : public __monolog_base<T, NBUCKETS> {
 public:
  monolog_linearizable()
      : write_tail_(0),
        read_tail_(0) {
  }

  // Append an entry at the end of the MonoLog
  uint32_t push_back(const T val) {
    uint32_t idx = std::atomic_fetch_add(&write_tail_, 1U);
    this->set(idx, val);
    while (!std::atomic_compare_exchange_strong(&read_tail_, &idx, idx + 1))
      ;
    return idx;
  }

  // Get the entry at the specified index `idx'.
  T at(const uint32_t idx) const {
    return this->get(idx);
  }

  // Get the size of the MonoLog (i.e., number of completely written entries)
  uint32_t size() const {
    return read_tail_.load();
  }

 private:
  std::atomic<uint32_t> write_tail_;
  std::atomic<uint32_t> read_tail_;
};

/**
 * Relaxed (i.e., not linearizable) implementation for the MonoLog.
 *
 * Maintains a single tail that ensures:
 * - Write operations are atomic
 */
template<class T, uint32_t NBUCKETS = 32>
class monolog_relaxed : public __monolog_base<T, NBUCKETS> {
 public:
  monolog_relaxed()
      : tail_(0) {
  }

  uint32_t push_back(const T val) {
    uint32_t idx = tail_.fetch_add(1U, std::memory_order_release);
    this->set(idx, val);
    return idx;
  }

  T at(const uint32_t idx) const {
    return this->get(idx);
  }

  uint32_t size() const {
    return tail_.load(std::memory_order_acquire);
  }

 private:
  std::atomic<uint32_t> tail_;
};

}

#endif /* SLOG_MONOLOG_H_ */
