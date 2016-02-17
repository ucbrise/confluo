#ifndef LOG_STORE_H_
#define LOG_STORE_H_

#include <cstdint>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <iterator>
#include "linked_list.h"

#include <boost/thread.hpp>

#define USE_INT_HASH
#define USE_STL_HASHMAP_NGRAM
// #define USE_BLOCK_LIST

namespace succinct {

#ifdef USE_INT_HASH
class Hash {
 public:
  static const uint32_t K1 = 256;
  static const uint32_t K2 = 65536;
  static const uint32_t K3 = 16777216;

  static uint32_t simple_hash2(const char* buf) {
    return buf[0] * K1 + buf[1];
  }

  static uint32_t simple_hash3(const char* buf) {
    return buf[0] * K2 + buf[1] * K1 + buf[2];
  }

  static uint32_t simple_hash4(const char* buf) {
    return buf[0] * K3 + buf[1] * K2 + buf[2] * K1 + buf[3];
  }
};
#endif

class LogStore {
 public:
  static const uint32_t kLogStoreSize = 1024 * 1024 * 1024;
  static const char kValueDelim = '\n';

#ifdef USE_INT_HASH
#ifdef USE_STL_HASHMAP_KV
  typedef std::unordered_map<uint32_t,
  std::unordered_map<int64_t, std::vector<uint32_t>>> NGramIdx;
  typedef std::unordered_map<int64_t, uint64_t> KeyIdx;
#else
#ifdef USE_STL_HASHMAP_NGRAM
#ifdef USE_BLOCK_LIST
  typedef std::unordered_map<uint32_t, LinkedList> NGramIdx;
#else
  typedef std::unordered_map<uint32_t, std::vector<uint32_t>> NGramIdx;
#endif
#else
  typedef std::map<uint32_t, std::vector<uint32_t>> NGramIdx;
#endif
#endif
#else
  typedef std::map<const std::string, std::vector<uint32_t>> NGramIdx;
#endif

  LogStore(uint32_t ngram_n = 3);

  int Append(const int64_t key, const std::string& value);
  void Get(std::string& value, const int64_t key);
  void Search(std::set<int64_t>& _return, const std::string& query);
  int64_t Dump(const std::string& path);
  int64_t Load(const std::string& path);

  size_t GetNumKeys() {
#ifdef USE_STL_HASHMAP_KV
    return key_map_.size();
#else
    return keys_.size();
#endif
  }

  size_t GetNumNGrams() {
    return ngram_idx_.size();
  }

  int64_t GetSize() {
    return tail_;
  }

 private:

#ifdef USE_STL_HASHMAP_KV
  bool MatchFirst(const std::vector<uint32_t>& offsets, const char* suffix, const size_t len) {
    for (auto off: offsets) {
      if (strncmp(data_ + off + ngram_n_, suffix, len)) {
        return true;
      }
    }
    return false;
  }
#else
  int64_t GetValueOffsetPos(const int64_t key) {
    size_t pos = std::lower_bound(keys_.begin(), keys_.end(), key)
        - keys_.begin();
    return (pos >= keys_.size() || keys_[pos] != key) ? -1 : pos;
  }

  int64_t GetKeyPos(const int64_t value_offset) {
    int64_t pos = std::prev(
        std::upper_bound(value_offsets_.begin(), value_offsets_.end(),
                         value_offset)) - value_offsets_.begin();
    return (pos >= (int64_t) value_offsets_.size()) ? -1 : pos;
  }
#endif

  template<typename T>
  size_t WriteVectorToFile(std::ostream& out, std::vector<T>& data) {
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
  size_t ReadVectorFromFile(std::istream& in, std::vector<T>& data) {
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

#ifdef USE_BLOCK_LIST
  size_t WriteListToFile(std::ostream& out, LinkedList& data) {
    size_t out_size = 0;

    size_t size = data.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size_t));
    out_size += sizeof(size_t);
    for (auto val : data) {
      out.write(reinterpret_cast<const char *>(&val), sizeof(uint32_t));
      out_size += sizeof(uint32_t);
    }

    return out_size;
  }

  size_t ReadListFromFile(std::istream& in, LinkedList& data) {
    // Read keys
    size_t in_size = 0;

    size_t size;
    in.read(reinterpret_cast<char *>(&size), sizeof(size_t));
    in_size += sizeof(size_t);
    for (size_t i = 0; i < size; i++) {
      uint64_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(uint32_t));
      data.push_back(val);
      in_size += sizeof(uint32_t);
    }

    return in_size;
  }
#endif

  char *data_;
  uint32_t tail_;

#ifdef USE_STL_HASHMAP_KV
  KeyIdx key_map_;
#else
  std::vector<int64_t> keys_;
  std::vector<int32_t> value_offsets_;
#endif

  NGramIdx ngram_idx_;
  uint32_t ngram_n_;

  boost::shared_mutex mutex_;
};
}

#endif
