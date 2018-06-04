#include "archival/archival_actions.h"

namespace confluo {
namespace archival {

monolog_linear_archival_action::monolog_linear_archival_action()
    : tail_(0) {
}

monolog_linear_archival_action::monolog_linear_archival_action(size_t tail)
    : tail_(tail) {
}

size_t monolog_linear_archival_action::archival_tail() {
  return tail_;
}

filter_archival_action::filter_archival_action(std::string action)
    : action_(action) {
}

filter_archival_action::filter_archival_action(byte_string radix_tree_key, size_t reflog_idx, size_t data_log_offset)
    : filter_archival_action(radix_tree_key.data(), reflog_idx, data_log_offset) {
}

filter_archival_action::filter_archival_action(uint8_t *radix_tree_key, size_t reflog_idx, size_t data_log_offset) {
  action_ = "";
  action_.append(reinterpret_cast<const char *>(radix_tree_key), sizeof(uint64_t));
  action_.append(reinterpret_cast<const char *>(&reflog_idx), sizeof(reflog_idx));
  action_.append(reinterpret_cast<const char *>(&data_log_offset), sizeof(data_log_offset));
}

std::string filter_archival_action::to_string() {
  return action_;
}

byte_string filter_archival_action::radix_tree_key() {
  return byte_string(action_.substr(0, sizeof(uint64_t)));
}

size_t filter_archival_action::reflog_archival_tail() {
  return *reinterpret_cast<const size_t *>(action_.c_str() + sizeof(uint64_t));
}

size_t filter_archival_action::data_log_archival_tail() {
  return *reinterpret_cast<const size_t *>(action_.c_str() + sizeof(size_t) + sizeof(uint64_t));
}

filter_aggregates_archival_action::filter_aggregates_archival_action(std::string metadata)
    : action_(metadata) {
}

filter_aggregates_archival_action::filter_aggregates_archival_action(byte_string byte_str)
    : filter_aggregates_archival_action(byte_str.data()) {
}

filter_aggregates_archival_action::filter_aggregates_archival_action(uint8_t *archival_tail_key) {
  action_ = std::string(reinterpret_cast<const char *>(archival_tail_key), sizeof(uint64_t));
}

std::string filter_aggregates_archival_action::to_string() {
  return action_;
}

byte_string filter_aggregates_archival_action::archival_tail_key() {
  return byte_string(action_);
}

index_archival_action::index_archival_action(std::string action)
    : action_(action) {
}

index_archival_action::index_archival_action(byte_string byte_str, size_t archival_tail, size_t data_log_tail)
    : index_archival_action(byte_str.size(), byte_str.data(), archival_tail, data_log_tail) {
}

index_archival_action::index_archival_action(size_t key_size,
                                             uint8_t *archival_tail_key,
                                             size_t archival_tail,
                                             size_t data_log_tail) {
  action_ = "";
  action_.append(reinterpret_cast<const char *>(&key_size), sizeof(key_size));
  action_.append(reinterpret_cast<const char *>(archival_tail_key), key_size);
  action_.append(reinterpret_cast<const char *>(&archival_tail), sizeof(archival_tail));
  action_.append(reinterpret_cast<const char *>(&data_log_tail), sizeof(data_log_tail));
}

std::string index_archival_action::to_string() {
  return action_;
}

size_t index_archival_action::key_size() {
  return *reinterpret_cast<const size_t *>(action_.c_str());
}

byte_string index_archival_action::radix_tree_key() {
  return byte_string(action_.substr(sizeof(size_t), key_size()));
}

size_t index_archival_action::reflog_archival_tail() {
  return *reinterpret_cast<const size_t *>(action_.c_str() + sizeof(size_t) + this->key_size());
}

size_t index_archival_action::data_log_archival_tail() {
  return *reinterpret_cast<const size_t *>(action_.c_str() + 2 * sizeof(size_t) + this->key_size());
}

}
}