#ifndef CONFLUO_ARCHIVAL_METADATA_H_
#define CONFLUO_ARCHIVAL_METADATA_H_

namespace confluo {
namespace archival {

class monolog_linear_archival_metadata {
 public:
  monolog_linear_archival_metadata()
      : tail_(0) {
  }

  monolog_linear_archival_metadata(size_t tail)
      : tail_(tail) {
  }

  size_t archival_tail() {
    return tail_;
  }

 private:
  size_t tail_;
};

class reflog_aggregates_archival_metadata {

 public:
  reflog_aggregates_archival_metadata(std::string metadata)
      : metadata_(metadata) {
  }

  reflog_aggregates_archival_metadata(byte_string byte_str)
      : reflog_aggregates_archival_metadata(byte_str.size(), byte_str.data()) {
  }

  reflog_aggregates_archival_metadata(size_t key_size, uint8_t* archival_tail_key) {
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

class reflog_archival_metadata {

 public:
  reflog_archival_metadata(std::string metadata)
      : metadata_(metadata) {
  }

  reflog_archival_metadata(byte_string byte_str, size_t archival_tail)
      : reflog_archival_metadata(byte_str.size(), byte_str.data(), archival_tail) {
  }

  reflog_archival_metadata(size_t key_size, uint8_t* archival_tail_key, size_t archival_tail) {
    metadata_ = "";
    metadata_.append(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
    metadata_.append(reinterpret_cast<const char*>(archival_tail_key), key_size);
    metadata_.append(reinterpret_cast<const char*>(&archival_tail), sizeof(archival_tail));
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

 private:
  std::string metadata_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_METADATA_H_ */
