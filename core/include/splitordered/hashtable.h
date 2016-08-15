#ifndef SPLITORDERED_HASHTABLE_H_
#define SPLITORDERED_HASHTABLE_H_

#include <cassert>
#include <cstdlib>

#include <atomic>
#include <array>

#include "defs.h"
#include "listops.h"
#include "utils.h"

namespace splitordered {

// The container class for the bucket array; can be doubled in size when
// the load exceeds threshold
template<class T>
class doubling_list {
 public:
  doubling_list() {
    T* null_ptr = NULL;
    for (auto& x : buckets_)
      x = null_ptr;
    buckets_[0] = new T[2];
    buckets_[0][0] = 0;
    buckets_[0][1] = 0;
    num_populated_buckets_.store(1);
  }

  ~doubling_list() {
    for (auto& x : buckets_)
      delete[] x;
  }

  T& operator[](uint32_t i) {
    uint32_t hibit = slog::bit_utils::highest_bit(i);
    uint32_t bucket_off = i ^ (1 << hibit);
    uint32_t bucket_idx = hibit;
    return buckets_[bucket_idx][bucket_off];
  }

  size_t size() {
    uint32_t n = num_populated_buckets_.load();
    return 0x01 << n;
  }

  size_t double_size(size_t expected_size) {
    uint32_t n = num_populated_buckets_.load();
    uint32_t s = 0x01 << n;
    if (expected_size == s) {
      T* new_bucket = new T[s];
      memset(new_bucket, 0, s * sizeof(T));
      T* null_ptr = NULL;

      // Only one thread will be successful in replacing the NULL reference with newly
      // allocated bucket.
      if (std::atomic_compare_exchange_weak(&buckets_[n], &null_ptr,
                                            new_bucket)) {
        num_populated_buckets_.fetch_add(1);
      } else {
        delete[] new_bucket;
      }
    }
    return size();
  }

 private:
  std::atomic<uint32_t> num_populated_buckets_;
  std::array<std::atomic<T*>, 32> buckets_;
};

// Lock-free hash table based on split-ordered lists
template<class data_type>
class hash_table {
 public:
  static const int32_t MAX_LOAD = 4;

  typedef hash_entry<data_type> node_t;
  typedef node_t* node_ptr_t;

  hash_table() {
    count.store(0);
    {
      node_t *dummy = new node_t();
      assert(dummy);
      buckets_[0] = dummy;
    }
  }

  ~hash_table() {
    node_ptr_t cursor;
    cursor = buckets_[0];
    while (cursor != NULL) {
      node_ptr_t tmp = cursor;
      cursor = cursor->next;
      delete tmp;
    }
  }

  bool insert(const key_t key, const data_type value) {
    node_t *node = new node_t();  // XXX: should pull out of a memory pool
    size_t bucket;
    uint64_t lkey = key;

    lkey = hashword(lkey);
    bucket = lkey % buckets_.size();

    node->key = regularkey(lkey);
    node->value = value;
    node->next = UNINITIALIZED;

    if (buckets_[bucket] == UNINITIALIZED)
      initialize_bucket(bucket);

    if (!list_ops<data_type>::insert(&(buckets_[bucket]), node, NULL)) {
      delete node;
      return false;
    }

    size_t csize = buckets_.size();
    if (count.fetch_add(1) / csize > MAX_LOAD) {
      // double size
      size_t dsize = buckets_.double_size(csize);
      assert(dsize >= csize * 2);
    }
    return true;
  }

  bool upsert(const key_t key, const data_type value, data_type* old_value) {
    size_t bucket;
    uint64_t lkey = key;

    lkey = hashword(lkey);
    bucket = lkey % buckets_.size();

    so_key_t node_key = regularkey(lkey);

    if (buckets_[bucket] == UNINITIALIZED)
      initialize_bucket(bucket);

    bool updated = list_ops<data_type>::upsert(&(buckets_[bucket]), node_key,
                                               value, old_value);

    size_t csize = buckets_.size();
    if (count.fetch_add(1) / csize > MAX_LOAD) {
      // double size
      size_t dsize = buckets_.double_size(csize);
      assert(dsize >= csize * 2);
    }

    return updated;
  }

  bool get(const key_t key, data_type* value) {
    size_t bucket;
    uint64_t lkey = key;

    lkey = hashword(lkey);
    bucket = lkey % buckets_.size();

    if (buckets_[bucket] == UNINITIALIZED)
      initialize_bucket(bucket);

    return list_ops<data_type>::find(&(buckets_[bucket]), regularkey(lkey),
                                     value,
                                     NULL,
                                     NULL,
                                     NULL);
  }

 private:
  void initialize_bucket(size_t bucket) {
    size_t parent = get_parent(bucket);
    node_ptr_t cur;

    if (buckets_[parent] == UNINITIALIZED)
      initialize_bucket(parent);

    node_t *dummy = new node_t();  // XXX: should pull out of a memory pool
    assert(dummy);
    dummy->key = dummykey(bucket);
    dummy->value = (data_type) 0;
    dummy->next = UNINITIALIZED;
    if (!list_ops<data_type>::insert(&(buckets_[parent]), dummy, &cur)) {
      delete dummy;
      dummy = cur;
      while (buckets_[bucket] != dummy)
        ;
    } else {
      buckets_[bucket] = dummy;
    }
  }

  inline size_t get_parent(uint64_t bucket) {
    uint64_t t = bucket;
    t |= t >> 1;
    t |= t >> 2;
    t |= t >> 4;
    t |= t >> 8;
    t |= t >> 16;
    t |= t >> 32;     // creates a mask
    return bucket & (t >> 1);
  }

  inline so_key_t regularkey(const key_t key) {
    return REVERSE(key | MSB);
  }

  inline so_key_t dummykey(const key_t key) {
    return REVERSE(key);
  }

  /* this function based on http://burtleburtle.net/bob/hash/evahash.html */
  inline uint64_t hashword(uint64_t key) { /*{{{*/
    uint32_t a, b, c;

    const union {
      uint64_t key;
      uint8_t b[sizeof(uint64_t)];
    } k = { key };

    a = b = c = 0x32533d0c + sizeof(uint64_t);  // an arbitrary value, randomly selected
    c += 47;

    b += k.b[7] << 24;
    b += k.b[6] << 16;
    b += k.b[5] << 8;
    b += k.b[4];
    a += k.b[3] << 24;
    a += k.b[2] << 16;
    a += k.b[1] << 8;
    a += k.b[0];

    c ^= b;
    c -= rot(b, 14);
    a ^= c;
    a -= rot(c, 11);
    b ^= a;
    b -= rot(a, 25);
    c ^= b;
    c -= rot(b, 16);
    a ^= c;
    a -= rot(c, 4);
    b ^= a;
    b -= rot(a, 14);
    c ^= b;
    c -= rot(b, 24);
    return ((uint64_t) c + (((uint64_t) b) << 32)) & (~MSB);
  }

  doubling_list<node_ptr_t> buckets_;
  std::atomic<int64_t> count;
};

}

#endif /* SPLITORDERED_HASHTABLE_H_ */
