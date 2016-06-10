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
#include "value_offsets.h"
#include "offset_list.h"
#include "ngram_idx.h"
#include "memory_map.h"

namespace succinct {

template<uint32_t MAX_KEYS = 134217728, uint32_t LOG_SIZE = 1073741824>
class LogStore {
 public:
  static const uint64_t KEY_INCR = 1ULL << 32;

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
    if (GetSize() + value.length() > LOG_SIZE || GetNumKeys() > MAX_KEYS) {
      return -1;   // Data exceeds max log size
    }

    // Atomically update the ongoing append tail of the log
    uint64_t tail_increment = TailIncrement(value.length());
    uint64_t current_tail = AtomicUpdateOngoingAppendsTail(tail_increment);

    // This thread now has exclusive access to
    // (1) the current internal key, and
    // (2) the region marked by (current value offset, current value offset + current value length)
    uint32_t internal_key = current_tail << 32;
    uint32_t value_offset = current_tail & 0xFFFF, value_length =
        value.length();

    // We can add the new value offset to the value offsets array
    // without worrying about locking, since we have uncontested
    // access to the 'internal_key' index in the value offsets array.
    value_offsets_.set(internal_key, value_offset);

    // Similarly, we can append the value to the log without locking
    // since this thread has exclusive access to the region (value_offset, value_offset + .
    memcpy(data_ + value_offset, value.c_str(), value_length);

    // Safely update secondary index entries. Currently uses locks for the index; TODO: remove
    uint32_t value_end = value_offset + value_length;
    for (uint32_t i = value_offset; i < value_end - ngram_n_; i++) {
#ifdef USE_INT_HASH
      uint32_t ngram = Hash::simple_hash3(data_ + i);
#else
      std::string ngram(data_ + i, ngram_n_);
#endif
      ngram_idx_.add_if_not_contains(ngram);
      ngram_idx_.at(ngram).push_back(i);
    }

    // Atomically update the completed append tail of the log.
    // This is done using CAS, and may have bounded waiting until
    // all appends before the current_tail are completed.
    AtomicUpdateCompletedAppendsTail(current_tail, tail_increment);

    // Return the current internal key
    return internal_key;
  }

  const void Get(std::string& value, const int64_t key) {
    uint64_t current_tail = completed_appends_tail_;
    uint32_t max_key = current_tail >> 32;
    if (key < 0 || key >= max_key)
      return;

    uint32_t start = value_offsets_.get(key);
    uint32_t end =
        (key + 1 < max_key) ? value_offsets_.get(key + 1) : current_tail & 0xFF;

    value.assign(data_ + start, end - start);
  }

  const void Search(std::set<int64_t>& results, const std::string& query) {
    uint64_t current_tail = completed_appends_tail_;
    uint32_t max_key = current_tail >> 32;
    uint32_t max_off = current_tail & 0xFFFF;
    char *substr = (char *) query.c_str();
    char *suffix = substr + ngram_n_;
    size_t suffix_len = query.length() - ngram_n_;

#ifdef USE_INT_HASH
    uint32_t prefix_ngram = Hash::simple_hash3(substr);
#else
    std::string prefix_ngram = query.substr(0, ngram_n_);
#endif

#ifdef LOCK_FREE_OFFSET_LIST
    OffsetList& offsets = ngram_idx_.at(prefix_ngram);
#else
    std::vector<uint32_t> offsets;
    ngram_idx_.at(prefix_ngram).snapshot(offsets);
#endif

    uint32_t size = offsets.size();
    for (uint32_t i = 0; i < size; i++) {
      if (!strncmp(data_ + offsets.at(i) + ngram_n_, suffix, suffix_len)) {
        // TODO: Take care of query.length() <= ngram_n_ case
        int64_t key = GetKey(offsets.at(i), max_key);
        if (key >= 0 && key < max_key) {
          results.insert(key);
        }
      }
    }
  }

  const int64_t Dump(const std::string& path) {
    int64_t out_size = 0;
    uint64_t tail = completed_appends_tail_;
    uint32_t log_size = tail & 0xFFFF;
    uint32_t max_key = tail >> 32;
    std::ofstream out(path);

    // Write data
    out.write(reinterpret_cast<const char *>(&(log_size)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    out.write(reinterpret_cast<const char *>(data_), log_size * sizeof(char));
    out_size += (log_size * sizeof(char));

    // Write value offsets
    out_size += value_offsets_.serialize(out, max_key);

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
    uint32_t log_size;
    uint32_t max_key;
    std::ifstream in(path);

    // Read data
    in.read(reinterpret_cast<char *>(&log_size), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    in.read(reinterpret_cast<char *>(data_), log_size * sizeof(char));
    in_size += (log_size * sizeof(char));

    // Read value offsets
    in_size += value_offsets_.deserialize(in, &max_key);
    ongoing_appends_tail_ = ((uint64_t) max_key) << 32 | ((uint64_t) log_size);
    completed_appends_tail_ = ((uint64_t) max_key) << 32
        | ((uint64_t) log_size);

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
    uint64_t current_tail = completed_appends_tail_;
    return current_tail >> 32;
  }

  const int64_t GetSize() {
    uint64_t current_tail = completed_appends_tail_;
    return current_tail & 0xFF;
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

  uint64_t TailIncrement(uint32_t value_length) {
    return KEY_INCR | value_length;
  }

  uint64_t AtomicUpdateOngoingAppendsTail(uint64_t tail_increment) {
    return std::atomic_fetch_add(&ongoing_appends_tail_, tail_increment);
  }

  void AtomicUpdateCompletedAppendsTail(uint64_t expected_append_tail,
                                        uint64_t tail_increment) {
    while (!std::atomic_compare_exchange_weak(
        &completed_appends_tail_, &expected_append_tail,
        expected_append_tail + tail_increment))
      ;
  }

  const int64_t GetKey(const uint32_t value_offset, const uint32_t max_key) {
    auto begin = value_offsets_.begin();
    auto end = value_offsets_.end(max_key);
    int64_t internal_key = std::prev(std::upper_bound(begin, end, value_offset)) - begin;
    return (internal_key >= max_key || internal_key < 0) ? -1 : internal_key;
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
  std::atomic<uint64_t> ongoing_appends_tail_;
  std::atomic<uint64_t> completed_appends_tail_;

  ValueOffsets value_offsets_;
  std::vector<bool> valid_records_;

  ConcurrentNGramIdx ngram_idx_;
  uint32_t ngram_n_;

  Mutex value_offsets_mtx_;
  Mutex secondary_idx_mtx_;
  Mutex valid_records_mtx_;
};
}

#endif
