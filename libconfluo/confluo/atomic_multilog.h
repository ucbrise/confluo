#ifndef CONFLUO_ATOMIC_MULTILOG_H_
#define CONFLUO_ATOMIC_MULTILOG_H_

#include <cmath>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <functional>
#include <numeric>
#include <thread>

#include "archival/archival_mode.h"
#include "archival/load_utils.h"
#include "optional.h"
#include "exceptions.h"
#include "parser/expression_compiler.h"
#include "trigger.h"
#include "types/type_manager.h"
#include "alert_index.h"
#include "archival/atomic_multilog_archiver.h"
#include "atomic_multilog_metadata.h"
#include "conf/configuration_params.h"
#include "container/data_log.h"
#include "container/cursor/record_cursors.h"
#include "container/cursor/alert_cursor.h"
#include "container/monolog/monolog.h"
#include "container/radix_tree.h"
#include "container/string_map.h"
#include "filter.h"
#include "filter_log.h"
#include "index_log.h"
#include "parser/schema_parser.h"
#include "parser/expression_parser.h"
#include "parser/expression_compiler.h"
#include "parser/aggregate_parser.h"
#include "parser/trigger_parser.h"
#include "planner/query_planner.h"
#include "read_tail.h"
#include "schema/column.h"
#include "schema/record_batch.h"
#include "schema/schema.h"
#include "storage/storage.h"
#include "time_utils.h"
#include "string_utils.h"
#include "threads/periodic_task.h"
#include "threads/task_pool.h"

using namespace ::confluo::archival;
using namespace ::confluo::monolog;
using namespace ::confluo::index;
using namespace ::confluo::monitor;
using namespace ::confluo::parser;
using namespace ::confluo::planner;
using namespace ::utils;

namespace confluo {

/**
* The main data structure for the data store
*/
class atomic_multilog {
 public:
  /**
   * Type of data log
   */
  typedef data_log data_log_type;

  /**
   * Type of schema
   */
  typedef schema_t schema_type;

  /**
   * Type of read tail
   */
  typedef read_tail read_tail_type;

  /**
   * Type of metadata_writer
   */
  typedef metadata_writer metadata_writer_type;

  /**
   * Identifier for filter
   */
  typedef size_t filter_id_t;

  /**
   * Identifier for aggregate
   */
  struct aggregate_id_t {
    /** The filter index */
    filter_id_t filter_idx;
    /** The aggregate index */
    size_t aggregate_idx;
  };

  /**
   * Identifier for trigger
   */
  struct trigger_id_t {
    /** Identifier for the aggregate */
    aggregate_id_t aggregate_id;
    /** Index of the trigger */
    size_t trigger_idx;
  };

  /**
   * List of alerts
   */
  typedef alert_index::alert_list alert_list;

  /**
   * Constructor that initializes atomic multilog
   * @param name The atomic multilog name
   * @param schema The schema for the atomic multilog
   * @param path The path of the atomic multilog
   * @param mode The storage mode of the atomic multilog
   * @param pool The pool of tasks
   */
  atomic_multilog(const std::string &name, const std::vector<column_t> &schema, const std::string &path,
                  const storage::storage_mode &s_mode, const archival_mode &a_mode, task_pool &pool);

  /**
   * Initializes an atomic multilog from the given parameters
   *
   * @param name The name of the atomic multilog
   * @param schema The schema of the atomic multilog
   * @param path The path to store multilog data
   * @param mode The storage mode of the multilog
   * @param pool The pool of tasks for the multilog
   */
  atomic_multilog(const std::string &name, const std::string &schema, const std::string &path,
                  const storage::storage_mode &storage_mode, const archival_mode &a_mode, task_pool &pool);

  /**
   * Constructor that initializes atomic multilog from existing archives.
   * @param name The atomic multilog name
   * @param path The path of the atomic multilog
   * @param pool The pool of tasks
   */
  atomic_multilog(const std::string &name, const std::string &path, task_pool &pool);

  /**
   * Force archival of multilog up to the read tail.
   */
  void archive();

  /**
   * Force archival of multilog up to an offset.
   * @param offset The offset into the data log at which the data is stored
   */
  void archive(size_t offset);

  // Management ops
  /**
   * Adds index to the atomic multilog
   * @param field_name The name of the field in the atomic multilog
   * @param bucket_size The size of the bucket
   * @throw ex Management exception
   */
  void add_index(const std::string &field_name, double bucket_size = configuration_params::INDEX_BUCKET_SIZE());

  /**
   * Removes index from the atomic multilog
   * @param field_name The name of the field in the atomic multilog
   * @throw ex Management exception
   */
  void remove_index(const std::string &field_name);

  /**
   * Checks whether column of atomic multilog is indexed
   * @param field_name The name of the field
   * @return True if column is indexed, false otherwise
   * @throw ex Management exception
   */
  bool is_indexed(const std::string &field_name);

  /**
   * Adds filter to the atomic multilog
   * @param name The name of the filter
   * @param expr The expression to filter out elements in the atomic multilog
   * @throw ex Management exception
   */
  void add_filter(const std::string &name, const std::string &expr);

  /**
   * Removes filter from the atomic multilog
   * @param name The name of the filter
   * @throw ex Management exception
   */
  void remove_filter(const std::string &name);

  /**
   * Adds aggregate to the atomic multilog
   *
   * @param name Name of the aggregate.
   * @param filter_name Name of filter to add aggregate to.
   * @param expr Aggregate expression (e.g., min(temp))
   */
  void add_aggregate(const std::string &name, const std::string &filter_name, const std::string &expr);

  /**
   * Removes aggregate from the atomic multilog
   *
   * @param name The name of the aggregate
   * @throw Management exception
   */
  void remove_aggregate(const std::string &name);

  /**
   * Adds trigger to the atomic multilog
   * @param name The name of the trigger
   * @param expr The trigger expression to be executed
   * @param periodicity_ms The periodicity in milliseconds
   * @throw ex Management exception
   */
  void install_trigger(const std::string &name,
                       const std::string &expr,
                       const uint64_t periodicity_ms = configuration_params::MONITOR_PERIODICITY_MS());

  /**
   * Removes trigger from the atomic multilog
   * @param name The name of the trigger
   * @throw Management exception
   */
  void remove_trigger(const std::string &name);

  // Query ops
  /**
   * Get a record batch builder.
   * @return The record batch builder
   */
  record_batch_builder get_batch_builder() const;

  /**
   * Appends a batch of records to the atomic multilog
   * @param batch The record batch to be added
   * @return The offset where the batch is located
   */
  size_t append_batch(record_batch &batch);

  /**
   * Appends data to the atomic multilog
   * @param data The data to be stored
   * @return The offset of where the data is located
   */
  size_t append(void *data);

  /**
   * Appends json-formatted data to the atomic multilog
   * @param json_data The json-formatted data to be stored
   * @return The offset of where the data is located
   */
  size_t append_json(std::string json_data);

  // TODO: Add a std::tuple based variant
  // TODO: Add a JSON based variant
  /**
   * Appends a record to the atomic multilog
   * @param record The record to be stored
   * @return The offset in data log where the record is written
   */
  size_t append(const std::vector<std::string> &record);

  /**
   * Reads data from the atomic multilog at the specified offset into a pointer.
   * This read variant is the most efficient, since it avoids copies when possible.
   * @param offset The offset into the data log at which the data is stored
   * @param version The current version
   * @param ptr The pointer to populate
   */
  void read(uint64_t offset, uint64_t &version, read_only_data_log_ptr &ptr) const;

  /**
   * Reads data from the atomic multilog at the specified offset into a pointer.
   * This read variant is the most efficient, since it avoids copies when possible.
   * @param offset The offset into the data log at which the data is stored
   * @param ptr The pointer to populate
   */
  void read(uint64_t offset, read_only_data_log_ptr &ptr) const;

  /**
   * Reads a record given an offset into the data log
   * @param offset The offset into the data log of the record
   * @param version The current version
   * @return The corresponding record as a vector of strings.
   */
  std::vector<std::string> read(uint64_t offset, uint64_t &version) const;

  /**
   * Reads a record given an offset into the data log
   * @param offset The offset into the data log of the record
   * @return The corresponding record
   */
  std::vector<std::string> read(uint64_t offset) const;

  /**
   * Reads a record given an offset into the data log
   * @param offset The offset into the data log of the record
   * @param version The current version
   * @return The corresponding record as a vector of strings.
   */
  std::string read_json(uint64_t offset, uint64_t &version) const;

  /**
   * Reads a record given an offset into the data log
   * @param offset The offset into the data log of the record
   * @return The corresponding record
   */
  std::string read_json(uint64_t offset) const;

  /**
   * Reads a record given an offset into the data log
   * @param offset The offset into the data log of the record
   * @param version The current version
   * @return Pointer to the corresponding raw record bytes
   */
  std::unique_ptr<uint8_t> read_raw(uint64_t offset, uint64_t &version) const;

  /**
   * Reads a record given an offset into the data log
   * @param offset The offset into the data log of the record
   * @param version The current version
   * @return Pointer to the corresponding raw record bytes
   */
  std::unique_ptr<uint8_t> read_raw(uint64_t offset) const;

  /**
   * Executes the filter expression
   * @param expr The filter expression
   * @return The result of applying the filter to the atomic multilog
   */
  std::unique_ptr<record_cursor> execute_filter(const std::string &expr) const;

  // TODO: Add tests
  /**
   * Executes an aggregate
   *
   * @param aggregate_expr The aggregate expression
   * @param filter_expr The filter expression
   *
   * @return A numeric containing the result of the aggregate
   */
  numeric execute_aggregate(const std::string &aggregate_expr, const std::string &filter_expr);

  /**
   * Queries an existing filter
   * @param filter_name Name of the filter
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @return A stream containing the results of the filter
   */
  std::unique_ptr<record_cursor> query_filter(const std::string &filter_name,
                                              uint64_t begin_ms,
                                              uint64_t end_ms) const;

  /**
   * Queries an existing filter
   * @param filter_name The name of the filter
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @param additional_filter_expr Additional filter expression
   * @return A stream containing the results of the filter
   */
  std::unique_ptr<record_cursor> query_filter(const std::string &filter_name, uint64_t begin_ms, uint64_t end_ms,
                                              const std::string &additional_filter_expr) const;

  /**
   * Query a stored aggregate.
   * @param aggregate_name The name of the aggregate
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @return The aggregate value for the given time range.
   */
  numeric get_aggregate(const std::string &aggregate_name, uint64_t begin_ms, uint64_t end_ms);

  /**
   * Obtain a cursor over alerts in a time-range
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @return Cursor over alerts in the time range
   */
  std::unique_ptr<alert_cursor> get_alerts(uint64_t begin_ms, uint64_t end_ms) const;

  /**
   * Obtain a cursor over alerts on a given trigger in a time-range
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @param trigger_name Name of the trigger.
   * @return Cursor over alerts in the time range
   */
  std::unique_ptr<alert_cursor> get_alerts(uint64_t begin_ms, uint64_t end_ms, const std::string &trigger_name) const;

  /**
   * Gets the name of the atomic multilog
   * @return The atomic multilog name
   */
  const std::string &get_name() const;

  /**
   * Gets the atomic multilog schema
   * @return The schema of the atomic multilog
   */
  const schema_t &get_schema() const;

  /**
   * Gets the number of records in the atomic multilog
   * @return The number of records
   */
  size_t num_records() const;

  /**
   * Gets the record size
   * @return The record size of the schema
   */
  size_t record_size() const;

 protected:
  /**
   * Load multilog from archives. Expects metadata to be loaded
   * and archiver to be initialized.
   * Note: no reads/writes should occur during this period, since the data log is being initialized.
   * @param mode The storage mode used in the previous life-cycle of this multilog
   */
  void load(const storage::storage_mode &mode);

  /**
   * Lead multilog metadata from path
   * @param path Path to load metadata from
   * @param s_mode Storage mode
   * @param a_mode Archival mode
   */
  void load_metadata(const std::string &path, storage_mode &s_mode, archival_mode &a_mode);

  /**
   * Updates the record block
   * @param log_offset The offset of the log
   * @param block The record block
   * @param record_size The size of each record
   */
  void update_aux_record_block(uint64_t log_offset, record_block &block, size_t record_size);

  /**
   * Adds an index to the schema for a given field
   *
   * @param field_name The name of the field to index
   * @param bucket_size The bucket_size used for indexing
   * @param ex The exception when the index could not be added
   */
  void add_index_task(const std::string &field_name, double bucket_size, optional<management_exception> &ex);

  /**
   * Removes an index for a given field in the schema
   *
   * @param field_name The name of the field to index
   * @param ex The exception when the index could not be removed
   */
  void remove_index_task(const std::string &field_name, optional<management_exception> &ex);

  /**
   * Adds a filter to be executed on the data
   *
   * @param name The name of the filter
   * @param expr The filter expression to execute
   * @param ex The exception when the filter could not be added
   */
  void add_filter_task(const std::string &name, const std::string &expr, optional<management_exception> &ex);

  /**
   * Removes a filter that was created
   *
   * @param name The name of the filter
   * @param ex The exception when the filter could not be removed
   */
  void remove_filter_task(const std::string &name, optional<management_exception> &ex);

  /**
   * Adds an aggregate 
   *
   * @param name The name of the aggregate
   * @param filter_name The name of the filter
   * @param expr The filter expression
   * @param ex The exception if the aggregate could not be added
   */
  void add_aggregate_task(const std::string &name,
                          const std::string &filter_name,
                          const std::string &expr,
                          optional<management_exception> &ex);

  /**
   * Removes an aggregate
   *
   * @param name The name of the aggregate to remove
   * @param ex The exception if the aggregate cannot be removed
   */
  void remove_aggregate_task(const std::string &name,
                             optional<management_exception> &ex);

  /**
   * Adds a trigger to the atomic multilog
   *
   * @param name The name of the trigger
   * @param expr The trigger expression
   * @param periodicity_ms The periodicity of the trigger measured in
   * milliseconds
   * @param ex The exception when the trigger cannot be added
   */
  void add_trigger_task(const std::string &name,
                        const std::string &expr,
                        uint64_t periodicity_ms,
                        optional<management_exception> &ex);

  /**
   * Removes a trigger from the atomic multilog
   *
   * @param name The name of the trigger
   * @param ex The exception when the trigger could not be removed
   */
  void remove_trigger_task(const std::string &name, optional<management_exception> &ex);

  /**
   * Archives until only a configured number of bytes
   * of the data log are resident in memory.
   */
  void archival_task();

  /**
   * Monitors the task executed on the atomic multilog
   */
  void monitor_task();

  /**
   * Checks the time bucket and adds alerts when necessary
   *
   * @param f The filter 
   * @param t The trigger
   * @param tid The thread identifier
   * @param time_bucket The time_bucket to check
   * @param version The version to check
   */
  void check_time_bucket(filter *f, trigger *t, size_t tid, uint64_t time_bucket, uint64_t version);

  /** The name of the multilog */
  std::string name_;
  /** The schema of the multilog */
  schema_type schema_;

  /** The data log */
  data_log_type data_log_;
  /** The read tail */
  read_tail_type rt_;
  /** The metadata associated with the multilog */
  metadata_writer_type metadata_;

  // In memory structures
  /** The list of filters */
  filter_log filters_;
  /** The list of indexes */
  index_log indexes_;
  /** The list of alerts */
  alert_index alerts_;

  /** A map from id to filter */
  string_map<filter_id_t> filter_map_;
  /** A map from id to aggregate */
  string_map<aggregate_id_t> aggregate_map_;
  /** A map from id to trigger */
  string_map<trigger_id_t> trigger_map_;

  /** The query planner for the multilog */
  query_planner planner_;

  // Archival
  atomic_multilog_archiver archiver_;
  periodic_task archival_task_;
  task_pool archival_pool_;

  // Manangement
  /** The pool of tasks */
  task_pool &mgmt_pool_;
  /** The monitor task */
  periodic_task monitor_task_;
};

}

#endif /* CONFLUO_ATOMIC_MULTILOG_H_ */
