#include "log_store.h"

#include <fstream>

namespace succinct {

LogStore::LogStore(uint32_t ngram_n, const char* path)
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

  data_ = (char *) mmap(NULL, kLogStoreSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd, 0);
  if (data_ == MAP_FAILED) {
    fprintf(stderr, "Could not mmap file.\n");
    throw -1;
  }
}

int LogStore::Append(const int64_t key, const std::string& value) {
  if (tail_ + value.length() > kLogStoreSize) {
    return -1;   // Data exceeds max chunk size
  }

  uint64_t end = tail_;

  {
    std::lock_guard<std::mutex> guard(append_mtx_);

    // Append value to log
    memcpy(data_ + end, value.c_str(), value.length());

#ifdef PERSIST_AFTER_EVERY_WRITE
    msync(data_ + tail_ - tail_ % page_size_, value.length(), MS_SYNC);
#endif

    // Update primary index
    keys_.push_back(key);
    value_offsets_.push_back(end);

#ifndef NON_CONCURRENT_WRITES
  }
#endif

    // Update secondary index
    for (int64_t i = std::max(static_cast<int64_t>(0),
                              static_cast<int64_t>(end - ngram_n_));
        i < end + value.length() - ngram_n_; i++) {
#ifdef USE_INT_HASH
      uint32_t ngram = Hash::simple_hash3(data_ + i);
#else
      std::string ngram;
      for (uint32_t off = 0; off < ngram_n_; off++) {
        ngram += data_[i + off];
      }
#endif
      {
#ifndef NON_CONCURRENT_WRITES
        std::lock_guard<std::mutex> guard(ngram_idx_[ngram].mtx_);
#endif
        ngram_idx_[ngram].offsets_.push_back(i);
      }
    }

    tail_ += value.length();
#ifdef NON_CONCURRENT_WRITES
  }
#endif

  return 0;
}

void LogStore::Get(std::string& value, const int64_t key) {
  int64_t pos = GetValueOffsetPos(key);

  if (pos < 0) {
    value = "INVALID";
    return;
  }

  int64_t start = value_offsets_[pos];
  uint32_t tail = tail_;
  int64_t end =
      (pos + 1 < value_offsets_.size()) ? value_offsets_[pos + 1] : tail;
  size_t len = end - start;

  value.assign(data_ + start, len);
}

void LogStore::Search(std::set<int64_t>& results, const std::string& query) {

  char *substr = (char *) query.c_str();
  char *suffix = substr + ngram_n_;
  bool skip_filter = (query.length() <= ngram_n_);
  size_t suffix_len = skip_filter ? 0 : query.length() - ngram_n_;
  uint32_t tail = tail_;

#ifdef USE_INT_HASH
  uint32_t prefix_ngram = Hash::simple_hash3(substr);
#else
  std::string prefix_ngram = query.substr(0, ngram_n_);
#endif

  auto& idx_off = ngram_idx_[prefix_ngram].offsets_;
  for (uint32_t i = 0; i < idx_off.size(); i++) {
    if (idx_off[i] < tail
        && (skip_filter
            || strncmp(data_ + idx_off[i] + ngram_n_, suffix, suffix_len) == 0)) {
      // TODO: Take care of query.length() < ngram_n_ case
      int64_t pos = GetKeyPos(idx_off[i]);
      if (pos >= 0)
        results.insert(keys_[pos]);
    }
  }
}

int64_t LogStore::Dump(const std::string& path) {
  int64_t out_size = 0;
  std::ofstream out(path);

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
  size_t ngram_idx_size = ngram_idx_.size();
  out.write(reinterpret_cast<const char *>(&(ngram_idx_size)), sizeof(size_t));
  out_size += sizeof(size_t);
  for (auto& entry : ngram_idx_) {
#ifdef USE_INT_HASH
    out.write(reinterpret_cast<const char *>(&(entry.first)), sizeof(uint32_t));
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

int64_t LogStore::Load(const std::string& path) {
  int64_t in_size = 0;
  std::ifstream in(path);

  // Read data
  in.read(reinterpret_cast<char *>(&tail_), sizeof(uint32_t));
  in_size += sizeof(uint32_t);
  in.read(reinterpret_cast<char *>(data_), tail_ * sizeof(char));
  in_size += (tail_ * sizeof(char));

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

}
