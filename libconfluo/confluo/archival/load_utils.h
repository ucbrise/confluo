#ifndef CONFLUO_ARCHIVAL_LOAD_UTILS_H_
#define CONFLUO_ARCHIVAL_LOAD_UTILS_H_

#include "storage/allocator.h"
#include "container/data_log.h"
#include "filter.h"
#include "filter_log.h"
#include "io/incr_file_reader.h"
#include "io/incr_file_writer.h"
#include "index_log.h"
#include "container/reflog.h"

namespace confluo {
namespace archival {

using namespace ::utils;
using namespace storage;

class load_utils {

 public:

  template<typename T, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUF_SIZE>
  static void load_monolog_linear(const std::string& path,
                                  monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE>& log) {
    incremental_file_reader reader(path, "data_log", ".dat");
    auto archival_metadata = reader.read_metadata<monolog_linear_archival_metadata>();

    size_t load_offset = 0;
    while (reader.has_more() && load_offset < archival_metadata.archival_tail()) {
      ptr_metadata metadata = reader.read<ptr_metadata>();
      size_t size = metadata.data_size_;
      incremental_file_offset off = reader.tell();

      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), size, state_type::D_ARCHIVED);
      log.init_bucket_ptr(load_offset / BUCKET_SIZE, encoded_ptr<T>(encoded_bucket));

      log.reserve(BUCKET_SIZE);
      reader.advance<uint8_t>(size);
      load_offset += BUCKET_SIZE;
    }

    if (load_offset != archival_metadata.archival_tail()) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    if (reader.has_more()) {
      // TODO truncate all files after as wells
      incremental_file_offset off = reader.tell();
      std::string path = off.path();
      file_utils::truncate_file(path, off.offset());
    }
  }

  static void load_replay_filter_log(const std::string& path, filter_log& filters,
                                     data_log& log, schema_t& schema) {
    for (size_t i = 0; i < filters.size(); i++) {
      monitor::filter* filter = filters[i];
      if (filter->is_valid()) {
        size_t data_log_archival_tail = load_filter(path, filter);
        replay_filter(filter, log, schema, data_log_archival_tail);
      }
    }
  }

  static void load_replay_index_log(const std::string& path, index_log& indexes,
                                    data_log& log, schema_t& schema) {
    for (size_t i = 0; i < schema.size(); i++) {
      auto& col = schema[i];
      if (col.is_indexed()) {
        auto* index = indexes[col.index_id()];
        size_t data_log_archival_tail = load_index(path, index);
        replay_index(index, col.index_id(), log, schema, data_log_archival_tail);
      }
    }
  }

  static size_t load_filter(const std::string& path, monitor::filter* filter) {
    incremental_file_reader reflog_reader(path, "filter_data", ".dat");
    incremental_file_reader aggs_reader(path, "filter_aggs", ".dat");
    filter::idx_t* tree = filter->data();
    size_t refs_data_log_archival_tail = load_radix_tree<filter::idx_t>(reflog_reader, tree);
    size_t aggs_data_log_archival_tail = load_radix_tree_reflog_aggs<filter::idx_t>(aggs_reader, tree);
    if (refs_data_log_archival_tail != aggs_data_log_archival_tail) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    return refs_data_log_archival_tail;
  }

  static size_t load_index(const std::string& path, index::radix_index* index) {
    incremental_file_reader reflog_reader(path, "index_data", ".dat");
    return load_radix_tree<index::radix_index>(reflog_reader, index);
  }

  static void replay_filter(monitor::filter* filter, data_log& log, schema_t& schema, size_t start_off) {
    uint8_t* data_buf = new uint8_t[schema.record_size()];
    for (size_t j = start_off; j < log.size(); j++) {
      // TODO this is inefficient, amortize with iterator
      log.read(j, data_buf, schema.record_size());
      filter->update(schema.apply_unsafe(j, data_buf));
    }
    delete[] data_buf;
  }

  static void replay_index(index::radix_index* index, uint16_t id, data_log& log,
                           schema_t& schema, size_t start_off) {
    uint8_t* data_buf = new uint8_t[schema.record_size()];
    for (size_t j = start_off; j < log.size(); j += schema.record_size()) {
      // TODO this is inefficient, amortize with iterator
      log.read(j, data_buf, schema.record_size());
      record_t r = schema.apply_unsafe(j, data_buf);
      index->insert(r[id].get_key(), j);
    }
    delete[] data_buf;
  }

  template<typename radix_tree>
  static size_t load_radix_tree(incremental_file_reader& reflog_reader, radix_tree* index) {

    auto archival_metadata = reflog_archival_metadata(reflog_reader.read_metadata<std::string>());
    size_t key_size = archival_metadata.key_size();
    byte_string archival_tail_key = archival_metadata.archival_tail_key();
    size_t archival_tail = archival_metadata.reflog_archival_tail();

    size_t cur_reflog_idx = 0;
    byte_string cur_key;

    while (reflog_reader.has_more() && cur_key != archival_tail_key && cur_reflog_idx != archival_tail) {

      cur_key = byte_string(reflog_reader.read(key_size));
      cur_reflog_idx = reflog_reader.read<size_t>();

      ptr_metadata metadata = reflog_reader.read<ptr_metadata>();
      size_t bucket_size = metadata.data_size_;
      incremental_file_offset off = reflog_reader.tell();

      auto*& refs = index->get_or_create(cur_key);
      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), bucket_size, state_type::D_ARCHIVED);
      refs->init_bucket_ptr(cur_reflog_idx, encoded_reflog_ptr(encoded_bucket));

      refs->reserve(reflog_constants::BUCKET_SIZE);
      reflog_reader.advance<uint64_t>(bucket_size * sizeof(uint64_t));

    }

    if (cur_key != archival_tail_key || cur_reflog_idx != archival_tail) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    if (reflog_reader.has_more()) {
      // TODO truncate all files after as well
      incremental_file_offset off = reflog_reader.tell();
      std::string path = off.path();
      file_utils::truncate_file(path, off.offset());
    }

    return archival_tail;
  }

  template<typename radix_tree>
  static size_t load_radix_tree_reflog_aggs(incremental_file_reader& aggs_reader, radix_tree* index) {
    auto archival_metadata = reflog_aggregates_archival_metadata(aggs_reader.read_metadata<std::string>());
    size_t key_size = archival_metadata.key_size();
    byte_string archival_tail_key = archival_metadata.archival_tail_key();

    byte_string cur_key;

    while (aggs_reader.has_more() && cur_key != archival_tail_key) {

      cur_key = byte_string(aggs_reader.read(key_size));
      size_t version = aggs_reader.read<size_t>();
      size_t num_aggs = aggs_reader.read<size_t>();

      size_t alloc_size = sizeof(aggregate) * num_aggs;
      aggregate* archived_aggs = static_cast<aggregate*>(ALLOCATOR.alloc(alloc_size, state_type::D_ARCHIVED));
      for (size_t i = 0; i < num_aggs; i++) {
        numeric agg = aggs_reader.read<numeric>();
        new (archived_aggs + i) aggregate(agg.type(), D_SUM, 1);
        archived_aggs[i].update(0, agg, version);
      }

      aggregated_reflog* refs = index->get_unsafe(cur_key);
      refs->init_aggregates(num_aggs, archived_aggs);

    }
    return index->get(archival_tail_key)->size() - reflog_constants::BUCKET_SIZE;
  }

};

}
}

#endif /* CONFLUO_ARCHIVAL_LOAD_UTILS_H_ */
