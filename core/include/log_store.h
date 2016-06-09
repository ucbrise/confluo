#ifndef LOG_STORE_H_
#define LOG_STORE_H_

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/mman.h>

#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <atomic>

#include "flags.h"
#include "hash_ops.h"
#include "locks.h"
#include "offset_list.h"
#include "ngram_idx.h"
#include "memory_map.h"

namespace succinct {

template<uint32_t MAX_KEYS = 134217728, uint32_t LOG_SIZE = 1073741824>
class LogStore {
 public:
  LogStore(uint32_t ngram_n = 3, const char* path = "log")
      : ongoing_appends_tail_(0),
        completed_appends_tail_(0),
        ngram_n_(ngram_n) {

    data_ = MemoryMap::map(path, LOG_SIZE);
    valid_records_.resize(MAX_KEYS, false);
  }

  int Append(const int64_t key, const std::string& value) {
    return Append(value);
  }

  int32_t Append(const std::string& value) {
    if (ongoing_appends_tail_ + value.length() > LOG_SIZE) {
      return -1;   // Data exceeds max log size
    }

    uint32_t value_offset;
    uint32_t internal_key;

    // Safely update keys, value offsets
    {
      // TODO: Take care of user keys
      WriteLock value_offsets_guard(value_offsets_mtx_);

      value_offset = ongoing_appends_tail_.fetch_add(value.length());
      internal_key = value_offsets_.size();
      value_offsets_.push_back(value_offset);
    }

    uint32_t value_end = value_offset + value.length();

    // Append value to log; can be done without locking since
    // this thread has exclusive access to the region.
    memcpy(data_ + value_offset, value.c_str(), value.length());

    // Safely update secondary index entries
    for (uint32_t i = value_offset; i < value_end - ngram_n_; i++) {
#ifdef USE_INT_HASH
      uint32_t ngram = Hash::simple_hash3(data_ + i);
#else
      std::string ngram(data_ + i, ngram_n_);
#endif
      ngram_idx_.add_if_not_contains(ngram);
      ngram_idx_.at(ngram).push_back(i);
    }

    Validate(internal_key);

    while (!std::atomic_compare_exchange_weak(&completed_appends_tail_,
                                              &value_offset, value_end))
      ;

    return internal_key;
  }

  const void Get(std::string& value, const int64_t key) {
    if (key < 0)
      return;

    int64_t start, end;
    {
      ReadLock value_offsets_guard(value_offsets_mtx_);
      start = value_offsets_.at(key);
      uint32_t tail = ongoing_appends_tail_;
      end =
          (key + 1 < value_offsets_.size()) ? value_offsets_.at(key + 1) : tail;
    }

    value.assign(data_ + start, end - start);
  }

  const void Search(std::set<int64_t>& results, const std::string& query) {
    char *substr = (char *) query.c_str();
    char *suffix = substr + ngram_n_;
    bool skip_filter = (query.length() <= ngram_n_);
    size_t suffix_len = skip_filter ? 0 : query.length() - ngram_n_;

#ifdef USE_INT_HASH
    uint32_t prefix_ngram = Hash::simple_hash3(substr);
#else
    std::string prefix_ngram = query.substr(0, ngram_n_);
#endif

#ifdef LOCK_FREE
    OffsetList& offsets = ngram_idx_.at(prefix_ngram);
#else
    std::vector<uint32_t> offsets;
    ngram_idx_.at(prefix_ngram).snapshot(offsets);
#endif

    uint32_t size = offsets.size();
    for (uint32_t i = 0; i < offsets.size(); i++) {
      if (skip_filter
          || !strncmp(data_ + offsets.at(i) + ngram_n_, suffix, suffix_len)) {
        // TODO: Take care of query.length() < ngram_n_ case
        int64_t key = GetKey(offsets.at(i));
        if (key >= 0) {
          results.insert(key);
        }
      }
    }
  }

  const int64_t Dump(const std::string& path) {
    int64_t out_size = 0;
    uint32_t tail = completed_appends_tail_;
    std::ofstream out(path);

    // Write data
    out.write(reinterpret_cast<const char *>(&(tail)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    out.write(reinterpret_cast<const char *>(data_), tail * sizeof(char));
    out_size += (tail * sizeof(char));

    // Write value offsets
    out_size += WriteVectorToFile(out, value_offsets_);

    // Write n-gram index
    out.write(reinterpret_cast<const char *>(&(ngram_n_)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    size_t ngram_idx_size = ngram_idx_.size();
    out.write(reinterpret_cast<const char *>(&(ngram_idx_size)),
              sizeof(size_t));
    out_size += sizeof(size_t);
    for (auto& entry : ngram_idx_) {
#ifdef USE_INT_HASH
      out.write(reinterpret_cast<const char *>(&(entry.first)),
                sizeof(uint32_t));
      out_size += (sizeof(uint32_t));
#else
      out.write(reinterpret_cast<const char *>(entry.first.c_str()),
          ngram_n_ * sizeof(char));
      out_size += (ngram_n_ * sizeof(char));
#endif

      out_size += entry.second.serialize(out);
    }
    return out_size;
  }

  int64_t Load(const std::string& path) {
    int64_t in_size = 0;
    uint32_t tail;
    std::ifstream in(path);

    // Read data
    in.read(reinterpret_cast<char *>(&tail), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    in.read(reinterpret_cast<char *>(data_), tail * sizeof(char));
    in_size += (tail * sizeof(char));
    ongoing_appends_tail_ = tail;
    completed_appends_tail_ = tail;

    // Read value offsets
    value_offsets_.clear();
    in_size += ReadVectorFromFile(in, value_offsets_);

    // Read n-gram index
    in.read(reinterpret_cast<char *>(&ngram_n_), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
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

      in_size += ngram_idx_.at(first).deserialize(in);
    }

    return in_size;
  }

  const size_t GetNumKeys() {
    ReadLock value_offsets_guard(value_offsets_mtx_);
    return value_offsets_.size();
  }

  const int64_t GetSize() {
    return completed_appends_tail_;
  }

 private:
  void Validate(const size_t key_pos) {
    WriteLock valid_records_guard(valid_records_mtx_);
    valid_records_[key_pos] = true;
  }

  const bool IsValid(const size_t key_pos) {
    ReadLock valid_records_guard(valid_records_mtx_);
    return valid_records_.at(key_pos);
  }

  const int64_t GetKey(const int64_t value_offset) {
    int64_t pos, size;
    {
      ReadLock value_offsets_guard(value_offsets_mtx_);
      auto begin = value_offsets_.begin();
      auto end = value_offsets_.end();
      size = end - begin;
      pos = std::prev(std::upper_bound(begin, end, value_offset)) - begin;
    }

    return (pos >= size || !IsValid(pos)) ? -1 : pos;
  }

  template<typename T>
  const size_t WriteVectorToFile(std::ostream& out, std::vector<T>& data) {
    size_t out_size = 0;

    size_t size = data.size();
    out.write(reinterpret_cast<const char *>(&(size)), sizeof(size_t));
    out_size += sizeof(size_t);
    for (size_t i = 0; i < size; i++) {
      out.write(reinterpret_cast<const char *>(&data[i]), sizeof(T));
      out_size += sizeof(T);
    }

    return out_size;
  }

  template<typename T>
  const size_t ReadVectorFromFile(std::istream& in, std::vector<T>& data) {
    // Read keys
    size_t in_size = 0;

    size_t size;
    in.read(reinterpret_cast<char *>(&size), sizeof(size_t));
    in_size += sizeof(size_t);
    data.reserve(size);
    for (size_t i = 0; i < size; i++) {
      uint64_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(T));
      data.push_back(val);
      in_size += sizeof(T);
    }

    return in_size;
  }

  char *data_;
  std::atomic<uint32_t> ongoing_appends_tail_;
  std::atomic<uint32_t> completed_appends_tail_;

  std::vector<int32_t> value_offsets_;
  std::vector<bool> valid_records_;

  ConcurrentNGramIdx ngram_idx_;
  uint32_t ngram_n_;

  Mutex value_offsets_mtx_;
  Mutex secondary_idx_mtx_;
  Mutex valid_records_mtx_;
};
}

#endif
