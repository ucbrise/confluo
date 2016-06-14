#ifndef CONC_VECTORS_H_
#define CONC_VECTORS_H_

#include <array>
#include <vector>
#include <atomic>
#include <fstream>

#include "locks.h"

template<class T>
class ConcurrentVector {
 public:
  ConcurrentVector() {
  }

  uint32_t push_back(const T val) {
    WriteLock write_guard(mtx_);
    uint32_t idx = data_.size();
    data_.push_back(val);
    return idx;
  }

  T at(const uint32_t i) {
    ReadLock read_guard(mtx_);
    return data_.at(i);
  }

  size_t size() {
    ReadLock read_guard(mtx_);
    return data_.size();
  }

  void snapshot(std::vector<T> &out) {
    ReadLock read_guard(mtx_);
    out = data_;
  }

  const uint32_t serialize(std::ostream& out) {
    uint32_t out_size = 0;

    uint32_t sz = size();
    out.write(reinterpret_cast<const char *>(&(sz)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    for (uint32_t i = 0; i < sz; i++) {
      T val = at(i);
      out.write(reinterpret_cast<const char *>(&val), sizeof(T));
      out_size += sizeof(T);
    }

    return out_size;
  }

  uint32_t deserialize(std::istream& in) {
    // Read keys
    size_t in_size = 0;

    uint32_t sz;
    in.read(reinterpret_cast<char *>(&sz), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    for (size_t i = 0; i < sz; i++) {
      uint64_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(T));
      push_back(val);
      in_size += sizeof(T);
    }

    return in_size;
  }

 private:
  std::vector<T> data_;
  Mutex mtx_;
};

static inline uint32_t HighestBit(uint32_t x) {
  uint32_t y = 0;
#ifdef BSR
  asm ( "\tbsr %1, %0\n"
      : "=r"(y)
      : "r" (x)
  );
#else
  while (x >>= 1)
    ++y;
#endif
  return y;
}

template<class T, uint32_t NBUCKETS = 32>
class __LockFreeBase {
 public:
  static const uint32_t FBS = 16;
  static const uint32_t FBS_HIBIT = 4;

  typedef std::atomic<T*> AtomicBucketRef;

  __LockFreeBase() {
    T* null_ptr = NULL;
    for (auto& x : buckets_)
      x = null_ptr;
    buckets_[0] = new T[FBS];
  }

  void try_allocate_bucket(uint32_t bucket_idx) {
    uint32_t size = (1U << (bucket_idx + FBS_HIBIT));
    T* new_bucket = new T[size];
    T* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_weak(&buckets_[bucket_idx], &null_ptr,
                                          new_bucket)) {
      return;
    }

    // All other threads will deallocate the newly allocated bucket.
    delete[] new_bucket;
  }

  void set(uint32_t idx, const T val) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = HighestBit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx] == NULL)
      try_allocate_bucket(bucket_idx);
    buckets_[bucket_idx][bucket_off] = val;
  }

  T get(const uint32_t idx) const {
    uint32_t pos = idx + FBS;
    uint32_t hibit = HighestBit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx][bucket_off];
  }

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

  uint32_t deserialize(std::istream& in, uint32_t *sz) {
    // Read keys
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(sz), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    for (size_t i = 0; i < *sz; i++) {
      uint64_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(T));
      set(i, val);
      in_size += sizeof(T);
    }

    return in_size;
  }

 private:
  std::array<AtomicBucketRef, NBUCKETS> buckets_;
};

template<class T, uint32_t NBUCKETS = 32>
class __LockFreeBaseAtomic {
 public:
  static const uint32_t FBS = 16;
  static const uint32_t FBS_HIBIT = 4;

  typedef std::atomic<T> AtomicType;
  typedef std::atomic<AtomicType*> AtomicBucketRef;

  __LockFreeBaseAtomic() {
    AtomicType* null_ptr = NULL;
    for (auto& x : buckets_)
      x = null_ptr;
    buckets_[0] = new AtomicType[FBS];
  }

  void try_allocate_bucket(uint32_t bucket_idx) {
    uint32_t size = (1U << (bucket_idx + FBS_HIBIT));
    AtomicType* new_bucket = new AtomicType[size];
    AtomicType* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_weak(&buckets_[bucket_idx], &null_ptr,
                                          new_bucket)) {
      return;
    }

    // All other threads will deallocate the newly allocated bucket.
    delete[] new_bucket;
  }

  void set(uint32_t idx, const T val) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = HighestBit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx] == NULL)
      try_allocate_bucket(bucket_idx);
    buckets_[bucket_idx][bucket_off] = val;
  }

  T get(const uint32_t idx) const {
    uint32_t pos = idx + FBS;
    uint32_t hibit = HighestBit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx][bucket_off];
  }

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

  uint32_t deserialize(std::istream& in, uint32_t *sz) {
    // Read keys
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(sz), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    for (size_t i = 0; i < *sz; i++) {
      uint64_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(T));
      set(i, val);
      in_size += sizeof(T);
    }

    return in_size;
  }

 protected:
  std::array<AtomicBucketRef, NBUCKETS> buckets_;
};

template<class T, uint32_t NBUCKETS = 32>
class LockFreeGrowingList : public __LockFreeBase<T, NBUCKETS> {
 public:
  LockFreeGrowingList()
      : write_tail_(0),
        read_tail_(0) {
  }

  uint32_t push_back(const T val) {
    uint32_t idx = std::atomic_fetch_add(&write_tail_, 1U);
    this->set(idx, val);
    while (!std::atomic_compare_exchange_weak(&read_tail_, &idx, idx + 1))
      ;
    return idx;
  }

  const T at(const uint32_t idx) {
    return this->get(idx);
  }

  const uint32_t size() {
    return read_tail_;
  }

  const uint32_t serialize(std::ostream& out) {
    return __LockFreeBase<T, NBUCKETS>::serialize(out, this->size());
  }

  const uint32_t deserialize(std::istream& in) {
    uint32_t num_entries;
    uint32_t in_size = __LockFreeBase<T, NBUCKETS>::deserialize(in,
                                                                &num_entries);
    write_tail_ = num_entries;
    read_tail_ = num_entries;
    return in_size;
  }

 private:
  std::atomic<uint32_t> write_tail_;
  std::atomic<uint32_t> read_tail_;
};

#endif
