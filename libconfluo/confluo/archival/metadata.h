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

class reflog_aggregates_archival_header {

 public:
  reflog_aggregates_archival_header(std::string header)
      : header_(header) {
  }

  // TODO make symmetric
  reflog_aggregates_archival_header(size_t key_size, uint8_t* archival_tail_key) {
    header_ = "";
    header_.append(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
    header_.append(reinterpret_cast<const char*>(archival_tail_key), key_size);
  }

  std::string to_string() {
    return header_;
  }

  size_t key_size() {
    return *reinterpret_cast<const size_t*>(header_.c_str());
  }

  byte_string archival_tail_key() {
    return byte_string(header_.substr(sizeof(size_t), this->key_size()));
  }

 private:
  std::string header_;

};

class reflog_archival_header {

 public:
  reflog_archival_header(std::string header)
      : header_(header) {
  }

  // TODO make symmetric
  reflog_archival_header(size_t key_size, uint8_t* archival_tail_key, size_t archival_tail) {
    header_ = "";
    header_.append(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
    header_.append(reinterpret_cast<const char*>(archival_tail_key), key_size);
    header_.append(reinterpret_cast<const char*>(&archival_tail), sizeof(archival_tail));
  }

  std::string to_string() {
    return header_;
  }

  size_t key_size() {
    return *reinterpret_cast<const size_t*>(header_.c_str());
  }

  byte_string archival_tail_key() {
    return byte_string(header_.substr(sizeof(size_t), key_size()));
  }

  size_t reflog_archival_tail() {
    return *reinterpret_cast<const size_t*>(header_.c_str() + sizeof(size_t) + this->key_size());
  }

 private:
  std::string header_;

};

//template<typename T>
//typedef struct reflog_archival_metadata {
//  T archival_tail_key_;
//  size_t archival_tail_reflog_off_;
//};
//
//template<typename T>
//typedef struct reflog_aggregates_metadata {
//  T archival_tail_key_;
//  size_t num_aggregates_;
//};

}
}

#endif /* CONFLUO_ARCHIVAL_METADATA_H_ */
