#ifndef CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_

#include "aggregated_reflog.h"
#include "archival_actions.h"
#include "archival_metadata.h"
#include "conf/configuration_params.h"
#include "storage/encoder.h"
#include "filter.h"
#include "io/incr_file_writer.h"
#include "io/incr_file_reader.h"
#include "container/reflog.h"

namespace confluo {
namespace archival {

class filter_archiver {

 public:
  /**
   * Constructor.
   * @param path directory to archive in
   * @param filter filter to archive
   */
  filter_archiver(const std::string& path, monitor::filter* filter)
      : filter_(filter),
        refs_writer_(path, "filter_data", archival_configuration_params::MAX_FILE_SIZE),
        aggs_writer_(path, "filter_aggs", archival_configuration_params::MAX_FILE_SIZE),
        refs_tail_(0),
        ts_tail_(0) {
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
      auto ts_tail_ns = ts_tail_ * configuration_params::TIME_RESOLUTION_NS;
      if (time_utils::cur_ns() - ts_tail_ns <
          archival_configuration_params::RECENCY_FILTER_RESTRICTION_WINDOW_NS) {
        break;
      }
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

  /**
   * Archive reflog corresponding to a key, up to an offset.
   * @param key radix tree key to which reflog belongs
   * @param refs reflog to archive
   * @param offset data log offset
   * @return data log archival tail
   */
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
   * Archive bucket.
   * @param key
   * @param refs
   * @param bucket
   * @param offset
   */
  void archive_bucket(byte_string key, reflog& refs, uint64_t* bucket, size_t offset) {
    auto* metadata = ptr_metadata::get(bucket);
    auto encoded_bucket = encoder::encode(bucket, archival_configuration_params::REFLOG_ENCODING_TYPE);
    size_t bucket_size = std::min(reflog_constants::BUCKET_SIZE, refs.size() - refs_tail_);
    size_t enc_size = encoded_bucket.size();

    auto archival_metadata = radix_tree_archival_metadata(key, refs_tail_, bucket_size);
    auto action = filter_archival_action(key, refs_tail_ + bucket_size, offset);

    radix_tree_archival_metadata::append(archival_metadata, refs_writer_);
    auto off = refs_writer_.append<ptr_metadata, uint8_t>(metadata, 1, encoded_bucket.get(), enc_size);
    refs_writer_.commit(action.to_string());

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
    aggs_writer_.commit(filter_aggregates_archival_action(key).to_string());
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
  * @param path path to load reflogs from
  * @param filter filter to load into
  * @return data log offset until which radix tree has been loaded
  */
 static size_t load_reflogs(const std::string& path, filter::idx_t* filter) {
   incremental_file_reader reader(path, "filter_data");
   size_t data_log_archival_tail = 0;
   while (reader.has_more()) {
     auto action = filter_archival_action(reader.read_action<std::string>());
     data_log_archival_tail = action.data_log_offset();

     auto archival_metadata = radix_tree_archival_metadata::read(reader, sizeof(uint64_t));
     byte_string cur_key = archival_metadata.key();
     size_t reflog_idx = archival_metadata.reflog_index();

     incremental_file_offset off = reader.tell();
     ptr_metadata metadata = reader.read<ptr_metadata>();
     size_t bucket_size = metadata.data_size_;

     auto*& refs = filter->get_or_create(cur_key);
     void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), bucket_size, state_type::D_ARCHIVED);
     refs->init_bucket_ptr(reflog_idx, encoded_reflog_ptr(encoded_bucket));
     refs->reserve(archival_metadata.bucket_size());
     reader.advance<uint8_t>(bucket_size);
   }
   reader.truncate(reader.tell(), reader.tell_transaction_log());
   return data_log_archival_tail;
 }

 /**
  * Load aggregates of reflogs in radix tree archived on disk.
  * @param path path to archived filter
  * @param tree radix tree
  * @return data log offset until which radix tree aggregates have been loaded
  */
 template<typename radix_tree>
 static size_t load_reflog_aggregates(const std::string& path, radix_tree* tree) {
   incremental_file_reader reader(path, "filter_aggs");
   byte_string cur_key;
   while (reader.has_more()) {
     auto action = filter_aggregates_archival_action(reader.read_action<std::string>());
     size_t key_size = action.key_size();
     auto archival_metadata = filter_aggregates_archival_metadata::read(reader);
     cur_key = archival_metadata.ts_block();
     size_t num_aggs = archival_metadata.num_aggregates();

     if (num_aggs > 0) {
       size_t size = sizeof(aggregate) * num_aggs;
       aggregate* archived_aggs = static_cast<aggregate*>(ALLOCATOR.alloc(size, state_type::D_ARCHIVED));
       for (size_t i = 0; i < num_aggs; i++) {
         data_type type = reader.read<data_type>();
         std::string data = reader.read(type.size);
         numeric agg(type, &data[0]);
         new (archived_aggs + i) aggregate(agg.type(), D_SUM, 1);
         archived_aggs[i].update(0, agg, archival_metadata.version());
       }
       tree->get_unsafe(cur_key)->init_aggregates(num_aggs, archived_aggs);
     }
   }
   reader.truncate(reader.tell(), reader.tell_transaction_log());
   auto last = tree->get(cur_key);
   return *std::max_element(last->begin(), last->end());
 }

};

}
}

#endif /* CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_ */
