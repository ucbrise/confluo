#ifndef CONFLUO_ARCHIVAL_ARCHIVAL_UTILS_H_
#define CONFLUO_ARCHIVAL_ARCHIVAL_UTILS_H_

#include "aggregated_reflog.h"
#include "archival_metadata.h"
#include "storage/allocator.h"
#include "storage/encoder.h"
#include "io/incr_file_reader.h"
#include "io/incr_file_utils.h"
#include "io/incr_file_writer.h"
#include "container/reflog.h"

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
    size_t archival_tail_beg = start;
    size_t data_log_archival_tail = 0;
    // TODO replace w/ iterator

    while (data_log_archival_tail < data_log_stop && archival_tail_beg < reflog.size()) {
      reflog.ptr(archival_tail_beg, bucket_ptr);
      decoded_reflog_ptr_t decoded_ptr = bucket_ptr.decode_ptr();
      auto* metadata = ptr_metadata::get(bucket_ptr.get().ptr());
      uint64_t* data = decoded_ptr.get();

      if (metadata->state_ != state_type::D_IN_MEMORY) {
        archival_tail_beg += reflog_constants::BUCKET_SIZE;
        continue;
      }

      data_log_archival_tail = max_in_reflog_bucket(data);
      if (data_log_archival_tail >= data_log_stop) {
        break;
      }

      // TODO abstract into metadata as well
      size_t bucket_size = std::min(reflog_constants::BUCKET_SIZE, reflog.size() - archival_tail_beg);
      writer.append<uint8_t>(key.data(), key.size());
      writer.append<size_t>(archival_tail_beg);
      writer.append<size_t>(bucket_size);

      size_t encoded_size;
      raw_encoded_ptr_t raw_encoded_bucket = encoder::encode<uint64_t, ENCODING>(data, encoded_size);
      auto off = writer.append<ptr_metadata, uint8_t>(metadata, 1, raw_encoded_bucket.get(), encoded_size);
      auto archival_metadata = reflog_archival_metadata(key, archival_tail_beg + bucket_size, data_log_archival_tail);
      writer.update_metadata(archival_metadata.to_string());

      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), encoded_size, state_type::D_ARCHIVED);
      reflog.swap_bucket_ptr(archival_tail_beg, encoded_reflog_ptr_t(encoded_bucket));

      archival_tail_beg += bucket_size;
    }
    return archival_tail_beg;
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
    // TODO abstract into metadata
    writer.append<uint8_t>(radix_tree_key.data(), radix_tree_key.size());
    writer.append<size_t>(version);
    writer.append<size_t>(num_aggs);

    if (num_aggs > 0) {
      size_t alloc_size = sizeof(aggregate) * num_aggs;
      aggregate* archived_aggs = static_cast<aggregate*>(ALLOCATOR.alloc(alloc_size, state_type::D_ARCHIVED));

      for (size_t i = 0; i < num_aggs; i++) {
        numeric collapsed_aggregate = reflog.get_aggregate(i, version);
        writer.append<data_type>(collapsed_aggregate.type());
        writer.append<uint8_t>(collapsed_aggregate.data(), collapsed_aggregate.type().size);
        new (archived_aggs + i) aggregate(collapsed_aggregate.type(), D_SUM, 1);
        archived_aggs[i].update(0, collapsed_aggregate, version);
      }
      reflog.swap_aggregates(archived_aggs);
    }
    writer.update_metadata(reflog_aggregates_archival_metadata(radix_tree_key).to_string());
  }


  /**
   * Load radix tree archived on disk.
   * @param reader reflog reader
   * @param index index to load
   * @return data log offset until which radix tree has been archived
   */
  template<typename radix_tree>
  static size_t load(incremental_file_reader& reader, radix_tree* index) {
    auto archival_metadata = reflog_archival_metadata(reader.read_metadata<std::string>());
    size_t key_size = archival_metadata.key_size();
    byte_string archival_tail_key = archival_metadata.archival_tail_key();
    size_t archival_tail = archival_metadata.reflog_archival_tail();
    size_t data_log_archival_tail = archival_metadata.data_log_archival_tail();

    size_t cur_reflog_archival_idx = 0;
    byte_string cur_key(std::string(key_size, '0'));

    size_t i = 0;
    while (reader.has_more() && (cur_key != archival_tail_key || cur_reflog_archival_idx != archival_tail)) {

      cur_key = byte_string(reader.read(key_size));
      cur_reflog_archival_idx = reader.read<size_t>();
      size_t bucket_len = reader.read<size_t>();

      incremental_file_offset off = reader.tell();
      ptr_metadata metadata = reader.read<ptr_metadata>();
      size_t bucket_size = metadata.data_size_;

      auto*& refs = index->get_or_create(cur_key);
      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), bucket_size, state_type::D_ARCHIVED);
      refs->init_bucket_ptr(cur_reflog_archival_idx, encoded_reflog_ptr(encoded_bucket));
      refs->reserve(bucket_len);
      reader.advance<uint8_t>(bucket_size);
      cur_reflog_archival_idx += bucket_len;
    }

    if (cur_key != archival_tail_key || cur_reflog_archival_idx != archival_tail) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    incr_file_utils::truncate_rest(reader.tell());
    return data_log_archival_tail;
  }

  /**
   * Load aggregates of reflogs in radix tree archived on disk.
   * @param reader aggregates reader
   * @param tree radix tree
   * @return
   */
  template<typename radix_tree>
  static size_t load_aggregates(incremental_file_reader& reader, radix_tree* tree) {
    auto archival_metadata = reflog_aggregates_archival_metadata(reader.read_metadata<std::string>());
    size_t key_size = archival_metadata.key_size();
    byte_string archival_tail_key = archival_metadata.archival_tail_key();

    byte_string cur_key(std::string(key_size, '0'));

    while (reader.has_more() && cur_key != archival_tail_key) {
      // TODO abstract into metadata
      cur_key = byte_string(reader.read(key_size));
      size_t version = reader.read<size_t>();
      size_t num_aggs = reader.read<size_t>();

      if (num_aggs > 0) {
        size_t size = sizeof(aggregate) * num_aggs;
        aggregate* archived_aggs = static_cast<aggregate*>(ALLOCATOR.alloc(size, state_type::D_ARCHIVED));
        for (size_t i = 0; i < num_aggs; i++) {
          data_type type = reader.read<data_type>();
          std::string data = reader.read(type.size);
          numeric agg(type, &data[0]);
          new (archived_aggs + i) aggregate(agg.type(), D_SUM, 1);
          archived_aggs[i].update(0, agg, version);
        }
        tree->get_unsafe(cur_key)->init_aggregates(num_aggs, archived_aggs);
      }
    }

    if (cur_key != archival_tail_key) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    incr_file_utils::truncate_rest(reader.tell());
    auto last = tree->get(archival_tail_key);
    return *std::max_element(last->begin(), last->end());
  }

 private:
  static uint64_t max_in_reflog_bucket(uint64_t* data) {
    uint64_t max = 0;
    for (size_t i = 0; i < reflog_constants::BUCKET_SIZE && data[i] != limits::ulong_max; i++)
      max = std::max(max, data[i]);
    return max;
  }
};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVAL_UTILS_H_ */
