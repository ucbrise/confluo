#ifndef CONFLUO_ARCHIVAL_METADATA_H_
#define CONFLUO_ARCHIVAL_METADATA_H_

#include "types/byte_string.h"
#include "io/incremental_file_reader.h"
#include "io/incremental_file_writer.h"

namespace confluo {
namespace archival {

// TODO distinction between metadata & transaction log actions:
// most recent commit should describe everything before it, and
// make recovery possible. Everything else should go here.
// Need to separate it out better.

class filter_aggregates_archival_metadata {
 public:
  /**
   * Constructor.
   * @param ts_block timestamp block corresponding to reflog containing aggregates
   * @param version aggregates version
   * @param num_aggs number of aggregates
   */
  filter_aggregates_archival_metadata(byte_string ts_block, size_t version, size_t num_aggs)
      : ts_block_(ts_block), version_(version), num_aggs_(num_aggs) {
  }

  /**
   * Read metadata from archives reader.
   * @param reader archives reader
   * @return metadata
   */
  static filter_aggregates_archival_metadata read(incremental_file_reader& reader) {
    byte_string ts_block = byte_string(reader.read(sizeof(uint64_t)));
    size_t version = reader.read<size_t>();
    size_t num_aggs = reader.read<size_t>();
    return filter_aggregates_archival_metadata(ts_block, version, num_aggs);
  }

  /**
   * Append metadata to file.
   * @param metadata archival metadata
   * @param writer archives writer
   */
  static void append(filter_aggregates_archival_metadata metadata, incremental_file_writer& writer) {
    writer.append<uint8_t>(metadata.ts_block_.data(), sizeof(uint64_t));
    writer.append<size_t>(metadata.version_);
    writer.append<size_t>(metadata.num_aggs_);
  }

  /**
   *
   * @return timestamp block corresponding to reflog containing aggregates
   */
  byte_string ts_block() {
    return ts_block_;
  }

  /**
   *
   * @return aggregates version
   */
  size_t version() {
    return version_;
  }

  /**
   *
   * @return number of aggregates
   */
  size_t num_aggregates() {
    return num_aggs_;
  }

 private:
  byte_string ts_block_;
  size_t version_;
  size_t num_aggs_;
};

class radix_tree_archival_metadata {
 public:
  /**
   * Constructor.
   * @param key radix tree key
   * @param reflog_idx reflog index of archived bucket
   * @param bucket_size bucket size
   */
  radix_tree_archival_metadata(byte_string key, size_t reflog_idx, size_t bucket_size)
      : key_(key), reflog_index_(reflog_idx), bucket_size_(bucket_size) {
  }

  /**
   * Read metadata from archives reader.
   * @param reader archives reader
   * @param key_size size of radix tree key
   * @return metadata
   */
  static radix_tree_archival_metadata read(incremental_file_reader& reader, size_t key_size) {
    byte_string key = byte_string(reader.read(key_size));
    size_t reflog_idx = reader.read<size_t>();
    size_t bucket_len = reader.read<size_t>();
    return radix_tree_archival_metadata(key, reflog_idx, bucket_len);
  }

  /**
   * Append metadata to file.
   * @param metadata archival metadata
   * @param writer archives writer
   */
  static void append(radix_tree_archival_metadata metadata, incremental_file_writer& writer) {
    writer.append<uint8_t>(metadata.key_.data(), metadata.key_.size());
    writer.append<size_t>(metadata.reflog_index_);
    writer.append<size_t>(metadata.bucket_size_);
  }

  /**
   *
   * @return corresponding radix tree key of the reflog archived
   */
  byte_string key() {
    return key_;
  }

  /**
   *
   * @return reflog index of archived bucket
   */
  size_t reflog_index() {
    return reflog_index_;
  }

  /**
   *
   * @return size of archived bucket
   */
  size_t bucket_size() {
    return bucket_size_;
  }

 private:
  byte_string key_; // key at which reflog is stored in radix tree
  size_t reflog_index_; // start index of bucket in reflog
  size_t bucket_size_; // size of bucket
};

}
}

#endif /* CONFLUO_ARCHIVAL_METADATA_H_ */
