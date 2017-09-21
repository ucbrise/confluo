#ifndef MONOLOG_MONOLOG_EXP2_LINEAR_H_
#define MONOLOG_MONOLOG_EXP2_LINEAR_H_

#include <array>
#include <vector>

#include "atomic.h"
#include "mempool.h"
#include "bit_utils.h"

using namespace utils;

namespace dialog {
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
  typedef atomic::type<T*> __atomic_bucket_ref;
  typedef atomic::type<__atomic_bucket_ref*> __atomic_bucket_container_ref;

  monolog_exp2_linear_base()
      : fcs_hibit_(bit_utils::highest_bit(fcs_)) {
    __atomic_bucket_ref* null_ptr = nullptr;
    for (auto& x : bucket_containers_) {
      atomic::init(&x, null_ptr);
    }

    __atomic_bucket_ref* first_container = new __atomic_bucket_ref[FCB]();
    T* first_bucket = BUCKET_POOL.alloc();
    memset(first_bucket, 0xFF, BUCKET_SIZE * sizeof(T));

    atomic::init(&first_container[0], first_bucket);
    atomic::init(&bucket_containers_[0], first_container);
  }

  ~monolog_exp2_linear_base() {
    size_t num_buckets = FCB;
    for (auto& x : bucket_containers_) {
      __atomic_bucket_ref* container = atomic::load(&x);
      if (container != nullptr) {
        for (size_t i = 0; i < num_buckets; i++) {
          if (container[i] != nullptr) {
            delete[] atomic::load(&container[i]);
          }
        }
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
    size_t pos1 = start_idx + fcs_;
    size_t pos2 = end_idx + fcs_;
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
   * @param idx index to set at
   * @param val value to set
   */
  void set(size_t idx, const T val) {
    size_t pos = idx + fcs_;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    __atomic_bucket_ref* container = atomic::load(&bucket_containers_[container_idx]);
    if (container == nullptr) {
      container = try_allocate_container(container_idx);
    }
    T* bucket = atomic::load(&container[bucket_idx]);
    if (bucket == nullptr) {
      bucket = try_allocate_bucket(container, bucket_idx);
    }
    bucket[bucket_off] = val;
  }

  /**
   * Sets the data at index idx to val. Does NOT allocate memory --
   * ensure memory is allocated before calling this function.
   * @param idx index to set at
   * @param val value to set
   */
  void set_unsafe(size_t idx, const T val) {
    size_t pos = idx + fcs_;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;
    atomic::load(&(atomic::load(&bucket_containers_[container_idx])[bucket_idx]))[bucket_off] = val;
  }

  /**
   * Sets a contiguous region of the MonoLog base to the provided data.
   * @param idx monolog index
   * @param data data to set
   * @param len length of data
   */
  void set(size_t idx, const T* data, size_t len) {
    size_t pos = idx + fcs_;
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
      T* bucket = atomic::load(&container[bucket_idx]);
      if (bucket == nullptr) {
        bucket = try_allocate_bucket(container, bucket_idx);
      }

      size_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      memcpy(&bucket[bucket_off], data + data_off, bytes_to_write);
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
   * @param idx monolog index
   * @param data data to set
   * @param len length of data
   */
  void set_unsafe(size_t idx, const T* data, size_t len) {
    size_t pos = idx + fcs_;
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
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      T* bucket = atomic::load(&atomic::load(&bucket_containers_[container_idx])[bucket_idx]);
      memcpy(&bucket[bucket_off], data + data_off, bytes_to_write);
      bucket_idx++;
      if (bucket_idx >= (1U << (container_idx + FCB_HIBIT))) {
        container_idx++;
        bucket_idx = 0;
      }
      bucket_off = 0;
    }
  }

  /**
   * Gets the pointer to the data at index idx
   * @param idx monolog index
   * @return pointer
   */
  const T* ptr(size_t idx) const {
    size_t pos = idx + fcs_;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;
    return atomic::load(&atomic::load(&bucket_containers_[container_idx])[bucket_idx]) + bucket_off;
  }

  /**
   * Gets the data at index idx.
   * @param idx index
   * @return data
   */
  const T& get(size_t idx) const {
    size_t pos = idx + fcs_;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;
    return atomic::load(&atomic::load(&bucket_containers_[container_idx])[bucket_idx])[bucket_off];
  }

  T& operator[](size_t idx) {
    size_t pos = idx + fcs_;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t highest_cleared = pos ^ (1 << hibit);
    size_t bucket_idx = highest_cleared / BUCKET_SIZE;
    size_t bucket_off = highest_cleared % BUCKET_SIZE;
    size_t container_idx = hibit - fcs_hibit_;

    __atomic_bucket_ref* container = atomic::load(&bucket_containers_[container_idx]);
    if (container == nullptr) {
      container = try_allocate_container(container_idx);
    }
    T* bucket = atomic::load(&container[bucket_idx]);
    if (bucket == nullptr) {
      bucket = try_allocate_bucket(container, bucket_idx);
    }
    return bucket[bucket_off];
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
    size_t pos = idx + fcs_;
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
      data_remaining -= bytes_to_read;
      data_off += bytes_to_read;
      T* bucket = atomic::load(&atomic::load(&bucket_containers_[container_idx])[bucket_idx]);
      memcpy(data + data_off, &bucket[bucket_off], bytes_to_read);
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
          T* bucket = atomic::load(&container[j]);
          if (bucket != nullptr) {
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
   * Allocates a bucket.
   * @param container container to put bucket pointer in
   * @param bucket_idx index into the container to allocate bucket at
   * @return allocated bucket
   */
  T* try_allocate_bucket(__atomic_bucket_ref* container, size_t bucket_idx) {
    T* new_bucket = BUCKET_POOL.alloc();
    T* expected = nullptr;

    if (!atomic::strong::cas(&container[bucket_idx], &expected, new_bucket)) {
      BUCKET_POOL.dealloc(new_bucket);
      return expected;
    }
    memset(new_bucket, 0xFF, BUCKET_SIZE * sizeof(T));
    return new_bucket;
  }

 private:
  static const size_t FCB = 16;
  static const size_t FCB_HIBIT = 4;
  const size_t fcs_ = FCB * BUCKET_SIZE;
  const size_t fcs_hibit_;

  static mempool<T, BUCKET_SIZE * sizeof(T)> BUCKET_POOL;

  // Stores the pointers to the bucket containers for MonoLog.
  std::array<__atomic_bucket_container_ref, NCONTAINERS> bucket_containers_;
};

template<typename T, size_t NCONTAINERS, size_t BUCKET_SIZE>
mempool<T, BUCKET_SIZE * sizeof(T)> monolog_exp2_linear_base<T, NCONTAINERS, BUCKET_SIZE>::BUCKET_POOL;

/**
 * Relaxed (i.e., not linearizable) implementation for the MonoLog.
 *
 * Maintains a single tail that ensures:
 * - Write operations are atomic
 */
template<class T, size_t NCONTAINERS = 32, size_t BUCKET_SIZE = 1024>
class monolog_exp2_linear : public monolog_exp2_linear_base<T, NCONTAINERS, BUCKET_SIZE> {
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

#endif /* MONOLOG_MONOLOG_EXP2_LINEAR_H_ */
