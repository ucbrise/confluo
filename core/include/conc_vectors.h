#ifndef CONC_VECTORS_H_
#define CONC_VECTORS_H_

#include <array>
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

static inline uint32_t log2(uint32_t x) {
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

template<class T, uint32_t FBS = 2, uint32_t NBUCKETS = 32>
class LockFreeGrowingList {
 public:
  typedef std::atomic<T*> AtomicBucketRef;

  LockFreeGrowingList() {
    T* null_ptr = NULL;
    for (auto& x : buckets_)
      x = null_ptr;
    buckets_[0] = new T[FBS];
    T* bucket = buckets_[0];
    write_tail_ = 0;
    read_tail_ = 0;
  }

  uint32_t push_back(const T val) {
    uint32_t idx = write_tail_.fetch_add(1);
    uint32_t bucket_idx = idx >= FBS ? (log2(idx / FBS) + 1) : 0;

    if (buckets_[bucket_idx] == NULL)
      try_allocate_bucket(bucket_idx);

    uint32_t bucket_start = idx >= FBS ? (FBS * (1U << (bucket_idx - 1))) : 0;
    uint32_t bucket_off = idx - bucket_start;
    set(bucket_idx, bucket_off, val);
    while (!std::atomic_compare_exchange_weak(&read_tail_, &idx, idx + 1));
    return idx;
  }

  const T at(const uint32_t idx) {
    uint32_t bucket_idx = idx >= FBS ? (log2(idx / FBS) + 1) : 0;
    uint32_t bucket_off = idx - (FBS * (1U << (bucket_idx - 1)));
    return get(bucket_idx, bucket_off);
  }

  const uint32_t size() {
    return read_tail_;
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
  void try_allocate_bucket(uint32_t bucket_idx) {
    uint32_t size = FBS * (1U << (bucket_idx - 1));
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

  void set(uint32_t bucket_idx, uint32_t bucket_off, const T val) {
    T* bucket = buckets_[bucket_idx];
    bucket[bucket_off] = val;
  }

  const T get(uint32_t bucket_idx, uint32_t bucket_off) {
    T* bucket = buckets_[bucket_idx];
    return bucket[bucket_off];
  }

  std::array<AtomicBucketRef, NBUCKETS> buckets_;
  std::atomic<uint32_t> write_tail_;
  std::atomic<uint32_t> read_tail_;
};

#endif /* CONC_VECTORS_H_ */
