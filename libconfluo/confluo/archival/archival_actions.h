#ifndef CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_
#define CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_

#include <cstdint>
#include <string>
#include "types/byte_string.h"

namespace confluo {
namespace archival {

/**
 * Describes a monolog_linear archival action: archival
 * of a single bucket of a monolog_linear.
 */
class monolog_linear_archival_action {
 public:
  monolog_linear_archival_action();

  monolog_linear_archival_action(size_t tail);

  /**
   *
   * @return offset up to which monolog is archived
   */
  size_t archival_tail();

 private:
  size_t tail_;
};

/**
 * Describes a filter archival action: archival
 * of a single bucket of a reflog in a filter.
 */
class filter_archival_action {
 public:
  filter_archival_action(std::string action);

  filter_archival_action(byte_string radix_tree_key, size_t reflog_idx, size_t data_log_offset);

  filter_archival_action(uint8_t *radix_tree_key, size_t reflog_idx, size_t data_log_offset);

  std::string to_string();

  /**
   *
   * @return corresponding radix tree key of the reflog archived
   */
  byte_string radix_tree_key();

  /**
   *
   * @return offset up to which reflog has been archived
   */
  size_t reflog_archival_tail();

  /**
   *
   * @return corresponding data log offset up to which reflog has been archived
   */
  size_t data_log_archival_tail();

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
  filter_aggregates_archival_action(std::string metadata);

  filter_aggregates_archival_action(byte_string byte_str);

  filter_aggregates_archival_action(uint8_t *archival_tail_key);

  std::string to_string();

  /**
   *
   * @return radix_tree key corresponding to reflog that contains the aggregates
   */
  byte_string archival_tail_key();

 private:
  std::string action_;

};

/**
 * Describes a index archival action: archival
 * of a single bucket of a reflog in an index.
 */
class index_archival_action {

 public:
  index_archival_action(std::string action);

  /**
   * Constructor.
   * @param byte_str key
   * @param archival_tail
   * @param data_log_tail
   */
  index_archival_action(byte_string byte_str, size_t archival_tail, size_t data_log_tail);

  /**
   * Constructor.
   * @param key_size size of key
   * @param archival_tail_key key corresponding to reflog being archived
   * @param archival_tail offset up to which reflog has been archived
   * @param data_log_tail corresponding data log offset up to which reflog has been archived
   */
  index_archival_action(size_t key_size, uint8_t *archival_tail_key,
                        size_t archival_tail, size_t data_log_tail);

  std::string to_string();

  /**
   *
   * @return size of key type of the index
   */
  size_t key_size();

  /**
   *
   * @return corresponding radix tree key of the reflog archived
   */
  byte_string radix_tree_key();

  /**
   *
   * @return offset up to which reflog has been archived
   */
  size_t reflog_archival_tail();

  /**
   *
   * @return corresponding data log offset up to which reflog has been archived
   */
  size_t data_log_archival_tail();

 private:
  std::string action_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVAL_HEADERS_H_ */
