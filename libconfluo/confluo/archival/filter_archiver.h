#ifndef CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_

#include "aggregated_reflog.h"
#include "archival_headers.h"
#include "archival_metadata.h"
#include "conf/configuration_params.h"
#include "storage/encoder.h"
#include "filter.h"
#include "container/reflog.h"

namespace confluo {
namespace archival {

template<encoding_type ENCODING>
class filter_archiver {

 public:
//  static const uint64_t DELTA_NS = 60 * 1e9;

  filter_archiver(const std::string& path, monitor::filter* filter)
      : filter_(filter),
        refs_writer_(path, "filter_data", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        aggs_writer_(path, "filter_aggs", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        refs_tail_(0),
        ts_tail_(0) {
    refs_writer_.init();
    aggs_writer_.init();
    refs_writer_.close();
    aggs_writer_.close();
  }

  /**
   * Attempt to archive filter from the current archival tail up to a data log offset.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    auto reflogs = filter_->lookup_range_reflogs(ts_tail_, limits::long_max);
    refs_writer_.open();
    aggs_writer_.open();
    for (auto it = reflogs.begin(); it != reflogs.end(); ++it) {
      auto& refs = *it;
      byte_string key = it.key();
      ts_tail_ = key.template as<uint64_t>();
//      if (ts_tail_ > (time_utils::cur_ns() - DELTA_NS) / configuration_params::TIME_RESOLUTION_NS) {
//        break;
//      }
      size_t data_log_archival_tail = archive_reflog(key, refs, offset);
      if (refs_tail_ < refs.size()) {
        break;
      }
      archive_reflog_aggregates(key, refs, data_log_archival_tail);
      refs_tail_ = 0;
    }
    refs_writer_.close();
    aggs_writer_.close();
  }

 private:
  size_t archive_reflog(byte_string key, reflog& refs, size_t offset) {
    size_t data_log_archival_tail = 0;
    // TODO replace w/ bucket iterator
    while (data_log_archival_tail < offset && refs_tail_ < refs.size()) {
      read_only_reflog_ptr bucket_ptr;
      refs.ptr(refs_tail_, bucket_ptr);
      uint64_t* data = bucket_ptr.get().ptr_as<uint64_t>();
      auto* metadata = ptr_metadata::get(bucket_ptr.get().ptr());
      if (metadata->state_ != state_type::D_IN_MEMORY) {
        refs_tail_ += reflog_constants::BUCKET_SIZE;
        continue;
      }
      if ((data_log_archival_tail = max_in_reflog_bucket(data)) < offset) {
        archive_bucket(key, refs, data, offset);
        refs_tail_ += reflog_constants::BUCKET_SIZE;
      }
    }
    return data_log_archival_tail;
  }

  /**
   *
   * @param key
   * @param refs
   * @param bucket
   * @param offset
   */
  void archive_bucket(byte_string key, reflog& refs, uint64_t* bucket, size_t offset) {
    auto* metadata = ptr_metadata::get(bucket);
    auto encoded_bucket = encoder::encode<uint64_t, ENCODING>(bucket);
    size_t bucket_size = std::min(reflog_constants::BUCKET_SIZE, refs.size() - refs_tail_);
    size_t enc_size = encoded_bucket.size();

    auto archival_metadata = radix_tree_archival_metadata(key, refs_tail_, bucket_size);
    auto archival_header = filter_archival_header(key, refs_tail_ + bucket_size, offset);

    radix_tree_archival_metadata::append(archival_metadata, refs_writer_);
    auto off = refs_writer_.append<ptr_metadata, uint8_t>(metadata, 1, encoded_bucket.get(), enc_size);
    refs_writer_.update_metadata(archival_header.to_string());

    void* archived_bucket = ALLOCATOR.mmap(off.path(), off.offset(), enc_size, state_type::D_ARCHIVED);
    refs.swap_bucket_ptr(refs_tail_, encoded_reflog_ptr(archived_bucket));
  }

  /**
   * Archive aggregates of an aggregated reflog.
   * @param key radix tree key to which reflog belongs
   * @param reflog aggregated reflog
   * @param version version to get aggregates for
   */
  void archive_reflog_aggregates(byte_string key, aggregated_reflog& reflog, size_t version) {
    size_t num_aggs = reflog.num_aggregates();
    auto metadata = filter_aggregates_archival_metadata(key, version, num_aggs);
    filter_aggregates_archival_metadata::append(metadata, aggs_writer_);

    if (num_aggs > 0) {
      size_t alloc_size = sizeof(aggregate) * num_aggs;
      aggregate* archived_aggs = static_cast<aggregate*>(ALLOCATOR.alloc(alloc_size, state_type::D_ARCHIVED));
      for (size_t i = 0; i < num_aggs; i++) {
        numeric collapsed_aggregate = reflog.get_aggregate(i, version);
        aggs_writer_.append<data_type>(collapsed_aggregate.type());
        aggs_writer_.append<uint8_t>(collapsed_aggregate.data(), collapsed_aggregate.type().size);
        new (archived_aggs + i) aggregate(collapsed_aggregate.type(), D_SUM, 1);
        archived_aggs[i].update(0, collapsed_aggregate, version);
      }
      reflog.swap_aggregates(archived_aggs);
    }
    aggs_writer_.update_metadata(filter_aggregates_archival_header(key).to_string());
  }

  static uint64_t max_in_reflog_bucket(uint64_t* bucket) {
    uint64_t max = 0;
    for (size_t i = 0; i < reflog_constants::BUCKET_SIZE && bucket[i] != limits::ulong_max; i++)
      max = std::max(max, bucket[i]);
    return max;
  }

  monitor::filter* filter_;
  incremental_file_writer refs_writer_;
  incremental_file_writer aggs_writer_;

  size_t refs_tail_; // data in the current reflog up to this tail has been archived
  uint64_t ts_tail_; // reflogs in the filter up to this timestamp have been archived

};

class filter_load_utils {

public:
 /**
  * Load filter's reflogs archived on disk.
  * @param reader stream to load from
  * @param filter filter to load into
  * @return data log offset until which radix tree has been loaded
  */
 static size_t load_reflogs(incremental_file_reader& reader, filter::idx_t* filter) {
   auto archival_header = filter_archival_header(reader.read_metadata<std::string>());
   byte_string archival_tail_key = archival_header.radix_tree_key();
   size_t archival_tail = archival_header.reflog_index();
   size_t data_log_archival_tail = archival_header.data_log_offset();

   size_t reflog_idx = 0;
   byte_string cur_key(std::string(sizeof(uint64_t), '0'));

   while (reader.has_more() && (cur_key != archival_tail_key || reflog_idx != archival_tail)) {
     auto archival_metadata = radix_tree_archival_metadata::read(reader, sizeof(uint64_t));
     cur_key = archival_metadata.key();
     reflog_idx = archival_metadata.reflog_index();

     incremental_file_offset off = reader.tell();
     ptr_metadata metadata = reader.read<ptr_metadata>();
     size_t bucket_size = metadata.data_size_;

     auto*& refs = filter->get_or_create(cur_key);
     void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), bucket_size, state_type::D_ARCHIVED);
     refs->init_bucket_ptr(reflog_idx, encoded_reflog_ptr(encoded_bucket));
     refs->reserve(archival_metadata.bucket_size());
     reader.advance<uint8_t>(bucket_size);
     reflog_idx += archival_metadata.bucket_size();
   }

   if (cur_key != archival_tail_key || reflog_idx != archival_tail) {
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
 static size_t load_reflog_aggregates(incremental_file_reader& reader, radix_tree* tree) {
   auto archival_metadata = filter_aggregates_archival_header(reader.read_metadata<std::string>());
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

};

}
}

#endif /* CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_ */
