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
#include <mutex>
#include <thread>

#include "flags.h"
#include "hash_ops.h"

namespace succinct {

typedef struct {
  std::vector<uint32_t> offsets_;
#ifndef NON_CONCURRENT_WRITES
  std::mutex mtx_;
#endif
} OffsetList;

class LogStore {
 public:
  static const uint32_t kLogStoreSize = 1024 * 1024 * 1024;

#ifdef USE_INT_HASH
#ifdef USE_STL_HASHMAP_NGRAM
  typedef std::unordered_map<uint32_t, OffsetList> NGramIdx;
#else
  typedef std::map<uint32_t, std::vector<uint32_t>> NGramIdx;
#endif
#else
  typedef std::map<const std::string, std::vector<uint32_t>> NGramIdx;
#endif

  LogStore(uint32_t ngram_n = 3, const char* path = "log")
      : tail_(0),
        ngram_n_(ngram_n) {

    if ((page_size_ = sysconf(_SC_PAGE_SIZE)) < 0) {
      fprintf(stderr, "Could not obtain page size.\n");
      throw -1;
    }

    int fd = open(path, (O_CREAT | O_TRUNC | O_RDWR),
                  (S_IRWXU | S_IRWXG | S_IRWXO));

    if (fd < 0) {
      fprintf(stderr, "Could not obtain file descriptor.\n");
      throw -1;
    }

    off_t lastoffset = lseek(fd, kLogStoreSize, SEEK_SET);
    const char eof[1] = { 0 };
    size_t bytes_written = write(fd, eof, 1);

    if (bytes_written != 1) {
      fprintf(stderr, "Could not write to file.\n");
      throw -1;
    }

    data_ = (char *) mmap(NULL, kLogStoreSize, PROT_READ | PROT_WRITE,
    MAP_SHARED,
                          fd, 0);
    if (data_ == MAP_FAILED) {
      fprintf(stderr, "Could not mmap file.\n");
      throw -1;
    }
  }

  int Append(const int64_t key, const std::string& value) {
    if (tail_ + value.length() > kLogStoreSize) {
      return -1;   // Data exceeds max chunk size
    }

    // Update write_tail
    uint64_t value_offset = tail_.fetch_add(value.length());

    // Append value to log; can be done without locking since
    // this thread has exclusive access to the region.
    memcpy(data_ + value_offset, value.c_str(), value.length());

    // Safely update primary index
    {
      std::lock_guard<std::mutex> primary_idx_guard(append_mtx_);

      keys_.push_back(key);
      value_offsets_.push_back(value_offset);
    }

    // Safely update secondary index entries
    for (int64_t i = value_offset; i < value_offset + value.length() - ngram_n_;
        i++) {
#ifdef USE_INT_HASH
      uint32_t ngram = Hash::simple_hash3(data_ + i);
#else
      std::string ngram(data_ + i, ngram_n_);
#endif
      {
        if (ngram_idx_.find(ngram) == ngram_idx_.end()) {
          std::lock_guard<std::mutex> secondary_idx_guard(idx_append_mtx_);
          ngram_idx_[ngram];
        }
        std::lock_guard<std::mutex> guard(ngram_idx_.at(ngram).mtx_);
        ngram_idx_.at(ngram).offsets_.push_back(i);
      }
    }

    return 0;
  }

  const void Get(std::string& value, const int64_t key) {
    int64_t pos = GetValueOffsetPos(key);

    if (pos < 0)
      return;

    int64_t start = value_offsets_[pos];
    uint32_t tail = tail_;
    int64_t end =
        (pos + 1 < value_offsets_.size()) ? value_offsets_[pos + 1] : tail;
    size_t len = end - start;

    value.assign(data_ + start, len);
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

    auto& offsets = ngram_idx_.at(prefix_ngram).offsets_;
    for (uint32_t i = 0; i < offsets.size(); i++) {
      if (skip_filter
          || !strncmp(data_ + offsets[i] + ngram_n_, suffix, suffix_len)) {
        // TODO: Take care of query.length() < ngram_n_ case
        int64_t pos = GetKeyPos(offsets[i]);
        if (pos >= 0)
          results.insert(keys_[pos]);
      }
    }
  }

  const int64_t Dump(const std::string& path) {
    int64_t out_size = 0;
    uint32_t tail = tail_;
    std::ofstream out(path);

    // Write data
    out.write(reinterpret_cast<const char *>(&(tail)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    out.write(reinterpret_cast<const char *>(data_), tail * sizeof(char));
    out_size += (tail * sizeof(char));

    // Write keys
    out_size += WriteVectorToFile(out, keys_);

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

      out_size += WriteVectorToFile(out, entry.second.offsets_);
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
    tail_ = tail;

    // Read keys
    keys_.clear();
    in_size += ReadVectorFromFile(in, keys_);

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
    ngram_idx_.clear();
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

      in_size += ReadVectorFromFile(in, ngram_idx_[first].offsets_);
    }

    return in_size;
  }

  size_t GetNumKeys() {
    return keys_.size();
  }

  int64_t GetSize() {
    return tail_;
  }

 private:
  const int64_t GetValueOffsetPos(const int64_t key) {
    auto begin = keys_.begin();
    auto end = keys_.end();
    auto size = end - begin;
    size_t pos = std::lower_bound(begin, end, key) - begin;
    return (pos >= size || keys_[pos] != key) ? -1 : pos;
  }

  const int64_t GetKeyPos(const int64_t value_offset) {
    auto begin = value_offsets_.begin();
    auto end = value_offsets_.end();
    int64_t size = end - begin;
    int64_t pos = std::prev(std::upper_bound(begin, end, value_offset)) - begin;
    return (pos >= size) ? -1 : pos;
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
  std::atomic<uint32_t> tail_;
  int page_size_;

  std::vector<int64_t> keys_;
  std::vector<int32_t> value_offsets_;

  NGramIdx ngram_idx_;
  uint32_t ngram_n_;

  std::mutex append_mtx_;
  std::mutex idx_append_mtx_;
};
}

#endif
