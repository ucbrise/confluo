#ifndef CONFLUO_ARCHIVAL_LOAD_UTILS_H_
#define CONFLUO_ARCHIVAL_LOAD_UTILS_H_

#include "archival_utils.h"
#include "storage/allocator.h"
#include "container/data_log.h"
#include "filter.h"
#include "filter_archiver.h"
#include "filter_log.h"
#include "io/incremental_file_reader.h"
#include "io/incremental_file_writer.h"
#include "index_archiver.h"
#include "index_log.h"
#include "monolog_linear_archiver.h"
#include "container/reflog.h"
#include "storage/ptr_aux_block.h"

namespace confluo {
namespace archival {

using namespace ::utils;
using namespace storage;

class load_utils {

 public:
  /**
   * Load data log. Archived buckets are loaded first,
   * followed by durably written buckets if available.
   * @param path path to archived data log
   * @param log log to load into
   */
  static void load_data_log(const std::string& path, const storage_mode mode, data_log& log);

  /**
   * Load data log buckets from storage, starting at a particular bucket.
   * @param log data log to load into
   * @param start_bucket_idx start bucket
   */
  static void load_data_log_storage(data_log& log, size_t start_bucket_idx);

  /**
   * Load filter log archived on disk and replay
   * remaining data from data log over the filters.
   * @param path path to filter log data
   * @param filters filter log to load/replay over
   * @param log data log to replay records from
   * @param schema data log schema
   */
  static void load_replay_filter_log(const std::string& path, filter_log& filters, data_log& log, schema_t& schema);

  /**
   * Load index log archived on disk and replay
   * remaining data from data log over the indexes.
   * @param path path to index log data
   * @param indexes index log to load/replay over.
   * @param log data log to replay records from
   * @param schema record schema
   */
  static void load_replay_index_log(const std::string& path, index_log& indexes, data_log& log, schema_t& schema);

  /**
   * Load filter archived on disk.
   * @param path path to data
   * @param filter filter to load
   * @return data log offset to which filter has been archived
   */
  static size_t load_filter(const std::string& path, monitor::filter* filter);

  /**
   * Load index archived on disk.
   * @param path path to data
   * @param index index to load
   * @return data log offset until which index has been archived
   */
  static size_t load_index(const std::string& path, index::radix_index* index);

  /**
   * Replay data log over filter.
   * @param filter filter to replay over
   * @param log data log to replay from
   * @param schema record schema
   * @param start_off data log offset to start replaying from
   */
  static void replay_filter(monitor::filter* filter, data_log& log, schema_t& schema, size_t start_off);

  /**
   * Replay data log over index.
   * @param index index to replay over
   * @param id index id
   * @param log data log to replay from
   * @param schema record schema
   * @param start_off data log offset to start replaying from
   */
  static void replay_index(index::radix_index* index, uint16_t id, data_log& log, schema_t& schema, size_t start_off);

};

}
}

#endif /* CONFLUO_ARCHIVAL_LOAD_UTILS_H_ */
