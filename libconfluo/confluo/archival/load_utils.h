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

  /**
   * Load a monolog_linear archived on disk.
   * @param path path to data
   * @param log log to load into
   */
  template<typename T, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUF_SIZE>
  static void load_monolog_linear(const std::string& path,
                                  monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE>& log) {
    incremental_file_reader reader(path, "monolog_linear", ".dat");
    reader.open();
    auto archival_metadata = reader.read_metadata<monolog_linear_archival_metadata>();

    size_t load_offset = 0;
    while (reader.has_more() && load_offset < archival_metadata.archival_tail()) {
      incremental_file_offset off = reader.tell();
      ptr_metadata metadata = reader.read<ptr_metadata>();
      size_t size = metadata.data_size_;

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
      // TODO truncate all files after as well
      incremental_file_offset off = reader.tell();
      std::string path = off.path();
      file_utils::truncate_file(path, off.offset());
    }
  }

  /**
   * Load filter log archived on disk and replay
   * remaining data from data log over the filters.
   * @param path path to data
   * @param filters filter log to load/replay over
   * @param log data log to replay records from
   * @param schema data log schema
   */
  static void load_replay_filter_log(const std::string& path, filter_log& filters,
                                     data_log& log, schema_t& schema) {
    for (size_t i = 0; i < filters.size(); i++) {
      monitor::filter* filter = filters[i];
      if (filter->is_valid()) {
        size_t data_log_archival_tail = load_filter(path + "/filter_" + std::to_string(i) + "/", filter);
        replay_filter(filter, log, schema, data_log_archival_tail);
      }
    }
  }

  /**
   * Load index log archived on disk and replay
   * remaining data from data log over the indexes.
   * @param path path to data
   * @param indexes index log to load/replay over.
   * @param log data log to replay records from
   * @param schema record schema
   */
  static void load_replay_index_log(const std::string& path, index_log& indexes,
                                    data_log& log, schema_t& schema) {
    for (size_t i = 0; i < schema.size(); i++) {
      auto& col = schema[i];
      if (col.is_indexed()) {
        auto* index = indexes[col.index_id()];
        size_t data_log_archival_tail = load_index(path + "/index_" + std::to_string(i) + "/", index);
        replay_index(index, col.index_id(), log, schema, data_log_archival_tail);
      }
    }
  }

  /**
   * Load filter archived on disk.
   * @param path path to data
   * @param filter filter to load
   * @return data log offset to which filter has been archived
   */
  static size_t load_filter(const std::string& path, monitor::filter* filter) {
    incremental_file_reader reflog_reader(path, "filter_data", ".dat");
    incremental_file_reader aggs_reader(path, "filter_aggs", ".dat");
    reflog_reader.open();
    aggs_reader.open();
    filter::idx_t* tree = filter->data();
    size_t refs_data_log_archival_tail = load_radix_tree<filter::idx_t>(reflog_reader, tree);
    size_t aggs_data_log_archival_tail = load_radix_tree_reflog_aggs<filter::idx_t>(aggs_reader, tree);
    return refs_data_log_archival_tail;
  }

  /**
   * Load index archived on disk.
   * @param path path to data
   * @param index index to load
   * @return data log offset until which index has been archived
   */
  static size_t load_index(const std::string& path, index::radix_index* index) {
    incremental_file_reader reflog_reader(path, "index_data", ".dat");
    reflog_reader.open();
    return load_radix_tree<index::radix_index>(reflog_reader, index);
  }

  /**
   * Replay data log over filter.
   * @param filter filter to replay over
   * @param log data log to replay from
   * @param schema record schema
   * @param start_off data log offset to start replaying from
   */
  static void replay_filter(monitor::filter* filter, data_log& log, schema_t& schema, size_t start_off) {
    size_t record_size = schema.record_size();
    uint8_t* data_buf = new uint8_t[record_size];
    for (size_t i = start_off + record_size; i < log.size(); i += record_size) {
      // TODO this is inefficient, amortize with iterator
      log.read(i, data_buf, schema.record_size());
      record_t record = schema.apply_unsafe(i, data_buf);
      LOG_INFO << "data log off: " << i << ", record ts: " << record.timestamp();
      filter->update(record);
    }
    delete[] data_buf;
  }

  /**
   * Replay data log over index.
   * @param index index to replay over
   * @param id index id
   * @param log data log to replay from
   * @param schema record schema
   * @param start_off data log offset to start replaying from
   */
  static void replay_index(index::radix_index* index, uint16_t id, data_log& log,
                           schema_t& schema, size_t start_off) {
    uint8_t* data_buf = new uint8_t[schema.record_size()];
    for (size_t i = start_off; i < log.size(); i += schema.record_size()) {
      // TODO this is inefficient, amortize with iterator
      log.read(i, data_buf, schema.record_size());
      record_t r = schema.apply_unsafe(i, data_buf);
      index->insert(r[id].get_key(), i);
    }
    delete[] data_buf;
  }

  /**
   * Load radix tree archived on disk.
   * @param reader reflog reader
   * @param index index to load
   * @return data log offset until which radix tree has been archived
   */
  template<typename radix_tree>
  static size_t load_radix_tree(incremental_file_reader& reader, radix_tree* index) {

    auto archival_metadata = reflog_archival_metadata(reader.read_metadata<std::string>());
    size_t key_size = archival_metadata.key_size();
    byte_string archival_tail_key = archival_metadata.archival_tail_key();
    size_t archival_tail = archival_metadata.reflog_archival_tail();
    size_t data_log_archival_tail = archival_metadata.data_log_archival_tail();

    size_t cur_reflog_beg_idx = 0;
    size_t cur_reflog_end_idx = 0;
    byte_string cur_key(std::string(key_size, '0'));

    while (reader.has_more() && (cur_key != archival_tail_key || cur_reflog_beg_idx != archival_tail)) {

      cur_key = byte_string(reader.read(key_size));
      cur_reflog_beg_idx = reader.read<size_t>();
      cur_reflog_end_idx = reader.read<size_t>();

      incremental_file_offset off = reader.tell();
      ptr_metadata metadata = reader.read<ptr_metadata>();
      size_t bucket_size = metadata.data_size_;

      auto*& refs = index->get_or_create(cur_key);
      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), bucket_size, state_type::D_ARCHIVED);
      refs->init_bucket_ptr(cur_reflog_beg_idx, encoded_reflog_ptr(encoded_bucket));

      refs->reserve(cur_reflog_end_idx - cur_reflog_beg_idx);
      reader.advance<uint8_t>(bucket_size);
      cur_reflog_beg_idx = cur_reflog_end_idx;
    }

    if (cur_key != archival_tail_key || cur_reflog_beg_idx != archival_tail) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    if (reader.has_more()) {
      // TODO truncate all files after as well
      incremental_file_offset off = reader.tell();
      std::string path = off.path();
      LOG_INFO << "has more, truncate to " << off.offset() << " from " << reader.eof_offset();
      file_utils::truncate_file(path, off.offset());
    }

    return data_log_archival_tail;
  }

  /**
   * Load aggregates of reflogs in radix tree archived on disk.
   * @param reader aggregates reader
   * @param tree radix tree
   * @return
   */
  template<typename radix_tree>
  static size_t load_radix_tree_reflog_aggs(incremental_file_reader& reader, radix_tree* tree) {
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
        size_t alloc_size = sizeof(aggregate) * num_aggs;
        aggregate* archived_aggs = static_cast<aggregate*>(
                                                  ALLOCATOR.alloc(alloc_size, state_type::D_ARCHIVED));
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

    if (reader.has_more()) {
      LOG_INFO << "why..." << reader.eof_offset() << " vs " << reader.tell().offset();
    }

    return tree->get(archival_tail_key)->size();
  }

};

}
}

#endif /* CONFLUO_ARCHIVAL_LOAD_UTILS_H_ */
