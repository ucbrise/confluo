#ifndef MONOLOG_MONOLOG_EXP2_LINEAR_H_
#define MONOLOG_MONOLOG_EXP2_LINEAR_H_

#include <array>
#include <vector>

#include "atomic.h"
#include "bit_utils.h"
#include "dialog_allocator.h"
#include "ptr.h"

using namespace utils;

namespace confluo {
namespace monolog {

/**
 * The base class for MonoLog.
 *
 * Implements get/set/multiget/multiset functionalities,
 * but does not maintain read or write tails and does not
 * provide any atomicity/consistency guarantees by itself.
 *
 */
template<class T, size_t NCONTAINERS = 32, size_t BUCKET_SIZE = 1024>
class monolog_exp2_linear_base {
 public:
  typedef storage::swappable_ptr<T> __atomic_bucket_ref;
  typedef storage::read_only_ptr<T> __atomic_bucket_copy_ref;
  typedef atomic::type<__atomic_bucket_ref*> __atomic_bucket_container_ref;

  monolog_exp2_linear_base()
      : fcs_hibit_(bit_utils::highest_bit(FCS)) {
    __atomic_bucket_ref* null_ptr = nullptr;
    for (auto& x : bucket_containers_) {
      atomic::init(&x, null_ptr);
    }

    T* first_bucket_data = ALLOCATOR.alloc<T>(BUCKET_SIZE);
    memset(first_bucket_data, 0xFF, BUCKET_SIZE * sizeof(T));

    __atomic_bucket_ref* first_container = new __atomic_bucket_ref[FCB]();
    first_container[0].atomic_init(first_bucket_data);
    atomic::init(&bucket_containers_[0], first_container);
  }

  ~monolog_exp2_linear_base() {
    size_t num_buckets = FCB;
    for (auto& x : bucket_containers_) {
      __atomic_bucket_ref* container = atomic::load(&x);
      if (container != nullptr) {
        delete[] container;
      }
      num_buckets = num_buckets << 1U;
    }
  }

  /**
   * Ensures containers are allocated to cover the range of indexes given.
   * @param start_idx start index
   * @param end_idx end index
   */
  void ensure_alloc(size_t start_idx, size_t end_idx) {
    size_t pos1 = start_idx + FCS;
    size_t pos2 = end_idx + FCS;
    size_t hibit1 = bit_utils::highest_bit(pos1);
    size_t hibit2 = bit_utils::highest_bit(pos2);
    size_t highest_cleared1 = pos1 ^ (1 << hibit1);
    size_t highest_cleared2 = pos2 ^ (1 << hibit2);
    size_t bucket_idx1 = highest_cleared1 / BUCKET_SIZE;
    size_t bucket_idx2 = highest_cleared2 / BUCKET_SIZE;
    size_t container_idx1 = hibit1 - fcs_hibit_;
    size_t container_idx2 = hibit2 - fcs_hibit_;

    for (size_t i = container_idx1; i <= container_idx2; i++) {
      __atomic_bucket_ref* container = atomic::load(&bucket_containers_[i]);
      if (container == nullptr) {
        try_allocate_container(i);
      }
    }
  }

  /**
   * Sets the data at index idx to val. Allocates memory if necessary.
   * Assumes no contention between writers and archiver.
   * @param idx index to set at
   * @param val value to set
   */
  void set(size_t idx, const T val) {
    size_t pos = idx + FCS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    __atomic_bucket_ref* container = atomic::load(&bucket_containers_[container_idx]);
    if (container == nullptr) {
      container = try_allocate_container(container_idx);
    }

    T* ptr = container[bucket_idx].atomic_load();
    if (ptr == nullptr) {
      ptr = try_allocate_bucket(container, bucket_idx);
    }
    ptr[bucket_off] = val;
  }

  /**
   * Sets the data at index idx to val. Does NOT allocate memory --
   * ensure memory is allocated before calling this function.
   * Assumes no contention between writers and archiver.
   * @param idx index to set at
   * @param val value to set
   */
  void set_unsafe(size_t idx, const T val) {
    size_t pos = idx + FCS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    __atomic_bucket_ref* container = atomic::load(&bucket_containers_[container_idx]);
    container[bucket_idx].atomic_load()[bucket_idx] = val;
  }

  /**
   * Sets a contiguous region of the MonoLog base to the provided data.
   * Assumes no contention between writers and archiver.
   * @param idx monolog index
   * @param data data to set
   * @param len length of data
   */
  void set(size_t idx, const T* data, size_t len) {
    size_t pos = idx + FCS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    size_t bucket_remaining = BUCKET_SIZE * sizeof(T);
    while (data_remaining) {
      __atomic_bucket_ref* container = atomic::load(&bucket_containers_[container_idx]);
      if (container == nullptr) {
        container = try_allocate_container(container_idx);
      }

      T* bucket = container[bucket_idx].atomic_load();
      if (bucket == nullptr) {
        bucket = try_allocate_bucket(container, bucket_idx);
      }

      size_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      memcpy(&bucket[bucket_off], data + data_off, bytes_to_write);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      bucket_idx++;
      if (bucket_idx >= (1U << (container_idx + FCB_HIBIT))) {
        container_idx++;
        bucket_idx = 0;
      }
      bucket_off = 0;
    }
  }

  /**
   * Sets a contiguous region of the MonoLog base to the provided data. Does
   * NOT allocate memory -- ensure memory is allocated before calling this function.
   * Assumes no contention between writers and archiver.
   * @param idx monolog index
   * @param data data to set
   * @param len length of data
   */
  void set_unsafe(size_t idx, const T* data, size_t len) {
    size_t pos = idx + FCS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    size_t bucket_remaining = BUCKET_SIZE * sizeof(T);
    while (data_remaining) {
      size_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      __atomic_bucket_ref* container = atomic::load(&bucket_containers_[container_idx]);
      T* bucket = container[bucket_idx].atomic_load();
      memcpy(&bucket[bucket_off], data + data_off, bytes_to_write);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      bucket_idx++;
      if (bucket_idx >= (1U << (container_idx + FCB_HIBIT))) {
        container_idx++;
        bucket_idx = 0;
      }
      bucket_off = 0;
    }
  }

  /**
   * Gets a pointer to the data at index idx
   * @param idx monolog index
   * param data_ptr location to store pointer to data at
   */
   void ptr(size_t idx, __atomic_bucket_copy_ref& data_ptr) const {
    size_t pos = idx + FCS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    load_bucket_copy(container_idx, bucket_idx, data_ptr);
   }

  /**
   * Gets the data at index idx.
   * @param idx index
   * @return data
   */
  const T get(size_t idx) const {
    size_t pos = idx + FCS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;
    return atomic::load(&bucket_containers_[container_idx])[bucket_idx].atomic_get(bucket_off);
  }

  T& operator[](size_t idx) {
    size_t pos = idx + FCS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    __atomic_bucket_ref* container = atomic::load(&bucket_containers_[container_idx]);
    if (container == nullptr) {
      container = try_allocate_container(container_idx);
    }
    __atomic_bucket_copy_ref bucket;
    container[bucket_idx].atomic_copy(bucket);
    if (bucket.get() == nullptr) {
      try_allocate_bucket(container, bucket_idx, bucket);
    }
    return bucket.get()[bucket_off];
  }

  /**
   * Copies a contiguous region of the MonoLog base into the provided buffer.
   * The buffer should have sufficient space to hold the data requested, otherwise
   * undefined behavior may result.
   * @param data buffer to read into
   * @param idx start index
   * @param len bytes to read
   */
  void get(T* data, size_t idx, size_t len) const {
    size_t pos = idx + FCS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    size_t data_remaining = len * sizeof(T);
    size_t data_off = 0;
    size_t bucket_remaining = BUCKET_SIZE * sizeof(T);
    while (data_remaining) {
      size_t bytes_to_read = std::min(bucket_remaining, data_remaining);
      __atomic_bucket_copy_ref bucket;
      load_bucket_copy(container_idx, bucket_idx, bucket);
      memcpy(data + data_off, &bucket.get()[bucket_off], bytes_to_read);
      data_remaining -= bytes_to_read;
      data_off += bytes_to_read;
      bucket_idx++;
      if (bucket_idx >= (1U << (container_idx + FCB_HIBIT))) {
        container_idx++;
        bucket_idx = 0;
      }
      bucket_off = 0;
    }
  }

  /**
   * @return storage size of the monolog
   */
  size_t storage_size() const {
    size_t container_size = bucket_containers_.size() * sizeof(__atomic_bucket_container_ref);
    size_t bucket_size = 0;
    size_t data_size = 0;
    size_t num_buckets = FCB;
    for (size_t i = 0; i < bucket_containers_.size(); i++) {
      __atomic_bucket_ref* container = atomic::load(&bucket_containers_[i]);
      if (container != nullptr) {
        bucket_size += num_buckets * sizeof(__atomic_bucket_ref);
        for (size_t j = 0; j < num_buckets; j++) {
          __atomic_bucket_copy_ref* bucket = container[j].atomic_copy();
          if (bucket->get() != nullptr) {
            data_size += BUCKET_SIZE * sizeof(T);
          }
        }
      }
      num_buckets = num_buckets << 1U;
    }
    return container_size + bucket_size + data_size;
  }

protected:
  /**
   * Tries to allocate the specified container. If another thread already succeeded
   * in allocating the container, the current thread deallocates and returns.
   * @param container_idx index into bucket containers to allocate container at
   * @return allocated container
   */
  __atomic_bucket_ref* try_allocate_container(size_t container_idx) {
    size_t num_buckets = 1U << (container_idx + FCB_HIBIT);
    __atomic_bucket_ref* new_container = new __atomic_bucket_ref[num_buckets]();
    __atomic_bucket_ref* expected = nullptr;

    // Only one thread will be successful in replacing the nullptr reference with newly
    // allocated container.
    if (!atomic::strong::cas(&bucket_containers_[container_idx], &expected, new_container)) {
      // All other threads will deallocate the newly allocated container.
      delete[] new_container;
      return expected;
    }
    return new_container;
  }

  /**
   * Tries to allocate a bucket. If another thread already succeeded in allocating
   * the bucket, the current thread deallocates.
   * @param container container to put bucket pointer in
   * @param bucket_idx index into the container to allocate bucket at
   * @param copy copy of pointer to allocated bucket
   */
  void try_allocate_bucket(__atomic_bucket_ref* container, size_t bucket_idx,
                           __atomic_bucket_copy_ref& copy) {
    T* new_bucket_data = ALLOCATOR.alloc<T>(BUCKET_SIZE);
    memset(new_bucket_data, 0xFF, BUCKET_SIZE * sizeof(T));
    if (!container[bucket_idx].atomic_init(new_bucket_data)) {
      ALLOCATOR.dealloc<T>(new_bucket_data);
    }
    container[bucket_idx].atomic_copy(copy);
  }

  T* try_allocate_bucket(__atomic_bucket_ref* container, size_t bucket_idx) {
    T* new_bucket_data = ALLOCATOR.alloc<T>(BUCKET_SIZE);
    memset(new_bucket_data, 0xFF, BUCKET_SIZE * sizeof(T));
    if (!container[bucket_idx].atomic_init(new_bucket_data)) {
      ALLOCATOR.dealloc<T>(new_bucket_data);
      return container[bucket_idx].atomic_load();
    }
    return new_bucket_data;
  }

  /**
   * Load read-only copy of a pointer
   * @param container_idx container index
   * @param bucket_idx bucket index
   * @param copy read-only pointer to store in
   * @param bucket_offset bucket offset to offset the copy's internal pointer
   */
  void load_bucket_copy(size_t container_idx, size_t bucket_idx, __atomic_bucket_copy_ref& copy,
                        size_t bucket_offset = 0) const {
    atomic::load(&bucket_containers_[container_idx])[bucket_idx].atomic_copy(copy, bucket_offset);
  }

private:
  static const size_t FCB = 16;
  static const size_t FCB_HIBIT = 4;
  static const size_t FCS = FCB * BUCKET_SIZE;
  const size_t fcs_hibit_;

  // Stores the pointers to the bucket containers for MonoLog.
  std::array<__atomic_bucket_container_ref, NCONTAINERS> bucket_containers_;
};

/**
 * Relaxed (i.e., not linearizable) implementation for the MonoLog.
 *
 * Maintains a single tail that ensures:
 * - Write operations are atomic
 */
template<class T, size_t NCONTAINERS = 32, size_t BUCKET_SIZE = 1024>
class monolog_exp2_linear : public monolog_exp2_linear_base<T, NCONTAINERS,
    BUCKET_SIZE> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<monolog_exp2_linear<T, NCONTAINERS>> iterator;
  typedef monolog_iterator<monolog_exp2_linear<T, NCONTAINERS>> const_iterator;

  monolog_exp2_linear()
      : tail_(0) {
  }

  size_t reserve(size_t count) {
    return atomic::faa(&tail_, count);
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

  const T at(size_t idx) const {
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

#endif /* MONOLOG_MONOLOG_EXP2_LINEAR_H_ */
