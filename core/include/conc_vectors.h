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

template<class LockFreeBaseImpl>
class ConstLockFreeBaseIterator {
 public:
  typedef typename LockFreeBaseImpl::pos_type pos_type;

  typedef typename LockFreeBaseImpl::difference_type difference_type;
  typedef typename LockFreeBaseImpl::value_type value_type;
  typedef typename LockFreeBaseImpl::pointer pointer;
  typedef typename LockFreeBaseImpl::reference reference;
  typedef typename LockFreeBaseImpl::iterator_category iterator_category;

  typedef typename LockFreeBaseImpl::value_type const_reference;

  ConstLockFreeBaseIterator(const LockFreeBaseImpl* array, pos_type pos)
      : array_(array),
        pos_(pos) {
  }

  const_reference operator*() const {
    return array_->get(pos_);
  }

  ConstLockFreeBaseIterator& operator++() {
    pos_++;
    return *this;
  }

  ConstLockFreeBaseIterator operator++(int) {
    ConstLockFreeBaseIterator it = *this;
    ++(*this);
    return it;
  }

  ConstLockFreeBaseIterator& operator--() {
    pos_--;
    return *this;
  }

  ConstLockFreeBaseIterator operator--(int) {
    ConstLockFreeBaseIterator it = *this;
    --(*this);
    return it;
  }

  ConstLockFreeBaseIterator& operator+=(difference_type i) {
    pos_ += i;
    return *this;
  }

  ConstLockFreeBaseIterator& operator-=(difference_type i) {
    pos_ -= i;
    return *this;
  }

  ConstLockFreeBaseIterator operator+(difference_type i) const {
    ConstLockFreeBaseIterator it = *this;
    return it += i;
  }

  ConstLockFreeBaseIterator operator-(difference_type i) const {
    ConstLockFreeBaseIterator it = *this;
    return it -= i;
  }

  const_reference operator[](difference_type i) const {
    return *(*this + i);
  }

  bool operator==(const ConstLockFreeBaseIterator& it) const {
    return it.pos_ == pos_;
  }

  bool operator!=(const ConstLockFreeBaseIterator& it) const {
    return !(*this == it);
  }

  bool operator<(const ConstLockFreeBaseIterator& it) const {
    return pos_ < it.pos_;
  }

  bool operator>(const ConstLockFreeBaseIterator& it) const {
    return pos_ > it.pos_;
  }

  bool operator>=(const ConstLockFreeBaseIterator& it) const {
    return !(*this < it);
  }

  bool operator<=(const ConstLockFreeBaseIterator& it) const {
    return !(*this > it);
  }

  difference_type operator-(const ConstLockFreeBaseIterator& it) {
    return pos_ - it.pos_;
  }

 private:
  const LockFreeBaseImpl *array_;
  pos_type pos_;
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
  // Type definitions
  typedef uint32_t pos_type;
  typedef uint32_t size_type;
  typedef ConstLockFreeBaseIterator<__LockFreeBase <T, NBUCKETS>> iterator;
  typedef ConstLockFreeBaseIterator<__LockFreeBase <T, NBUCKETS>> const_iterator;
  typedef std::random_access_iterator_tag iterator_category;
  typedef T value_type;
  typedef T* pointer;
  typedef T& reference;
  typedef int64_t difference_type;

  static const uint32_t FBS = 16;
  static const uint32_t FBS_HIBIT = 4;

  typedef std::atomic<T*> AtomicBucketRef;

  __LockFreeBase() {
    T* null_ptr = NULL;
    for (auto& x : buckets_)
      x = null_ptr;
    buckets_[0] = new T[FBS];
    T* bucket = buckets_[0];
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
    T* bucket = buckets_[bucket_idx];
    return bucket[bucket_off];
  }

  const_iterator begin() const {
    return const_iterator(this, 0);
  }

  const_iterator cbegin() const {
    return const_iterator(this, 0);
  }

  const_iterator end(const size_type num_elements) const {
    return const_iterator(this, num_elements);
  }

  const_iterator cend(const size_type num_elements) const {
    return const_iterator(this, num_elements);
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
class LockFreeGrowingList : public __LockFreeBase<T, NBUCKETS> {
 public:
  typedef uint32_t pos_type;
  typedef uint32_t size_type;
  typedef ConstLockFreeBaseIterator<__LockFreeBase <T, NBUCKETS>> iterator;
  typedef ConstLockFreeBaseIterator<__LockFreeBase <T, NBUCKETS>> const_iterator;
  typedef std::random_access_iterator_tag iterator_category;
  typedef T value_type;
  typedef T* pointer;
  typedef T& reference;
  typedef int64_t difference_type;

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

  const_iterator end() const {
    return const_iterator(this, read_tail_);
  }

  const_iterator cend() const {
    return const_iterator(this, read_tail_);
  }

 private:
  std::atomic<uint32_t> write_tail_;
  std::atomic<uint32_t> read_tail_;
};

#endif
