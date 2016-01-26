#include "log_store.h"

#include <fstream>

namespace succinct {

LogStore::LogStore(uint32_t ngram_n) {
  data_ = new char[LogStore::kLogStoreSize];
  tail_ = 0;

  ngram_n_ = ngram_n;
}

int LogStore::Append(const int64_t key, const std::string& value) {
  boost::unique_lock<boost::shared_mutex> lk(mutex_);

  if (tail_ + value.length() > kLogStoreSize) {
    return -1;   // Data exceeds max chunk size
  }

  // Append value to log
  strncpy(data_ + tail_, value.c_str(), value.length());

  // Update primary index
  keys_.push_back(key);
  value_offsets_.push_back(tail_);

  // Update secondary index
  for (int64_t i = std::max(0LL, static_cast<int64_t>(tail_ - ngram_n_));
      i < tail_ + value.length() - ngram_n_; i++) {
    std::string ngram;
    for (uint32_t off = 0; off < ngram_n_; off++) {
      ngram += data_[i + off];
    }
    idx_[ngram].push_back(i);
  }
  tail_ += value.length();

  return 0;
}

void LogStore::Get(std::string& value, const int64_t key) {
  boost::shared_lock<boost::shared_mutex> lk(mutex_);
  int64_t pos = GetValueOffsetPos(key);
  if (pos < 0) {
    value = "INVALID";
    return;
  }
  int64_t start = value_offsets_[pos];
  int64_t end =
      (pos + 1 < value_offsets_.size()) ? value_offsets_[pos + 1] : tail_;
  size_t len = end - start;
  value.resize(len);
  for (size_t i = 0; i < len; ++i) {
    value[i] = data_[start + i];
  }
}

void LogStore::Search(std::set<int64_t>& results, const std::string& query) {
  std::string substring_ngram = query.substr(0, ngram_n_);

  char *substr = (char *) query.c_str();
  char *suffix = substr + ngram_n_;
  bool skip_filter = (query.length() <= ngram_n_);
  size_t suffix_len = skip_filter ? 0 : query.length() - ngram_n_;

  boost::shared_lock<boost::shared_mutex> lk(mutex_);
  std::vector<uint32_t> idx_off = idx_[substring_ngram];
  for (uint32_t i = 0; i < idx_off.size(); i++) {
    if (skip_filter
        || strncmp(data_ + idx_off[i] + ngram_n_, suffix, suffix_len) == 0) {
      int64_t pos = GetKeyPos(idx_off[i]);
      if (pos >= 0)
        results.insert(keys_[pos]);
    }
  }
}

int64_t LogStore::Dump(const std::string& path) {
  std::ofstream out(path);
  int64_t out_size;

  // Write data
  out.write(reinterpret_cast<const char *>(&(tail_)), sizeof(uint32_t));
  out_size += sizeof(uint32_t);
  out.write(reinterpret_cast<const char *>(data_), tail_ * sizeof(char));
  out_size += (tail_ * sizeof(char));

  // Write keys
  out_size += WriteVectorToFile(out, keys_);

  // Write value offsets
  out_size += WriteVectorToFile(out, value_offsets_);

  // Write n-gram index
  out.write(reinterpret_cast<const char *>(&(ngram_n_)), sizeof(uint32_t));
  out_size += sizeof(uint32_t);
  size_t ngram_idx_size = idx_.size();
  out.write(reinterpret_cast<const char *>(&(ngram_idx_size)), sizeof(size_t));
  out_size += sizeof(size_t);
  for (auto entry : idx_) {
    out.write(reinterpret_cast<const char *>(entry.first.c_str()),
              ngram_n_ * sizeof(char));
    out_size += (ngram_n_ * sizeof(char));
    out_size += WriteVectorToFile(out, entry.second);
  }

  return out_size;
}

int64_t LogStore::Load(const std::string& path) {
  std::ifstream in(path);
  int64_t in_size;

  // Read data
  in.read(reinterpret_cast<char *>(&tail_), sizeof(uint32_t));
  in_size += sizeof(uint32_t);
  in.read(reinterpret_cast<char *>(data_), tail_ * sizeof(char));
  in_size += (tail_ * sizeof(char));

  // Read keys
  in_size += ReadVectorFromFile(in, keys_);

  // Read value offsets
  in_size += ReadVectorFromFile(in, value_offsets_);

  // Read n-gram index
  in.read(reinterpret_cast<char *>(&ngram_n_), sizeof(uint32_t));
  in_size += sizeof(uint32_t);
  size_t ngram_idx_size;
  in.read(reinterpret_cast<char *>(&ngram_idx_size), sizeof(size_t));
  in_size += sizeof(size_t);
  char *ngram_buf = new char[ngram_n_];
  for (size_t i = 0; i < ngram_idx_size; i++) {
    std::string first;
    std::vector<uint32_t> second;
    in.read(reinterpret_cast<char *>(ngram_buf), ngram_n_ * sizeof(char));
    first = std::string(ngram_buf);
    in_size += (ngram_n_ * sizeof(char));
    in_size += ReadVectorFromFile(in, second);
    typedef std::pair<const std::string, std::vector<uint32_t>> IdxEntry;
    idx_.insert(IdxEntry(first, second));
  }

  return in_size;
}

}
