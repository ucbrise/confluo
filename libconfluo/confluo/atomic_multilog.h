#ifndef CONFLUO_ATOMIC_MULTILOG_H_
#define CONFLUO_ATOMIC_MULTILOG_H_

#include <math.h>

#include <functional>
#include <numeric>
#include <thread>

#include "archival/archiver.h"
#include "archival/load_utils.h"
#include "optional.h"
#include "exceptions.h"
#include "parser/expression_compiler.h"
#include "trigger.h"
#include "types/type_manager.h"
#include "alert_index.h"
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
   * @param storage The storage mode of the atomic multilog
   * @param pool The pool of tasks
   */
  atomic_multilog(const std::string& name, const std::vector<column_t>& schema,
                  const std::string& path, const storage::storage_mode& mode,
                  task_pool& pool)
      : name_(name),
        schema_(schema),
        data_log_("data_log", path, mode),
        rt_(path, mode),
        metadata_(path, mode),
        planner_(&data_log_, &indexes_, &schema_),
        archiver_(new archiver(path, rt_, data_log_, filters_, indexes_, schema_)),
        archival_task_("archival"),
        mgmt_pool_(pool),
        monitor_task_("monitor") {
    metadata_.write_schema(schema_);
    monitor_task_.start(std::bind(&atomic_multilog::monitor_task, this),
                        configuration_params::MONITOR_PERIODICITY_MS);
//    archival_task_.start(std::bind(&archival::archival_manager::archive), <--- TODO fix this
//                         configuration_params::ARCHIVAL_PERIODICITY_MS);
  }

  atomic_multilog(const std::string& name, const std::string& schema,
                  const std::string& path, const storage::storage_mode& mode,
                  task_pool& pool)
      : atomic_multilog(name, parser::parse_schema(schema), path, mode, pool) {
  }

//  atomic_multilog(const std::string& name, const std::string& path)
//      : name_(name) {
//    load(path);
//    // TODO other initialization
//  }

  void load(const std::string& path) {
    metadata_reader reader(path);

    // redirect metadata writer temporarily to prevent duplicate writes
    metadata_writer writer = metadata_;
    metadata_ = metadata_writer("", storage::storage_id::D_IN_MEMORY);

    while (reader.has_next()) {
      switch (reader.next_type()) {
        case D_SCHEMA_METADATA: {
          schema_ = reader.next_schema();
          break;
        }
        case D_FILTER_METADATA: {
          auto filter_metadata = reader.next_filter_metadata();
          add_filter(filter_metadata.filter_name(), filter_metadata.expr());
          break;
        }
        case D_INDEX_METADATA: {
          auto index_metadata = reader.next_index_metadata();
          add_index(index_metadata.field_name(), index_metadata.bucket_size());
          break;
        }
        case D_AGGREGATE_METADATA: {
          auto agg_metadata = reader.next_aggregate_metadata();
          add_aggregate(agg_metadata.aggregate_name(), agg_metadata.filter_name(),
                        agg_metadata.aggregate_expression());
          break;
        }
        case D_TRIGGER_METADATA: {
          auto trigger_metadata = reader.next_trigger_metadata();
          add_trigger(trigger_metadata.trigger_name(), trigger_metadata.trigger_expression(),
                      trigger_metadata.periodicity_ms());
          break;
        }
      }
    }
    metadata_ = writer;
    // TODO: more prep before calling load, for ex: read the read_tail in etc

    archiver_ = new archiver(path, rt_, data_log_, filters_, indexes_, schema_);
    load_utils::load_monolog_linear<uint8_t,
                                    data_log_constants::MAX_BUCKETS,
                                    data_log_constants::BUCKET_SIZE,
                                    data_log_constants::BUFFER_SIZE>(archiver_->data_log_path(), data_log_);
    load_utils::load_replay_filter_log(archiver_->filter_log_path(), filters_, data_log_, schema_);
    load_utils::load_replay_index_log(archiver_->index_log_path(), indexes_, data_log_, schema_);
  }

  // Management ops
  /**
   * Adds index to the atomic multilog
   * @param field_name The name of the field in the atomic multilog
   * @throw ex Management exception
   */
  void add_index(const std::string& field_name, double bucket_size =
                     configuration_params::INDEX_BUCKET_SIZE) {
    optional<management_exception> ex;
    std::future<void> ret = mgmt_pool_.submit(
        [field_name, bucket_size, &ex, this] {
          add_index_task(field_name, bucket_size, ex);
        });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  /**
   * Removes index from the atomic multilog
   * @param field_name The name of the field in the atomic multilog
   * @throw ex Management exception
   */
  void remove_index(const std::string& field_name) {
    optional<management_exception> ex;
    std::future<void> ret = mgmt_pool_.submit([field_name, &ex, this] {
      remove_index_task(field_name, ex);
    });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  /**
   * Checks whether column of atomic multilog is indexed
   * @param field_name The name of the field
   * @return True if column is indexed, false otherwise
   * @throw ex Management exception
   */
  bool is_indexed(const std::string& field_name) {
    optional<management_exception> ex;
    uint16_t idx;
    try {
      idx = schema_.get_field_index(field_name);
    } catch (std::exception& e) {
      THROW(management_exception, "Field name does not exist");
    }
    column_t& col = schema_[idx];
    return col.is_indexed();
  }

  /**
   * Adds filter to the atomic multilog
   * @param name The name of the filter
   * @param expr The expression to filter out elements in the atomic multilog
   * @throw ex Management exception
   */
  void add_filter(const std::string& name, const std::string& expr) {
    optional<management_exception> ex;
    std::future<void> ret = mgmt_pool_.submit([name, expr, &ex, this] {
      add_filter_task(name, expr, ex);
    });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  /**
   * Removes filter from the atomic multilog
   * @param name The name of the filter
   * @throw ex Management exception
   */
  void remove_filter(const std::string& name) {
    optional<management_exception> ex;
    std::future<void> ret = mgmt_pool_.submit([name, &ex, this] {
      remove_filter_task(name, ex);
    });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  /**
   * Adds aggregate to the atomic multilog
   *
   * @param name Name of the aggregate.
   * @param filter_name Name of filter to add aggregate to.
   * @param expr Aggregate expression (e.g., min(temp))
   */
  void add_aggregate(const std::string& name, const std::string& filter_name,
                     const std::string& expr) {
    optional<management_exception> ex;
    std::future<void> ret = mgmt_pool_.submit(
        [name, filter_name, expr, &ex, this] {
          add_aggregate_task(name, filter_name, expr, ex);
        });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  /**
   * Removes aggregate from the atomic multilog
   *
   * @param name The name of the aggregate
   * @throw Management exception
   */
  void remove_aggregate(const std::string& name) {
    optional<management_exception> ex;
    std::future<void> ret = mgmt_pool_.submit([name, &ex, this] {
      remove_aggregate_task(name, ex);
    });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  /**
   * Adds trigger to the atomic multilog
   * @param name The name of the trigger
   * @param expr The trigger expression to be executed
   * @throw ex Management exception
   */
  void install_trigger(const std::string& name, const std::string& expr,
                       const uint64_t periodicity_ms =
                           configuration_params::MONITOR_PERIODICITY_MS) {

    if (periodicity_ms < configuration_params::MONITOR_PERIODICITY_MS) {
      throw management_exception(
          "Trigger periodicity (" + std::to_string(periodicity_ms)
              + "ms) cannot be less than monitor periodicity ("
              + std::to_string(configuration_params::MONITOR_PERIODICITY_MS)
              + "ms)");
    }

    if (periodicity_ms % configuration_params::MONITOR_PERIODICITY_MS != 0) {
      throw management_exception(
          "Trigger periodicity (" + std::to_string(periodicity_ms)
              + "ms) must be a multiple of monitor periodicity ("
              + std::to_string(configuration_params::MONITOR_PERIODICITY_MS)
              + "ms)");
    }

    optional<management_exception> ex;
    std::future<void> ret = mgmt_pool_.submit(
        [name, expr, periodicity_ms, &ex, this] {
          add_trigger_task(name, expr, periodicity_ms, ex);
        });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  /**
   * Removes trigger from the atomic multilog
   * @param trigger_name The name of the trigger
   * @throw Management exception
   */
  void remove_trigger(const std::string& name) {
    optional<management_exception> ex;
    std::future<void> ret = mgmt_pool_.submit([name, &ex, this] {
      remove_trigger_task(name, ex);
    });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  // Query ops
  /**
   * Get a record batch builder.
   * @return The record batch builder
   */
  record_batch_builder get_batch_builder() const {
    return record_batch_builder(schema_);
  }

  /**
   * Appends a batch of records to the atomic multilog
   * @param batch The record batch to be added
   * @return The offset where the batch is located
   */
  size_t append_batch(record_batch& batch) {
    size_t record_size = schema_.record_size();
    size_t batch_bytes = batch.nrecords * record_size;
    size_t log_offset = data_log_.reserve(batch_bytes);
    size_t cur_offset = log_offset;
    for (record_block& block : batch.blocks) {
      data_log_.write(cur_offset,
                      reinterpret_cast<const uint8_t*>(block.data.data()),
                      block.data.length());
      update_aux_record_block(cur_offset, block, record_size);
      cur_offset += block.data.length();
    }

    data_log_.flush(log_offset, batch_bytes);
    rt_.advance(log_offset, batch_bytes);
    return log_offset;
  }

  /**
   * Appends data to the atomic multilog
   * @param data The data to be stored
   * @return The offset of where the data is located
   */
  size_t append(void* data) {
    size_t record_size = schema_.record_size();
    size_t offset = data_log_.append((const uint8_t*) data, record_size);
    record_t r = schema_.apply_unsafe(offset, data);

    size_t nfilters = filters_.size();
    for (size_t i = 0; i < nfilters; i++)
      if (filters_.at(i)->is_valid())
        filters_.at(i)->update(r);

    for (const field_t& f : r)
      if (f.is_indexed())
        indexes_.at(f.index_id())->insert(f.get_key(), offset);

    data_log_.flush(offset, record_size);
    rt_.advance(offset, record_size);
    return offset;
  }

  // TODO: Add a std::tuple based variant
  // TODO: Add a JSON based variant
  /**
   * Appends a record to the atomic multilog
   * @param record The record to be stored
   * @return The offset in data log where the record is written
   */
  size_t append(const std::vector<std::string>& record) {
    void* buf = schema_.record_vector_to_data(record);
    size_t off = append(buf);
    delete[] reinterpret_cast<uint8_t*>(buf);
    return off;
  }

  /**
   * Reads the data from the atomic multilog
   * @param offset The location of where the data is stored
   * @param version The tail pointer's location
   * @param ptr The read-only pointer to store in
   */
  void read(uint64_t offset, uint64_t& version, storage::read_only_encoded_ptr<uint8_t>& ptr) const {
    version = rt_.get();
    if (offset < version) {
      data_log_.cptr(offset, ptr);
      return;
    }
    ptr.init(nullptr);
  }

  /**
   * Reads the data based on the offset
   * @param offset The location of the data
   * @param ptr The read-only pointer to store in
   */
  void read(uint64_t offset, storage::read_only_encoded_ptr<uint8_t>& ptr) const {
    uint64_t version;
    read(offset, version, ptr);
  }

  /**
   * Read the record given the offset into the data log
   * @param offset The offset of the data into the data log
   * @param version The current version
   * @return Return the corresponding record.
   */
  std::vector<std::string> read(uint64_t offset, uint64_t& version) const {
    read_only_data_log_ptr rptr;
    read(offset, version, rptr);
    decoded_data_log_ptr dec_ptr = rptr.decode_ptr();
    return schema_.data_to_record_vector(dec_ptr.get());
  }

  /**
   * Read the record given the offset into the data log
   * @param offset The offset of the data into the data log
   * @param version The current version
   * @return Return the corresponding record.
   */
  std::vector<std::string> read(uint64_t offset) const {
    uint64_t version;
    return read(offset, version);
  }

  /**
   * Executes the filter expression
   * @param expr The filter expression
   * @return The result of applying the filter to the atomic multilog
   */
  std::unique_ptr<record_cursor> execute_filter(const std::string& expr) const {
    uint64_t version = rt_.get();
    auto t = parser::parse_expression(expr);
    auto cexpr = parser::compile_expression(t, schema_);
    query_plan plan = planner_.plan(cexpr);
    return plan.execute(version);
  }

  // TODO: Add tests
  numeric execute_aggregate(const std::string& aggregate_expr,
                            const std::string& filter_expr) {
    auto pa = parser::parse_aggregate(aggregate_expr);
    aggregator agg = aggregate_manager::get_aggregator(pa.agg);
    uint16_t field_idx = schema_[pa.field_name].idx();
    uint64_t version = rt_.get();
    auto t = parser::parse_expression(filter_expr);
    auto cexpr = parser::compile_expression(t, schema_);
    query_plan plan = planner_.plan(cexpr);
    return plan.aggregate(version, field_idx, agg);
  }

  /**
   * Queries an existing filter
   * @param filter_name Name of the filter
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @return A stream containing the results of the filter
   */
  std::unique_ptr<record_cursor> query_filter(const std::string& filter_name,
                                              uint64_t begin_ms,
                                              uint64_t end_ms) const {
    filter_id_t filter_id;
    if (filter_map_.get(filter_name, filter_id) == -1) {
      throw invalid_operation_exception(
          "Filter " + filter_name + " does not exist.");
    }

    filter::range_result res = filters_.at(filter_id)->lookup_range(begin_ms,
                                                                    end_ms);
    uint64_t version = rt_.get();
    std::unique_ptr<offset_cursor> o_cursor(
        new offset_iterator_cursor<filter::range_result::iterator>(res.begin(),
                                                                   res.end(),
                                                                   version));
    return std::unique_ptr<record_cursor>(
        new filter_record_cursor(std::move(o_cursor), &data_log_, &schema_,
                                 parser::compiled_expression()));
  }

  /**
   * Queries an existing filter
   * @param filter_name The name of the filter
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @param additional_filter_expr Additional filter expression
   * @return A stream containing the results of the filter
   */
  std::unique_ptr<record_cursor> query_filter(
      const std::string& filter_name, uint64_t begin_ms, uint64_t end_ms,
      const std::string& additional_filter_expr) const {
    auto t = parser::parse_expression(additional_filter_expr);
    auto e = parser::compile_expression(t, schema_);
    filter_id_t filter_id;
    if (filter_map_.get(filter_name, filter_id) == -1) {
      throw invalid_operation_exception(
          "Filter " + filter_name + " does not exist.");
    }

    filter::range_result res = filters_.at(filter_id)->lookup_range(begin_ms,
                                                                    end_ms);
    uint64_t version = rt_.get();
    std::unique_ptr<offset_cursor> o_cursor(
        new offset_iterator_cursor<filter::range_result::iterator>(res.begin(),
                                                                   res.end(),
                                                                   version));
    return std::unique_ptr<record_cursor>(
        new filter_record_cursor(std::move(o_cursor), &data_log_, &schema_, e));
  }

  /**
   * Query a stored aggregate.
   * @param aggregate_name The name of the aggregate
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @return The aggregate value for the given time range.
   */
  numeric get_aggregate(const std::string& aggregate_name, uint64_t begin_ms,
                        uint64_t end_ms) {
    aggregate_id_t aggregate_id;
    if (aggregate_map_.get(aggregate_name, aggregate_id) == -1) {
      throw invalid_operation_exception(
          "Aggregate " + aggregate_name + " does not exist.");
    }
    uint64_t version = rt_.get();
    size_t fid = aggregate_id.filter_idx;
    size_t aid = aggregate_id.aggregate_idx;
    aggregate_info* a = filters_.at(fid)->get_aggregate_info(aid);
    numeric agg = a->zero();
    for (uint64_t t = begin_ms; t <= end_ms; t++) {
      aggregated_reflog const* refs;
      if ((refs = filters_.at(fid)->lookup(t)) == nullptr)
        continue;
      numeric t_agg = refs->get_aggregate(aid, version);
      agg = a->comb_op(agg, t_agg);
    }
    return agg;
  }

  /**
   * Obtain a cursor over alerts in a time-range
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @return Cursor over alerts in the time range
   */
  std::unique_ptr<alert_cursor> get_alerts(uint64_t begin_ms,
                                           uint64_t end_ms) const {
    return get_alerts(begin_ms, end_ms, "");

  }

  /**
   * Obtain a cursor over alerts on a given trigger in a time-range
   * @param begin_ms Beginning of time-range in ms
   * @param end_ms End of time-range in ms
   * @param trigger_name Name of the trigger.
   * @return Cursor over alerts in the time range
   */
  std::unique_ptr<alert_cursor> get_alerts(
      uint64_t begin_ms, uint64_t end_ms,
      const std::string& trigger_name) const {
    return std::unique_ptr<alert_cursor>(
        new trigger_alert_cursor(alerts_.get_alerts(begin_ms, end_ms),
                                 trigger_name));
  }

  /**
   * Gets the name of the atomic multilog
   * @return The atomic multilog name
   */
  const std::string& get_name() const {
    return name_;
  }

  /**
   * Gets the atomic multilog schema
   * @return The schema of the atomic multilog
   */
  const schema_t& get_schema() const {
    return schema_;
  }

  /**
   * Gets the number of records in the atomic multilog
   * @return The number of records
   */
  size_t num_records() const {
    return rt_.get() / schema_.record_size();
  }

  /**
   * Gets the record size
   * @return The record size of the schema
   */
  size_t record_size() const {
    return schema_.record_size();
  }

 protected:
  /**
   * Updates the record block
   * @param log_offset The offset of the log
   * @param block The record block
   * @param record_size The size of each record
   */
  void update_aux_record_block(uint64_t log_offset, record_block& block,
                               size_t record_size) {
    schema_snapshot snap = schema_.snapshot();
    for (size_t i = 0; i < filters_.size(); i++) {
      if (filters_.at(i)->is_valid()) {
        filters_.at(i)->update(log_offset, snap, block, record_size);
      }
    }

    for (size_t i = 0; i < schema_.size(); i++) {
      if (snap.is_indexed(i)) {
        radix_index* idx = indexes_.at(snap.index_id(i));
        // Handle timestamp differently
        // TODO: What if indexing requested for finer granularity?
        if (i == 0) {  // Timestamp
          auto& refs = idx->get_or_create(snap.time_key(block.time_block));
          size_t idx = refs->reserve(block.nrecords);
          for (size_t j = 0; j < block.nrecords; j++) {
            refs->set(idx + j, log_offset + j * record_size);
          }
        } else {
          for (size_t j = 0; j < block.nrecords; j++) {
            size_t block_offset = j * record_size;
            size_t record_offset = log_offset + block_offset;
            void* rec_ptr = reinterpret_cast<uint8_t*>(&block.data[0])
                + block_offset;
            idx->insert(snap.get_key(rec_ptr, i), record_offset);
          }
        }
      }
    }
  }

  /**
   * Adds an index to the schema for a given field
   *
   * @param field_name The name of the field to index
   * @param bucket_size The bucket_size used for indexing
   * @param ex The exception when the index could not be added
   */
  void add_index_task(const std::string& field_name, double bucket_size,
                      optional<management_exception>& ex) {
    uint16_t idx;
    try {
      idx = schema_.get_field_index(field_name);
    } catch (std::exception& e) {
      ex = management_exception(
          "Could not add index for " + field_name + " : " + e.what());
      return;
    }

    column_t& col = schema_[idx];
    bool success = col.set_indexing();
    if (success) {
      uint16_t index_id = UINT16_MAX;
      if (col.type().is_valid()) {
        index_id = indexes_.push_back(new radix_index(col.type().size, 256));
      } else {
        ex = management_exception("Index not supported for field type");
      }
      col.set_indexed(index_id, bucket_size);
      metadata_.write_index_metadata(field_name, bucket_size);
    } else {
      ex = management_exception(
          "Could not index " + field_name + ": already indexed/indexing");
      return;
    }
  }

  /**
   * Removes an index for a given field in the schema
   *
   * @param field_name The name of the field to index
   * @param ex The exception when the index could not be removed
   */
  void remove_index_task(const std::string& field_name,
                         optional<management_exception>& ex) {
    uint16_t idx;
    try {
      idx = schema_.get_field_index(field_name);
    } catch (std::exception& e) {
      ex = management_exception(
          "Could not remove index for " + field_name + " : " + e.what());
      return;
    }

    if (!schema_[idx].disable_indexing()) {
      ex = management_exception(
          "Could not remove index for " + field_name + ": No index exists");
      return;
    }
  }

  /**
   * Adds a filter to be executed on the data
   *
   * @param name The name of the filter
   * @param expr The filter expression to execute
   * @param ex The exception when the filter could not be added
   */
  void add_filter_task(const std::string& name, const std::string& expr,
                       optional<management_exception>& ex) {
    filter_id_t filter_id;
    if (filter_map_.get(name, filter_id) != -1) {
      ex = management_exception("Filter " + name + " already exists.");
      return;
    }
    auto t = parser::parse_expression(expr);
    auto cexpr = parser::compile_expression(t, schema_);
    filter_id = filters_.push_back(new filter(cexpr, default_filter));
    metadata_.write_filter_metadata(name, expr);
    if (filter_map_.put(name, filter_id) == -1) {
      ex = management_exception(
          "Could not add filter " + name + " to filter map.");
      return;
    }
  }

  /**
   * Removes a filter that was created
   *
   * @param name The name of the filter
   * @param ex The exception when the filter could not be removed
   */
  void remove_filter_task(const std::string& name,
                          optional<management_exception>& ex) {
    filter_id_t filter_id;
    if (filter_map_.get(name, filter_id) == -1) {
      ex = management_exception("Filter " + name + " does not exist.");
      return;
    }
    bool success = filters_.at(filter_id)->invalidate();
    if (!success) {
      ex = management_exception("Filter already invalidated.");
      return;
    }
    filter_map_.remove(name, filter_id);
  }

  /**
   * Adds an aggregate 
   *
   * @param name The name of the aggregate
   * @param filter_name The name of the filter
   * @param expr The filter expression
   * @param ex The exception if the aggregate could not be added
   */
  void add_aggregate_task(const std::string& name,
                          const std::string& filter_name,
                          const std::string& expr,
                          optional<management_exception>& ex) {
    aggregate_id_t aggregate_id;
    if (aggregate_map_.get(name, aggregate_id) != -1) {
      ex = management_exception("Aggregate " + name + " already exists.");
      return;
    }
    filter_id_t filter_id;
    if (filter_map_.get(filter_name, filter_id) == -1) {
      ex = management_exception("Filter " + filter_name + " does not exist.");
      return;
    }
    aggregate_id.filter_idx = filter_id;
    auto pa = parser::parse_aggregate(expr);
    const column_t& col = schema_[pa.field_name];
    aggregate_info *a = new aggregate_info(
        name, aggregate_manager::get_aggregator(pa.agg), col.idx());
    aggregate_id.aggregate_idx = filters_.at(filter_id)->add_aggregate(a);
    if (aggregate_map_.put(name, aggregate_id) == -1) {
      ex = management_exception(
          "Could not add trigger " + filter_name + " to trigger map.");
      return;
    }
    metadata_.write_aggregate_metadata(name, filter_name, expr);
  }

  /**
   * Removes an aggregate
   *
   * @param name The name of the aggregate to remove
   * @param ex The exception if the aggregate cannot be removed
   */
  void remove_aggregate_task(const std::string& name,
                             optional<management_exception>& ex) {
    aggregate_id_t aggregate_id;
    if (aggregate_map_.get(name, aggregate_id) == -1) {
      ex = management_exception("Aggregate " + name + " does not exist.");
      return;
    }
    bool success = filters_.at(aggregate_id.filter_idx)->remove_aggregate(
        aggregate_id.aggregate_idx);
    if (!success) {
      ex = management_exception("Aggregate already invalidated.");
      return;
    }
    aggregate_map_.remove(name, aggregate_id);
  }

  /**
   * Adds a trigger to the atomic multilog
   *
   * @param name The name of the trigger
   * @param expr The trigger expression
   * @param periodicity_ms The periodicity of the trigger measured in
   * milliseconds
   * @param ex The exception when the trigger cannot be added
   */
  void add_trigger_task(const std::string& name, const std::string& expr,
                        uint64_t periodicity_ms,
                        optional<management_exception>& ex) {
    trigger_id_t trigger_id;
    if (trigger_map_.get(name, trigger_id) != -1) {
      ex = management_exception("Trigger " + name + " already exists.");
      return;
    }
    auto pt = parser::parse_trigger(expr);
    std::string aggregate_name = pt.aggregate_name;
    aggregate_id_t aggregate_id;
    if (aggregate_map_.get(aggregate_name, aggregate_id) == -1) {
      ex = management_exception(
          "Aggregate " + aggregate_name + " does not exist.");
      return;
    }
    trigger_id.aggregate_id = aggregate_id;
    aggregate_info* a =
        filters_.at(aggregate_id.filter_idx)->get_aggregate_info(
            aggregate_id.aggregate_idx);
    trigger* t = new trigger(name, aggregate_name,
                             relop_utils::str_to_op(pt.relop),
                             a->value(pt.threshold), periodicity_ms);
    trigger_id.trigger_idx = a->add_trigger(t);
    if (trigger_map_.put(name, trigger_id) == -1) {
      ex = management_exception(
          "Could not add trigger " + name + " to trigger map.");
      return;
    }
    metadata_.write_trigger_metadata(name, expr, periodicity_ms);
  }

  /**
   * Removes a trigger from the atomic multilog
   *
   * @param name The name of the trigger
   * @param ex The exception when the trigger could not be removed
   */
  void remove_trigger_task(const std::string& name,
                           optional<management_exception>& ex) {
    trigger_id_t trigger_id;
    if (trigger_map_.get(name, trigger_id) == -1) {
      ex = management_exception("Trigger " + name + " does not exist.");
      return;
    }
    size_t fid = trigger_id.aggregate_id.filter_idx;
    size_t aid = trigger_id.aggregate_id.aggregate_idx;
    size_t tid = trigger_id.trigger_idx;
    bool success = filters_.at(fid)->get_aggregate_info(aid)->remove_trigger(
        tid);
    if (!success) {
      ex = management_exception("Trigger already invalidated.");
      return;
    }
    trigger_map_.remove(name, trigger_id);
  }

  /**
   * Monitors the task executed on the atomic multilog
   */
  void monitor_task() {
    uint64_t cur_ms = time_utils::cur_ms();
    uint64_t version = rt_.get();
    size_t nfilters = filters_.size();
    for (size_t i = 0; i < nfilters; i++) {
      filter* f = filters_.at(i);
      if (f->is_valid()) {
        size_t naggs = f->num_aggregates();
        for (size_t aid = 0; aid < naggs; aid++) {
          aggregate_info* a = f->get_aggregate_info(aid);
          if (a->is_valid()) {
            size_t ntriggers = a->num_triggers();
            for (size_t tid = 0; tid < ntriggers; tid++) {
              trigger* t = a->get_trigger(tid);
              if (t->is_valid() && cur_ms % t->periodicity_ms() == 0) {
                for (uint64_t ms = cur_ms
                    - configuration_params::MONITOR_WINDOW_MS; ms <= cur_ms;
                    ms++) {
                  if (ms % t->periodicity_ms() == 0) {
                    check_time_bucket(f, t, tid, cur_ms, version);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  /**
   * Checks the time bucket and adds alerts when necessary
   *
   * @param f The filter 
   * @param t The trigger
   * @param tid The thread identifier
   * @param time_bucket The time_bucket to check
   * @param version The version to check
   */
  void check_time_bucket(filter* f, trigger* t, size_t tid,
                         uint64_t time_bucket, uint64_t version) {
    size_t window_size = t->periodicity_ms();
    for (uint64_t ms = time_bucket - window_size; ms <= time_bucket; ms++) {
      const aggregated_reflog* ar = f->lookup(ms);
      if (ar != nullptr) {
        numeric agg = ar->get_aggregate(tid, version);
        if (numeric::relop(t->op(), agg, t->threshold())) {
          alerts_.add_alert(ms, t->name(), t->expr(), agg, version);
        }
      }
    }
  }

  std::string name_;
  schema_type schema_;

  data_log_type data_log_;
  read_tail_type rt_;
  metadata_writer_type metadata_;

  // In memory structures
  filter_log filters_;
  index_log indexes_;
  alert_index alerts_;

  string_map<filter_id_t> filter_map_;
  string_map<aggregate_id_t> aggregate_map_;
  string_map<trigger_id_t> trigger_map_;

  query_planner planner_;

  // Archival
  archiver* archiver_;
  periodic_task archival_task_;

  // Manangement
  task_pool& mgmt_pool_;
  periodic_task monitor_task_;
}
;

}

#endif /* CONFLUO_ATOMIC_MULTILOG_H_ */
