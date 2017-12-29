#ifndef CONFLUO_CONTAINER_MONOLOG_MONOLOG_EXP2_H_
#define CONFLUO_CONTAINER_MONOLOG_MONOLOG_EXP2_H_

#include <array>
#include <vector>

#include "atomic.h"
#include "bit_utils.h"

// TODO: Improve documentation

using namespace utils;

namespace confluo {
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

  monolog_iterator()
      : impl_(nullptr),
        pos_(0) {
  }

  monolog_iterator(const monolog_impl* impl, size_t pos)
      : impl_(impl),
        pos_(pos) {
  }

  reference operator*() const {
    return impl_->get(pos_);
  }

  pointer operator->() const {
    return impl_->ptr(pos_);
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

  monolog_iterator& operator=(const monolog_iterator& other) {
    impl_ = other.impl_;
    pos_ = other.pos_;
    return *this;
  }

 private:
  const monolog_impl* impl_;
  size_t pos_;
};

/**
 * @class
 * The base class for MonoLog.
 *
 * Implements get/set/multiget/multiset functionalities,
 * but does not maintain read or write tails and does not
 * provide any atomicity/consistency guarantees by itself.
 *
 */
template<class T, size_t NBUCKETS = 32>
class monolog_exp2_base {
 public:
  static const size_t FBS = 16;
  static const size_t FBS_HIBIT = 4;

  typedef atomic::type<T*> __atomic_bucket_ref;

  /**
   * The default constructor that initializes all of the buckets
   * for the monolog
   */
  monolog_exp2_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_) {
      atomic::init(&x, null_ptr);
    }
    atomic::init(&buckets_[0], new T[FBS]);
  }

  /**
   * The default destructor that deletes all of the buckets
   */
  ~monolog_exp2_base() {
    for (auto& x : buckets_) {
      delete[] atomic::load(&x);
    }
  }

  /**
   * Allocates space for buckets between the indices
   * @param start_idx The start index
   * @param end_idx The end index
   */
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

  /**
   * Sets the data at index idx to val. Allocates memory if necessary.
   * @param idx The specified index
   * @param val The data to be set at the index
   */
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

  /**
   * Sets the data at index idx to val. Does NOT allocate memory -- ensure
   * memory is allocated before calling this function.
   * @param idx The specified index
   * @param val The data to be set at the index
   */
  void set_unsafe(size_t idx, const T val) {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    atomic::load(&buckets_[bucket_idx])[bucket_off] = val;
  }

  /** Sets a contiguous region of the MonoLog base to the provided data.
   * @param idx The specified index
   * @param data The data that needs to be stored
   * @param len The length of the region
   */
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
      memcpy(bucket + bucket_off, data + data_off, bytes_to_write);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      bucket_idx++;
      bucket_off = 0;
    }
  }

  /** Sets a contiguous region of the MonoLog base to the provided data.
   * Does NOT allocate memory -- ensure memory is allocated before
   * calling this function.
   *
   * @param idx The specified index
   * @param data The data to be stored
   * @param len The length of the region
   */
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
      memcpy(atomic::load(&buckets_[bucket_idx]) + bucket_off, data + data_off,
             bytes_to_write);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      bucket_idx++;
      bucket_off = 0;
    }
  }

  /**
   * Gets the pointer to the data specified by the index
   * @param idx The index of where to get the data
   * @return The pointer to the region
   */
  const T* ptr(size_t idx) const {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    return atomic::load(&buckets_[bucket_idx]) + bucket_off;
  }

  /** Gets the data at index idx.
   * @param idx The index of where to get the data
   * @return A reference to the data at that index
   */
  const T& get(size_t idx) const {
    size_t pos = idx + FBS;
    size_t hibit = bit_utils::highest_bit(pos);
    size_t bucket_off = pos ^ (1 << hibit);
    size_t bucket_idx = hibit - FBS_HIBIT;
    return atomic::load(&buckets_[bucket_idx])[bucket_off];
  }

  /**
   * Accesses the data at a specific index
   * @param idx The index of what data to get
   * @return A reference to the data at the index
   */
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

  /** Copies a contiguous region of the MonoLog base into the provided 
   * buffer.The buffer should have sufficient space to hold the 
   * data requested, otherwise undefined behavior may result.
   * @param data The data to be copied
   * @param idx The index for where the data is copied to
   * @param len The length of the buffer
   */
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
      memcpy(data + data_off, atomic::load(&buckets_[bucket_idx]) + bucket_off,
             bytes_to_read);
      data_remaining -= bytes_to_read;
      data_off += bytes_to_read;
      bucket_idx++;
      bucket_off = 0;
    }
  }

  /**
   * Gets the size of the storage space
   * @return The size of storage in bytes
   */
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
  /* Tries to allocate the specifies bucket. If another thread has 
   * already succeeded in allocating the bucket, the current thread 
   * deallocates and returns.
   * @param bucket_idx The index of the specified bucket
   * @return The specified bucket
   */
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

/**
 * Relaxed (i.e., not linearizable) implementation for the MonoLog.
 *
 * Maintains a single tail that ensures:
 * - Write operations are atomic
 */
template<class T, size_t NBUCKETS = 32>
class monolog_exp2 : public monolog_exp2_base<T, NBUCKETS> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<monolog_exp2<T, NBUCKETS>> iterator;
  typedef monolog_iterator<monolog_exp2<T, NBUCKETS>> const_iterator;

  monolog_exp2()
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

#endif /* CONFLUO_CONTAINER_MONOLOG_MONOLOG_EXP2_H_ */
