#ifndef CONFLUO_ARCHIVAL_LOAD_UTILS_H_
#define CONFLUO_ARCHIVAL_LOAD_UTILS_H_

#include "storage/allocator.h"
#include "container/data_log.h"
#include "filter.h"
#include "filter_log.h"
#include "monolog_archival_utils.h"
#include "io/incr_file_reader.h"
#include "io/incr_file_utils.h"
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
   * Load data log.
   * @param path path to archived data log
   * @param log log to load into
   */
  static void load_data_log(const std::string& path, data_log& log) {
    monolog_linear_archival_utils::load<uint8_t,
                                        data_log_constants::MAX_BUCKETS,
                                        data_log_constants::BUCKET_SIZE,
                                        data_log_constants::BUFFER_SIZE>(path, log);
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
        replay_filter(filter, log, schema, data_log_archival_tail + schema.record_size());
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
        replay_index(index, col.index_id(), log, schema, data_log_archival_tail + schema.record_size());
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
    incremental_file_reader reflog_reader(path, "filter_data");
    incremental_file_reader aggs_reader(path, "filter_aggs");
    reflog_reader.open();
    aggs_reader.open();
    filter::idx_t* tree = filter->data();
    size_t archival_tail = radix_tree_archival_utils::load<filter::idx_t>(reflog_reader, tree);
    size_t archival_tail2 = radix_tree_archival_utils::load_aggregates<filter::idx_t>(aggs_reader, tree);
    if (archival_tail != archival_tail2) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    return archival_tail;
  }

  /**
   * Load index archived on disk.
   * @param path path to data
   * @param index index to load
   * @return data log offset until which index has been archived
   */
  static size_t load_index(const std::string& path, index::radix_index* index) {
    incremental_file_reader reader(path, "index_data");
    reader.open();
    return radix_tree_archival_utils::load<index::radix_index>(reader, index);
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
    for (size_t i = start_off; i < log.size(); i += record_size) {
      // TODO this is inefficient, amortize with iterator
      log.read(i, data_buf, schema.record_size());
      record_t record = schema.apply_unsafe(i, data_buf);
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

};

}
}

#endif /* CONFLUO_ARCHIVAL_LOAD_UTILS_H_ */
