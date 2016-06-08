#ifndef OFFSET_LIST_H_
#define OFFSET_LIST_H_

#include <atomic>

#include "flags.h"
#include "locks.h"

template<class T>
class ConcurrentVector {
 public:
  ConcurrentVector() {
  }

  void push_back(const T val) {
    WriteLock write_guard(mtx_);
    data_.push_back(val);
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

  std::vector<T>& vector() {
    return data_;
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
  while (x >>= 1) ++y;
#endif
  return y;
}

template<class T, uint32_t FBS = 2, uint32_t NBUCKETS = 32>
class LockFreeGrowingList {
 public:
  typedef std::atomic<T*> AtomicBucketRef;

  static uint32_t bucket_sizes[];

  LockFreeGrowingList() {
    buckets_ = {};
  }

  void push_back(const T val) {
    uint32_t idx = size_.fetch_add(1);
    uint32_t bucket_idx = idx > 0 ? (log2(idx / FBS) + 1) : 0;
    if (buckets_[bucket_idx] == NULL) {
      try_allocate_bucket(bucket_idx);
    }
    uint32_t bucket_off = idx - (FBS * (1U << (bucket_idx - 1)));
    set(bucket_idx, bucket_off, val);
  }

  const T at(const uint32_t idx) {
    uint32_t bucket_idx = idx > 0 ? (log2(idx / FBS) + 1) : 0;
    uint32_t bucket_off = idx - (FBS * (1U << (bucket_idx - 1)));
    return get(bucket_idx, bucket_off);
  }

private:
  void try_allocate_bucket(uint32_t bucket_idx) {
    uint32_t size = FBS * (1U << (bucket_idx - 1));
    T* new_bucket = new T[size];

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_weak(&buckets_[bucket_idx], NULL, new_bucket)) {
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
  std::atomic<uint32_t> size_;
};

typedef ConcurrentVector<uint32_t> OffsetList;

#endif /* OFFSET_LIST_H_ */
