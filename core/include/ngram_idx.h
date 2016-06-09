#ifndef NGRAM_IDX_H_
#define NGRAM_IDX_H_

#include <map>
#include <unordered_map>

#include "offset_list.h"
#include "locks.h"

#ifdef USE_INT_HASH
#ifdef USE_STL_HASHMAP_NGRAM
typedef std::unordered_map<uint32_t, OffsetList> NGramIdx;
#else
typedef std::map<uint32_t, std::vector<uint32_t>> NGramIdx;
#endif
#else
typedef std::map<const std::string, std::vector<uint32_t>> NGramIdx;
#endif

class ConcurrentNGramIdx {
 public:
  ConcurrentNGramIdx() {
  }

  void add_if_not_contains(const NGramIdx::key_type& ngram) {
    if (!contains(ngram)) {
      WriteLock guard(mtx_);
      idx_[ngram];
    }
  }

  NGramIdx::mapped_type& at(const NGramIdx::key_type& ngram) {
    ReadLock guard(mtx_);
    return idx_.at(ngram);
  }

  NGramIdx::size_type size() {
    ReadLock guard(mtx_);
    return idx_.size();
  }

  NGramIdx::iterator begin() {
    return idx_.begin();
  }

  NGramIdx::iterator end() {
    return idx_.end();
  }

 private:
  bool contains(const NGramIdx::key_type& ngram) {
    ReadLock guard(mtx_);
    return idx_.find(ngram) != idx_.end();
  }

  NGramIdx idx_;
  Mutex mtx_;
};

#endif /* NGRAM_IDX_H_ */
