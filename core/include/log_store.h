#ifndef LOG_STORE_H_
#define LOG_STORE_H_

#include <cstdint>
#include <string>
#include <set>
#include <map>
#include <vector>

#include <boost/thread.hpp>

// #define USE_INT_HASH

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
  typedef std::map<uint32_t, std::vector<uint32_t>> NGramIdx;
#else
  typedef std::map<const std::string, std::vector<uint32_t>> NGramIdx;
#endif

  LogStore(uint32_t ngram_n);

  int Append(const int64_t key, const std::string& value);
  void Get(std::string& value, const int64_t key);
  void Search(std::set<int64_t>& _return, const std::string& query);
  int64_t Dump(const std::string& path);
  int64_t Load(const std::string& path);

  size_t GetNumKeys() {
    return keys_.size();
  }

  int64_t GetCurrentSize() {
    return tail_;
  }

 private:

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

  char *data_;
  uint32_t tail_;

  std::vector<int32_t> keys_;
  std::vector<int32_t> value_offsets_;

  NGramIdx ngram_idx_;
  uint32_t ngram_n_;

  boost::shared_mutex mutex_;
};
}

#endif
