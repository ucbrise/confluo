#ifndef CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_
#define CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_

#include "types/byte_string.h"

namespace confluo {
namespace archival {

/**
 * Describes a monolog_linear archival action: archival
 * of a single bucket of a monolog_linear.
 */
class monolog_linear_archival_action {
 public:
  monolog_linear_archival_action()
      : tail_(0) {
  }

  monolog_linear_archival_action(size_t tail)
      : tail_(tail) {
  }

  /**
   *
   * @return offset up to which monolog is archived
   */
  size_t archival_tail() {
    return tail_;
  }

 private:
  size_t tail_;
};

/**
 * Describes a filter archival action: archival
 * of a single bucket of a reflog in a filter.
 */
class filter_archival_action {
 public:
  filter_archival_action(std::string action)
      : action_(action) {
  }

  filter_archival_action(byte_string radix_tree_key, size_t reflog_idx, size_t data_log_offset)
      : filter_archival_action(radix_tree_key.data(), reflog_idx, data_log_offset) {
  }

  filter_archival_action(uint8_t* radix_tree_key, size_t reflog_idx, size_t data_log_offset) {
    action_ = "";
    action_.append(reinterpret_cast<const char*>(radix_tree_key), sizeof(uint64_t));
    action_.append(reinterpret_cast<const char*>(&reflog_idx), sizeof(reflog_idx));
    action_.append(reinterpret_cast<const char*>(&data_log_offset), sizeof(data_log_offset));
  }

  std::string to_string() {
    return action_;
  }

  /**
   *
   * @return corresponding radix tree key of the reflog archived
   */
  byte_string radix_tree_key() {
    return byte_string(action_.substr(0, sizeof(uint64_t)));
  }

  /**
   *
   * @return offset up to which reflog has been archived
   */
  size_t reflog_archival_tail() {
    return *reinterpret_cast<const size_t*>(action_.c_str() + sizeof(uint64_t));
  }

  /**
   *
   * @return corresponding data log offset up to which reflog has been archived
   */
  size_t data_log_archival_tail() {
    return *reinterpret_cast<const size_t*>(action_.c_str() + sizeof(size_t) + sizeof(uint64_t));
  }

 private:
  std::string action_;

};

/**
 * Describes a filter aggregates archival action: archival
 * of a set of aggregates belonging to a reflog of the
 * filter that has already had its data archived.
 */
class filter_aggregates_archival_action {

 public:
  filter_aggregates_archival_action(std::string metadata)
      : action_(metadata) {
  }

  filter_aggregates_archival_action(byte_string byte_str)
      : filter_aggregates_archival_action(byte_str.data()) {
  }

  filter_aggregates_archival_action(uint8_t* archival_tail_key) {
    action_ = std::string(reinterpret_cast<const char*>(archival_tail_key), sizeof(uint64_t));
  }

  std::string to_string() {
    return action_;
  }

  /**
   *
   * @return radix_tree key corresponding to reflog that contains the aggregates
   */
  byte_string archival_tail_key() {
    return byte_string(action_);
  }

 private:
  std::string action_;

};

/**
 * Describes a index archival action: archival
 * of a single bucket of a reflog in an index.
 */
class index_archival_action {

 public:
  index_archival_action(std::string action)
      : action_(action) {
  }

  /**
   * Constructor.
   * @param byte_str key
   * @param archival_tail
   * @param data_log_tail
   */
  index_archival_action(byte_string byte_str, size_t archival_tail, size_t data_log_tail)
      : index_archival_action(byte_str.size(), byte_str.data(), archival_tail, data_log_tail) {
  }

  /**
   * Constructor.
   * @param key_size size of key
   * @param archival_tail_key key corresponding to reflog being archived
   * @param archival_tail offset up to which reflog has been archived
   * @param data_log_tail corresponding data log offset up to which reflog has been archived
   */
  index_archival_action(size_t key_size, uint8_t* archival_tail_key,
                        size_t archival_tail, size_t data_log_tail) {
    action_ = "";
    action_.append(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
    action_.append(reinterpret_cast<const char*>(archival_tail_key), key_size);
    action_.append(reinterpret_cast<const char*>(&archival_tail), sizeof(archival_tail));
    action_.append(reinterpret_cast<const char*>(&data_log_tail), sizeof(data_log_tail));
  }

  std::string to_string() {
    return action_;
  }

  /**
   *
   * @return size of key type of the index
   */
  size_t key_size() {
    return *reinterpret_cast<const size_t*>(action_.c_str());
  }

  /**
   *
   * @return corresponding radix tree key of the reflog archived
   */
  byte_string radix_tree_key() {
    return byte_string(action_.substr(sizeof(size_t), key_size()));
  }

  /**
   *
   * @return offset up to which reflog has been archived
   */
  size_t reflog_archival_tail() {
    return *reinterpret_cast<const size_t*>(action_.c_str() + sizeof(size_t) + this->key_size());
  }

  /**
   *
   * @return corresponding data log offset up to which reflog has been archived
   */
  size_t data_log_archival_tail() {
    return *reinterpret_cast<const size_t*>(action_.c_str() + 2 * sizeof(size_t) + this->key_size());
  }

 private:
  std::string action_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_ */
