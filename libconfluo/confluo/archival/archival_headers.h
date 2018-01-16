#ifndef CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_
#define CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_

#include "types/byte_string.h"

namespace confluo {
namespace archival {

class monolog_linear_archival_header {
 public:
  monolog_linear_archival_header()
      : tail_(0) {
  }

  monolog_linear_archival_header(size_t tail)
      : tail_(tail) {
  }

  size_t archival_tail() {
    return tail_;
  }

 private:
  size_t tail_;
};

class filter_archival_header {
 public:
  filter_archival_header(std::string metadata)
      : metadata_(metadata) {
  }

  filter_archival_header(byte_string radix_tree_key, size_t reflog_idx, size_t data_log_offset)
      : filter_archival_header(radix_tree_key.data(), reflog_idx, data_log_offset) {
  }

  filter_archival_header(uint8_t* radix_tree_key, size_t reflog_idx, size_t data_log_offset) {
    metadata_ = "";
    metadata_.append(reinterpret_cast<const char*>(radix_tree_key), sizeof(uint64_t));
    metadata_.append(reinterpret_cast<const char*>(&reflog_idx), sizeof(reflog_idx));
    metadata_.append(reinterpret_cast<const char*>(&data_log_offset), sizeof(data_log_offset));
  }

  std::string to_string() {
    return metadata_;
  }

  byte_string radix_tree_key() {
    return byte_string(metadata_.substr(0, sizeof(uint64_t)));
  }

  size_t reflog_index() {
    return *reinterpret_cast<const size_t*>(metadata_.c_str() + sizeof(uint64_t));
  }

  size_t data_log_offset() {
    return *reinterpret_cast<const size_t*>(metadata_.c_str() + sizeof(size_t) + sizeof(uint64_t));
  }

 private:
  std::string metadata_;

};

class filter_aggregates_archival_header {

 public:
  filter_aggregates_archival_header(std::string metadata)
      : metadata_(metadata) {
  }

  filter_aggregates_archival_header(byte_string byte_str)
      : filter_aggregates_archival_header(byte_str.size(), byte_str.data()) {
  }

  filter_aggregates_archival_header(size_t key_size, uint8_t* archival_tail_key) {
    metadata_ = "";
    metadata_.append(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
    metadata_.append(reinterpret_cast<const char*>(archival_tail_key), key_size);
  }

  std::string to_string() {
    return metadata_;
  }

  size_t key_size() {
    return *reinterpret_cast<const size_t*>(metadata_.c_str());
  }

  byte_string archival_tail_key() {
    return byte_string(metadata_.substr(sizeof(size_t), this->key_size()));
  }

 private:
  std::string metadata_;

};

class index_archival_header {

 public:
  index_archival_header(std::string metadata)
      : metadata_(metadata) {
  }

  index_archival_header(byte_string byte_str, size_t archival_tail, size_t data_log_tail)
      : index_archival_header(byte_str.size(), byte_str.data(), archival_tail, data_log_tail) {
  }

  index_archival_header(size_t key_size, uint8_t* archival_tail_key,
                        size_t archival_tail, size_t data_log_tail) {
    metadata_ = "";
    metadata_.append(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
    metadata_.append(reinterpret_cast<const char*>(archival_tail_key), key_size);
    metadata_.append(reinterpret_cast<const char*>(&archival_tail), sizeof(archival_tail));
    metadata_.append(reinterpret_cast<const char*>(&data_log_tail), sizeof(data_log_tail));
  }

  std::string to_string() {
    return metadata_;
  }

  size_t key_size() {
    return *reinterpret_cast<const size_t*>(metadata_.c_str());
  }

  byte_string archival_tail_key() {
    return byte_string(metadata_.substr(sizeof(size_t), key_size()));
  }

  size_t reflog_archival_tail() {
    return *reinterpret_cast<const size_t*>(metadata_.c_str() + sizeof(size_t) + this->key_size());
  }

  size_t data_log_archival_tail() {
    return *reinterpret_cast<const size_t*>(metadata_.c_str() + 2 * sizeof(size_t) + this->key_size());
  }

 private:
  std::string metadata_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_ */
