#ifndef SLOG_FACLOG_H_
#define SLOG_FACLOG_H_

#include <array>
#include <vector>
#include <atomic>
#include <fstream>

#include "utils.h"

namespace slog {

/**
 * The base class for Fast Append-only Concurrent Log (FACLog).
 *
 * Implements get/set/multiget/multiset functionalities,
 * but does not maintain read or write tails and does not
 * provide any atomicity/consistency guarantees by itself.
 *
 */
template<class T, uint32_t NBUCKETS = 32>
class __faclog_base {
 public:
  static const uint32_t FBS = 16;
  static const uint32_t FBS_HIBIT = 4;

  typedef std::atomic<T*> __atomic_bucket_ref;

  __faclog_base() {
    T* null_ptr = NULL;
    for (auto& x : buckets_)
      x = null_ptr;
    buckets_[0] = new T[FBS];
  }

  // Sets the data at index idx to val. Allocates memory if necessary.
  void set(uint32_t idx, const T val) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx] == NULL)
      try_allocate_bucket(bucket_idx);
    buckets_[bucket_idx][bucket_off] = val;
  }

  // Sets a contiguous region of the FACLog base to the provided data.
  void set(uint32_t idx, const T* data, const uint32_t len) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    uint32_t data_remaining = len * sizeof(T);
    uint32_t data_off = 0;
    while (data_remaining) {
      if (buckets_[bucket_idx] == NULL)
        try_allocate_bucket(bucket_idx);
      uint32_t bucket_remaining =
          ((1U << (bucket_idx + FBS_HIBIT)) - bucket_off) * sizeof(T);
      uint32_t bytes_to_write = std::min(bucket_remaining, data_remaining);
      data_remaining -= bytes_to_write;
      data_off += bytes_to_write;
      memcpy(buckets_[bucket_idx] + bucket_off, data + data_off,
             bytes_to_write);
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
    return buckets_[bucket_idx][bucket_off];
  }

  T& operator[](const uint32_t idx) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx] == NULL)
      try_allocate_bucket(bucket_idx);
    return buckets_[bucket_idx][bucket_off];
  }

  // Copies a contiguous region of the FACLog base into the provided buffer.
  // The buffer should have sufficient space to hold the data requested, otherwise
  // undefined behavior may result.
  void get(T* data, const uint32_t idx, const uint32_t len) {
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
      memcpy(data + data_off, buckets_[bucket_idx] + bucket_off, bytes_to_read);
      bucket_idx++;
      bucket_off = 0;
    }
  }

  // Serialize first `sz' elements to the provided output stream.
  const uint32_t serialize(std::ostream& out, uint32_t sz) {
    uint32_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&(sz)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    for (uint32_t i = 0; i < sz; i++) {
      T val = get(i);
      out.write(reinterpret_cast<const char *>(&val), sizeof(T));
      out_size += sizeof(T);
    }

    return out_size;
  }

  // Deserialize the FACLog base from the input stream. The second argument is
  // populated to the number of elements read if it is not NULL.
  uint32_t deserialize(std::istream& in, uint32_t *sz) {
    // Read keys
    size_t in_size = 0;

    uint32_t read_size;
    in.read(reinterpret_cast<char *>(read_size), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    for (uint32_t i = 0; i < read_size; i++) {
      uint64_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(T));
      set(i, val);
      in_size += sizeof(T);
    }

    if (sz)
      *sz = read_size;

    return in_size;
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
    if (!std::atomic_compare_exchange_weak(&buckets_[bucket_idx], &null_ptr,
                                           new_bucket)) {
      // All other threads will deallocate the newly allocated bucket.
      delete[] new_bucket;
    }

    return size;
  }

  std::array<__atomic_bucket_ref, NBUCKETS> buckets_;  // Stores the pointers to the buckets for FACLog.
};

/**
 * Strongly consistent (i.e., atomic, linearizable) implementation for the FACLog.
 *
 * Maintains a read and write tail to ensure:
 * - Read operations can only access data for completed writes; conversely,
 *   read operations can access data for all completed writes.
 * - Completion times for write operations are strictly ordered by their
 *   start times.
 */
template<class T, uint32_t NBUCKETS = 32>
class faclog_consistent : public __faclog_base<T, NBUCKETS> {
 public:
  faclog_consistent()
      : write_tail_(0),
        read_tail_(0) {
  }

  // Append an entry at the end of the FACLog
  uint32_t push_back(const T val) {
    uint32_t idx = std::atomic_fetch_add(&write_tail_, 1U);
    this->set(idx, val);
    while (!std::atomic_compare_exchange_weak(&read_tail_, &idx, idx + 1))
      ;
    return idx;
  }

  // Get the entry at the specified index `idx'.
  const T at(const uint32_t idx) {
    return this->get(idx);
  }

  // Get the size of the FACLog (i.e., number of completely written entries)
  const uint32_t size() {
    return read_tail_;
  }

  // Serialize the FACLog to the given output stream.
  const uint32_t serialize(std::ostream& out) {
    return __faclog_base<T, NBUCKETS>::serialize(out, this->size());
  }

  // Deserialize the FACLog from the input stream.
  const uint32_t deserialize(std::istream& in) {
    uint32_t nentries;
    uint32_t in_size = __faclog_base<T, NBUCKETS>::deserialize(in, &nentries);
    write_tail_ = nentries;
    read_tail_ = nentries;
    return in_size;
  }

 private:
  std::atomic<uint32_t> write_tail_;
  std::atomic<uint32_t> read_tail_;
};

/**
 * Eventually consistent (i.e., atomic, but not linearizable) implementation for the FACLog.
 *
 * Maintains a single tail that ensures:
 * - Write operations are atomic
 */
template<class T, uint32_t NBUCKETS = 32>
class faclog_relaxed : public __faclog_base<T, NBUCKETS> {
 public:
  faclog_relaxed()
      : tail_(0) {
  }

  uint32_t push_back(const T val) {
    uint32_t idx = std::atomic_fetch_add(&tail_, 1U);
    this->set(idx, val);
    return idx;
  }

  const T at(const uint32_t idx) {
    return this->get(idx);
  }

  const uint32_t size() {
    return tail_;
  }

  const uint32_t serialize(std::ostream& out) {
    return __faclog_base<T, NBUCKETS>::serialize(out, this->size());
  }

  const uint32_t deserialize(std::istream& in) {
    uint32_t nentries;
    uint32_t in_size = __faclog_base<T, NBUCKETS>::deserialize(in, &nentries);
    tail_ = nentries;
    return in_size;
  }

 private:
  std::atomic<uint32_t> tail_;
};

}

#endif /* SLOG_FACLOG_H_ */
