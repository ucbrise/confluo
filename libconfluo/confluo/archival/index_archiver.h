#ifndef CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_

#include "storage/encoder.h"
#include "aggregated_reflog.h"
#include "archival_headers.h"
#include "archival_metadata.h"
#include "schema/column.h"
#include "conf/configuration_params.h"
#include "index_log.h"
#include "io/incr_file_writer.h"
#include "schema/schema.h"

namespace confluo {
namespace archival {

template<encoding_type ENCODING>
class index_archiver {

 public:
  /**
   * Constructor
   * @param path directory to archive in
   * @param index index to archive
   * @param column column corresponding to the index
   */
  index_archiver(const std::string& path, index::radix_index* index, const column_t column)
      : index_(index),
        tail_(0),
        column_(column),
        writer_(path, "index_data", configuration_params::MAX_ARCHIVAL_FILE_SIZE) {
    writer_.init();
    writer_.close();
  }

  /**
   * Archive index up to a data log offset.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    writer_.open();
    byte_string min = column_.min().to_key(column_.index_bucket_size());
    byte_string max = column_.max().to_key(column_.index_bucket_size());
    auto reflogs = index_->range_lookup_reflogs(min, max);
    for (auto it = reflogs.begin(); it != reflogs.end(); it++) {
      auto& refs = *it;
      archive_reflog(it.key(), refs, offset);
    }
    writer_.close();
  }

 private:
  void archive_reflog(byte_string key, reflog& refs, size_t offset) {
    size_t reflog_idx = 0;
    size_t data_log_off = tail_;
    while (data_log_off < offset && reflog_idx < refs.size()) {
      read_only_reflog_ptr bucket_ptr;
      refs.ptr(reflog_idx, bucket_ptr);
      uint64_t* data = bucket_ptr.get().ptr_as<uint64_t>();
      auto* metadata = ptr_metadata::get(data);
      if (metadata->state_ != state_type::D_IN_MEMORY) {
        reflog_idx += reflog_constants::BUCKET_SIZE;
        continue;
      }
      if ((data_log_off = max_in_reflog_bucket(data)) < offset && data_log_off >= tail_) {
        archive_bucket(key, refs, reflog_idx, data, data_log_off);
        reflog_idx += reflog_constants::BUCKET_SIZE;
      }
    }
    tail_ = data_log_off;
 }

  /**
   * Archives a reflog bucket of a reflog corresponding to a radix tree key.
   * Archived bucket format: [key|bucket_index|bucket_size][bucket]
   * @param key key to which reflog belongs to in radix_tree
   * @param reflog reflog to which bucket belongs
   * @param idx reflog index at which bucket starts
   * @param bucket reflog bucket starting at idx
   * @param offset max data log offset in bucket
   */
  void archive_bucket(byte_string key, reflog& refs, size_t idx, uint64_t* bucket, size_t offset) {
    auto* metadata = ptr_metadata::get(bucket);
    auto raw_encoded_bucket = encoder::encode<uint64_t, ENCODING>(bucket);
    size_t bucket_size = std::min(reflog_constants::BUCKET_SIZE, refs.size() - idx);
    size_t enc_size = raw_encoded_bucket.size();

    auto archival_metadata = radix_tree_archival_metadata(key, idx, bucket_size);
    auto archival_header = index_archival_header(key, idx + bucket_size, offset);

    radix_tree_archival_metadata::append(archival_metadata, writer_);
    auto off = writer_.append<ptr_metadata, uint8_t>(metadata, 1, raw_encoded_bucket.get(), enc_size);
    writer_.update_metadata(archival_header.to_string());

    if (bucket_size < reflog_constants::BUCKET_SIZE) {
      void* enc_bucket = ALLOCATOR.mmap(off.path(), off.offset(), enc_size, state_type::D_ARCHIVED);
      refs.swap_bucket_ptr(idx, encoded_reflog_ptr(enc_bucket));
    }
  }

  static uint64_t max_in_reflog_bucket(uint64_t* bucket) {
    uint64_t max = 0;
    for (size_t i = 0; i < reflog_constants::BUCKET_SIZE && bucket[i] != limits::ulong_max; i++)
      max = std::max(max, bucket[i]);
    return max;
  }

  index::radix_index* index_;
  size_t tail_;
  column_t column_;
  incremental_file_writer writer_;

};

class index_load_utils {
 public:
  /**
   * Load index archived on disk.
   * @param reader stream to load from
   * @param index index to load into
   * @return data log offset until which radix tree has been archived
   */
  static size_t load(incremental_file_reader& reader, index::radix_index* index) {
    auto archival_header = index_archival_header(reader.read_metadata<std::string>());
    size_t key_size = archival_header.key_size();
    byte_string archival_tail_key = archival_header.archival_tail_key();
    size_t archival_tail = archival_header.reflog_archival_tail();
    size_t data_log_archival_tail = archival_header.data_log_archival_tail();

    size_t reflog_idx = 0;
    byte_string cur_key(std::string(key_size, '0'));

    while (reader.has_more() && (cur_key != archival_tail_key || reflog_idx != archival_tail)) {
      auto archival_metadata = radix_tree_archival_metadata::read(reader, key_size);
      cur_key = archival_metadata.key();
      reflog_idx = archival_metadata.reflog_index();

      incremental_file_offset off = reader.tell();
      ptr_metadata metadata = reader.read<ptr_metadata>();
      size_t bucket_size = metadata.data_size_;

      auto*& refs = index->get_or_create(cur_key);
      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), bucket_size, state_type::D_ARCHIVED);
      refs->init_bucket_ptr(reflog_idx, encoded_reflog_ptr(encoded_bucket));
      reader.advance<uint8_t>(bucket_size);
      reflog_idx += archival_metadata.bucket_size();
      refs->set_tail(reflog_idx + bucket_size);
      // TODO minor edge case with duplicate key/idx pairs
    }

    if (cur_key != archival_tail_key || reflog_idx != archival_tail) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    incr_file_utils::truncate_rest(reader.tell());
    return data_log_archival_tail;
  }
};

}
}


#endif /* CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_ */
