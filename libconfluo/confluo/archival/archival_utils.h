#ifndef CONFLUO_ARCHIVAL_ARCHIVAL_UTILS_H_
#define CONFLUO_ARCHIVAL_ARCHIVAL_UTILS_H_

#include "aggregated_reflog.h"
#include "archival_metadata.h"
#include "storage/allocator.h"
#include "storage/encoder.h"
#include "container/reflog.h"
#include "io/incr_file_writer.h"

namespace confluo {
namespace archival {

using namespace ::utils;
using namespace storage;

class radix_tree_archival_utils {
 public:
  typedef storage::read_only_encoded_ptr<uint64_t> reflog_bucket_ptr_t;
  typedef storage::decoded_ptr<uint64_t> decoded_reflog_ptr_t;
  typedef encoded_ptr<uint64_t> encoded_reflog_ptr_t;
  typedef encoder::raw_encoded_ptr raw_encoded_ptr_t;

  /**
   * Archives a reflog corresponding to a radix tree key.
   * Archival Format: [key][bucket_index][bucket]...
   * @param key key to which reflog belongs to in radix_tree
   * @param reflog reflog to archive
   * @param writer archival output
   * @param start reflog offset to start archival from
   * @param data_log_stop data log offset to archive up until
   * @return reflog offset archived up to
   */
  template<encoding_type ENCODING>
  static size_t archive_reflog(byte_string key, reflog& reflog, incremental_file_writer& writer,
                               size_t start, size_t data_log_stop) {
    reflog_bucket_ptr_t bucket_ptr;
    size_t archival_tail = start;
    size_t data_log_archival_tail = 0;
    // TODO replace w/ iterator
    while (data_log_archival_tail < data_log_stop && archival_tail < reflog.size()) {
      reflog.ptr(archival_tail, bucket_ptr);
      decoded_reflog_ptr_t decoded_ptr = bucket_ptr.decode_ptr();
      auto* metadata = ptr_metadata::get(bucket_ptr.get().ptr());
      uint64_t* data = decoded_ptr.get();

      if (metadata->state_ != state_type::D_IN_MEMORY) {
        archival_tail += reflog_constants::BUCKET_SIZE;
        continue;
      }

      data_log_archival_tail = max_in_reflog_bucket(data);
      if (data_log_archival_tail >= data_log_stop) {
        break;
      }

      // TODO abstract into metadata as well
      writer.append<uint8_t>(key.data(), key.size());
      writer.append<size_t>(archival_tail);

      size_t encoded_size;
      raw_encoded_ptr_t raw_encoded_bucket = encoder::encode<uint64_t, ENCODING>(data, encoded_size);
      auto off = writer.append<ptr_metadata, uint8_t>(metadata, 1, raw_encoded_bucket.get(), encoded_size);
      writer.update_metadata(reflog_archival_metadata(key, archival_tail).to_string());

      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), encoded_size, state_type::D_ARCHIVED);
      reflog.swap_bucket_ptr(archival_tail, encoded_reflog_ptr_t(encoded_bucket));

      archival_tail += reflog_constants::BUCKET_SIZE;
    }
    return archival_tail;
  }

  /**
   * Archive aggregates of an aggregated reflog.
   * @param radix_tree_key key to which reflog belongs to in radix_tree
   * @param reflog aggregated reflog
   * @param writer archival output
   * @param version version to get aggregates for
   */
  static void archive_reflog_aggregates(byte_string radix_tree_key, aggregated_reflog& reflog,
                                        size_t version, incremental_file_writer& writer) {

    size_t num_aggs = reflog.num_aggregates();
    numeric* collapsed_aggregates = new numeric[num_aggs];
    for (size_t i = 0; i < num_aggs; i++) {
      collapsed_aggregates[i] = reflog.get_aggregate(i, version);
    }

    // TODO abstract into metadata
    writer.append<uint8_t>(radix_tree_key.data(), radix_tree_key.size());
    writer.append<size_t>(version);
    writer.append<size_t>(num_aggs);
    writer.append<numeric>(collapsed_aggregates, num_aggs);
    writer.update_metadata(reflog_aggregates_archival_metadata(radix_tree_key).to_string());

    size_t alloc_size = sizeof(aggregate) * num_aggs;
    aggregate* archived_aggs = static_cast<aggregate*>(ALLOCATOR.alloc(alloc_size, state_type::D_ARCHIVED));
    for (size_t i = 0; i < num_aggs; i++) {
      new (archived_aggs + i) aggregate(collapsed_aggregates[i].type(), D_SUM, 1);
      archived_aggs[i].update(0, collapsed_aggregates[i], version);
    }

    reflog.swap_aggregates(archived_aggs);
    delete[] collapsed_aggregates;
  }

 private:
  static uint64_t max_in_reflog_bucket(uint64_t* data) {
    uint64_t max = 0;
    for (size_t i = 0; i < reflog_constants::BUCKET_SIZE && data[i] != limits::ulong_max; i++)
      max = std::max(max, data[i]);
    return max;
  }
};

class archival_utils {
 public:
  /**
   * Archive buckets of a monolog_linear until a given offset.
   * Format: [bucket][bucket][bucket]...
   * @param monolog monolog_linear to archive
   * @param writer archival output
   * @param start monolog offset to start archival from
   * @param stop monolog offset to end archival at
   * @return monolog offset archived up to
   */
  template<typename T, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUFFER_SIZE, encoding_type ENCODING>
  static size_t archive_monolog_linear(monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUFFER_SIZE>* monolog,
                                       incremental_file_writer& writer, size_t start, size_t stop) {
    // TODO replace with bucket iterator later
    size_t archival_tail = start;
    storage::read_only_encoded_ptr<T> bucket_ptr;
    while (archival_tail < stop) {
      monolog->ptr(archival_tail, bucket_ptr);
      auto decoded_ptr = bucket_ptr.decode_ptr();
      auto* metadata = storage::ptr_metadata::get(bucket_ptr.get().ptr());
      T* data = decoded_ptr.get();

      if (metadata->state_ != state_type::D_IN_MEMORY) {
        archival_tail += BUCKET_SIZE;
        continue;
      }

      size_t encoded_size;
      encoder::raw_encoded_ptr raw_encoded_bucket = encoder::encode<T, ENCODING>(data, encoded_size);
      auto off = writer.append<ptr_metadata, uint8_t>(metadata, 1, raw_encoded_bucket.get(), encoded_size);

      auto archival_metadata = monolog_linear_archival_metadata(archival_tail);
      writer.update_metadata<monolog_linear_archival_metadata>(archival_metadata);

      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), encoded_size, state_type::D_ARCHIVED);
      monolog->swap_bucket_ptr(archival_tail / BUCKET_SIZE, encoded_ptr<T>(encoded_bucket));
      archival_tail += BUCKET_SIZE;
    }
    return archival_tail;
  }
};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVAL_UTILS_H_ */
