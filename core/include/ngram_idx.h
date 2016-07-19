#ifndef NGRAM_IDX_H_
#define NGRAM_IDX_H_

#include <map>
#include <unordered_map>

#include "flags.h"
#include "offset_list.h"
#include "locks.h"
#include "hash_ops.h"
#include "lock_free_ngram_idx.h"

class ConcurrentNGramIdx {
 public:
#ifdef USE_INT_HASH
#ifdef USE_STL_HASHMAP_NGRAM
  typedef std::unordered_map<uint32_t, OffsetList*> NGramIdx;
#else
  typedef std::map<uint32_t, OffsetList*> NGramIdx;
#endif
#else
  typedef std::map<const std::string, OffsetList*> NGramIdx;
#endif

  ConcurrentNGramIdx() {
  }

  void add_offset(const char* ngram, const uint32_t offset) {
    add_if_not_contains(ngram);
    OffsetList* o;
    {
      ReadLock guard(mtx_);
#ifdef USE_INT_HASH
      uint32_t hash = Hash::simple_hash(ngram);
#else
      std::string hash(ngram, ngram_n_);
#endif
      o = idx_.at(hash);
    }
    o->push_back(offset);
  }

  OffsetList* get_offsets(const char* ngram) {
    ReadLock guard(mtx_);
#ifdef USE_INT_HASH
    uint32_t hash = Hash::simple_hash(ngram);
#else
    std::string hash(ngram, ngram_n_);
#endif
    return idx_.at(hash);
  }

  size_t serialize(std::ostream& out) {
    size_t out_size = 0;

    // Write n-gram index
    size_t ngram_idx_size = idx_.size();
    out.write(reinterpret_cast<const char *>(&(ngram_idx_size)),
              sizeof(size_t));
    out_size += sizeof(size_t);
    for (auto& entry : idx_) {
#ifdef USE_INT_HASH
      out.write(reinterpret_cast<const char *>(&(entry.first)),
                sizeof(uint32_t));
      out_size += (sizeof(uint32_t));
#else
      out.write(reinterpret_cast<const char *>(entry.first.c_str()),
          ngram_n_ * sizeof(char));
      out_size += (ngram_n_ * sizeof(char));
#endif

      out_size += entry.second->serialize(out);
    }
    return out_size;
  }

  size_t deserialize(std::istream& in) {
    size_t in_size = 0;

    // Read n-gram index
    size_t ngram_idx_size;
    in.read(reinterpret_cast<char *>(&ngram_idx_size), sizeof(size_t));
    in_size += sizeof(size_t);
#ifndef USE_INT_HASH
    char *ngram_buf = new char[ngram_n_];
#endif
    for (size_t i = 0; i < ngram_idx_size; i++) {

#ifdef USE_INT_HASH
      uint32_t first;

      in.read(reinterpret_cast<char *>(&(first)), sizeof(uint32_t));
      in_size += sizeof(uint32_t);
#else
      typedef std::pair<const std::string, std::vector<uint32_t>> IdxEntry;

      std::string first;
      in.read(reinterpret_cast<char *>(ngram_buf), ngram_n_ * sizeof(char));
      first = std::string(ngram_buf);
      in_size += (ngram_n_ * sizeof(char));
#endif

      in_size += idx_.at(first)->deserialize(in);
    }

    return in_size;
  }

 private:
  bool contains(const NGramIdx::key_type& ngram) {
    ReadLock guard(mtx_);
    return idx_.find(ngram) != idx_.end();
  }

  void add_if_not_contains(const char* ngram) {
#ifdef USE_INT_HASH
    uint32_t hash = Hash::simple_hash(ngram);
#else
    std::string hash(ngram, ngram_n_);
#endif
    if (!contains(hash)) {
      WriteLock guard(mtx_);
      idx_[hash] = new OffsetList;
    }
  }

  NGramIdx idx_;
  Mutex mtx_;
};

template<uint32_t MAX_SIZE = 16777216>
class ArrayNGramIdx {
 public:
  typedef std::atomic<OffsetList*> AtomicRef;

  ArrayNGramIdx() {
    OffsetList* null_ptr = NULL;
    for (auto& x : idx_)
      x = null_ptr;
  }

  void add_offset(const char* ngram, const uint32_t offset) {
#ifdef USE_INT_HASH
    uint32_t hash = Hash::simple_hash(ngram);
#else
    std::string hash(ngram, ngram_n_);
#endif
    if (idx_[hash] == NULL) {
      try_allocate_list(hash);
    }
    OffsetList* list = idx_[hash];
    list->push_back(offset);
  }

  OffsetList* get_offsets(const char* ngram) {
#ifdef USE_INT_HASH
    uint32_t hash = Hash::simple_hash(ngram);
#else
    std::string hash(ngram, ngram_n_);
#endif
    return idx_[hash];
  }

  size_t serialize(std::ostream& out) {
    size_t out_size = 0;

    // Write n-gram index
    size_t ngram_idx_size = size();
    out.write(reinterpret_cast<const char *>(&(ngram_idx_size)),
              sizeof(size_t));
    out_size += sizeof(size_t);
    {
      for (uint32_t i = 0; i < MAX_SIZE; i++) {
        OffsetList* entry = idx_[i];
        if (entry != NULL) {
          out.write(reinterpret_cast<const char *>(&(i)), sizeof(uint32_t));
          out_size += (sizeof(uint32_t));
          out_size += entry->serialize(out);
        }
      }
    }
    return out_size;
  }

  size_t deserialize(std::istream& in) {
    size_t in_size = 0;

    // Read n-gram index
    size_t ngram_idx_size;
    in.read(reinterpret_cast<char *>(&ngram_idx_size), sizeof(size_t));
    in_size += sizeof(size_t);
    for (size_t i = 0; i < ngram_idx_size; i++) {
      uint32_t first;

      in.read(reinterpret_cast<char *>(&(first)), sizeof(uint32_t));
      in_size += sizeof(uint32_t);

      OffsetList* offset_list = new OffsetList;
      in_size += offset_list->deserialize(in);
      idx_[first] = offset_list;
    }

    return in_size;
  }

 private:
  void try_allocate_list(uint32_t i) {
    OffsetList* new_list = new OffsetList;
    OffsetList* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_weak(&idx_[i], &null_ptr, new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  size_t size() {
    size_t sz = 0;
    for (auto& x : idx_)
      if (x != NULL)
        sz++;
    return sz;
  }

  std::array<AtomicRef, MAX_SIZE> idx_;
};

typedef LockFreeNGramIdx NGramIdx;

#endif /* NGRAM_IDX_H_ */
