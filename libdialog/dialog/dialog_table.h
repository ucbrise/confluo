#ifndef DIALOG_DIALOG_STORE_H_
#define DIALOG_DIALOG_STORE_H_

#include <math.h>

#include <functional>
#include <numeric>
#include <thread>

#include "storage.h"
#include "monolog.h"
#include "read_tail.h"
#include "auxlog.h"
#include "schema.h"
#include "string_map.h"
#include "table_metadata.h"
#include "data_log.h"
#include "task_worker.h"
#include "expression_compiler.h"
#include "query_planner.h"
#include "record_stream.h"
#include "radix_tree.h"
#include "filter.h"
#include "trigger.h"
#include "exceptions.h"

#include "optional.h"

#include "time_utils.h"
#include "string_utils.h"
#include "task_pool.h"

using namespace ::dialog::monolog;
using namespace ::dialog::index;
using namespace ::dialog::monitor;
using namespace ::utils;

// TODO: Add more tests
// TODO: Improve documentation

namespace dialog {

class dialog_table {
 public:
  typedef data_log data_log_type;
  typedef schema_t schema_type;
  typedef read_tail read_tail_type;
  typedef metadata_writer metadata_writer_type;
  typedef radix_tree::rt_range_result rt_offset_list;

  typedef record_stream<rt_offset_list, schema_type, data_log_type> rstream_type;
  typedef filtered_record_stream<rstream_type> fstream_type;
  typedef union_record_stream<fstream_type> filter_result_type;

  dialog_table(const std::vector<column_t>& table_schema,
               const std::string& path, const storage::storage_mode& storage,
               task_pool& pool)
      : data_log_("data_log", path, storage),
        rt_(path, storage),
        schema_(path, table_schema),
        metadata_(path, storage.id),
        mgmt_pool_(pool) {
  }

  // Management ops
  void add_index(const std::string& field_name, double bucket_size = 1.0) {
    optional<management_exception> ex;
    auto ret =
        mgmt_pool_.submit(
            [field_name, bucket_size, &ex, this] {

              uint16_t idx;
              try {
                idx = schema_.get_field_index(field_name);
              } catch (std::exception& e) {
                ex = management_exception("Could not add index for " + field_name + " : " + e.what());
              }

              column_t& col = schema_[idx];
              bool success = col.set_indexing();
              if (success) {
                uint16_t index_id = UINT16_MAX;
                switch (col.type().id) {
                  case type_id::D_BOOL:
                    index_id = idx_.push_back(new radix_tree(1, 2));
                    break;
                  case type_id::D_CHAR:
                  case type_id::D_SHORT:
                  case type_id::D_INT:
                  case type_id::D_LONG:
                  case type_id::D_FLOAT:
                  case type_id::D_DOUBLE:
                  case type_id::D_STRING:
                    index_id = idx_.push_back(new radix_tree(col.type().size, 256));
                    break;
                  default:
                    col.set_unindexed();
                    ex = management_exception("Index not supported for field type");
                }
                col.set_indexed(index_id, bucket_size);
                metadata_.write_index_info(field_name, bucket_size);
              } else {
                ex = management_exception("Could not index " + field_name + ": already indexed/indexing");
              }
            });

    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  void remove_index(const std::string& field_name) {
    optional<management_exception> ex;
    auto ret =
        mgmt_pool_.submit(
            [field_name, &ex, this] {
              uint16_t idx;
              try {
                idx = schema_.get_field_index(field_name);
              } catch (std::exception& e) {
                ex = management_exception("Could not remove index for " + field_name + " : " + e.what());
              }

              if (!schema_[idx].disable_indexing()) {
                ex = management_exception("Could not remove index for " + field_name + ": No index exists");
              }
            });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  void add_filter(const std::string& filter_name,
                  const std::string& expression) {
    optional<management_exception> ex;
    auto ret =
        mgmt_pool_.submit(
            [filter_name, expression, &ex, this] {
              uint32_t filter_id;
              if (filter_map_.get(filter_name, filter_id)) {
                ex = management_exception("Filter " + filter_name + " already exists.");
              }
              auto cexpr = expression_compiler::compile(expression, schema_);
              filter_id = filters_.push_back(new filter(cexpr, default_filter));
              metadata_.write_filter_info(filter_name, expression);
              bool success = filter_map_.put(filter_name, filter_id);
              if (!success) {
                ex = management_exception("Could not add filter " + filter_name + " to filter map.");
              }
            });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  void add_trigger(const std::string& trigger_name,
                   const std::string& filter_name,
                   const std::string& field_name, aggregate_id agg, relop_id op,
                   const numeric& threshold) {
    optional<management_exception> ex;
    auto ret =
        mgmt_pool_.submit(
            [trigger_name, filter_name, field_name, agg, op, threshold, &ex, this] {
              uint32_t trigger_id;
              if (trigger_map_.get(trigger_name, trigger_id)) {
                ex = management_exception("Trigger " + trigger_name + " already exists.");
              }
              uint32_t filter_id;
              if (!filter_map_.get(filter_name, filter_id)) {
                ex = management_exception("Filter " + filter_name + " does not exist.");
              }
              const column_t& col = schema_[field_name];
              trigger_id = triggers_.push_back(new trigger(agg, col.idx(), col.type(), op, threshold));
              metadata_.write_trigger_info(trigger_name, filter_name, agg, field_name, op,
                  threshold);
            });
    ret.wait();
    if (ex.has_value())
      throw ex.value();
  }

  // Query ops
  uint64_t append(void* data, size_t length, uint64_t ts =
                      time_utils::cur_ns()) {
    size_t record_length = length + sizeof(uint64_t);
    uint64_t offset = data_log_.reserve(record_length);
    data_log_.write(offset, (const uint8_t*) &ts, sizeof(uint64_t));
    data_log_.write(offset + sizeof(uint64_t), (const uint8_t*) data, length);

    record_t r = schema_.apply(offset, data_log_.ptr(offset), length);

    size_t nfilters = filters_.size();
    for (size_t i = 0; i < nfilters; i++)
      filters_.at(i)->update(r);

    for (const field_t& f : r) {
      if (f.is_indexed()) {
        idx_.at(f.index_id())->insert(f.get_key(), offset);
      }
    }

    data_log_.flush(offset, record_length);
    rt_.advance(offset, record_length);
    return offset;
  }

  bool read(uint64_t offset, record_t& rec) const {
    uint64_t version = rt_.get();
    if (offset < version) {
      rec = record_t(offset, data_log_.cptr(offset), schema_.record_size());
      return true;
    }
    return false;
  }

  filter_result_type execute_filter(const std::string& expr) const {
    uint64_t version = rt_.get();
    compiled_expression cexpr = expression_compiler::compile(expr, schema_);
    query_planner planner(cexpr, idx_);
    query_plan plan = planner.plan();
    std::vector<fstream_type> fstreams;
    for (minterm_plan& mplan : plan) {
      const index_filter& f = mplan.idx_filter();
      auto mres = idx_.at(f.index_id())->range_lookup(f.kbegin(), f.kend());
      rstream_type rs(version, mres, schema_, data_log_);
      fstreams.push_back(fstream_type(rs, mplan.data_filter()));
    }
    return filter_result_type(fstreams);
  }

  rstream_type query_filter(const std::string& filter_name,
                            uint64_t ts_block_begin,
                            uint64_t ts_block_end) const {

    uint32_t filter_id;
    if (!filter_map_.get(filter_name, filter_id)) {
      THROW(management_exception, "Filter " + filter_name + " does not exist.");
    }
    uint64_t version = rt_.get();
    auto res = filters_.at(filter_id)->lookup_range(ts_block_begin,
                                                    ts_block_end);
    return rstream_type(version, res, schema_, data_log_);
  }

  fstream_type query_filter(const std::string& filter_name,
                            const std::string& expr, uint64_t ts_block_begin,
                            uint64_t ts_block_end) const {
    uint32_t filter_id;
    if (!filter_map_.get(filter_name, filter_id)) {
      THROW(management_exception, "Filter " + filter_name + " does not exist.");
    }
    uint64_t version = rt_.get();
    compiled_expression cexpr = expression_compiler::compile(expr, schema_);
    auto res = filters_.at(filter_id)->lookup_range(ts_block_begin,
                                                    ts_block_end);
    rstream_type rs(version, res, schema_, data_log_);
    return fstream_type(rs, cexpr);
  }

  size_t num_records() const {
    return rt_.get();
  }

 protected:
  data_log_type data_log_;
  read_tail_type rt_;
  schema_type schema_;
  metadata_writer_type metadata_;

  // In memory structures
  filter_list_type filters_;
  trigger_list_type triggers_;
  index_list_type idx_;

  string_map<uint32_t> filter_map_;
  string_map<uint32_t> trigger_map_;

  // Manangement
  task_pool& mgmt_pool_;
};

}

#endif /* DIALOG_DIALOG_STORE_H_ */
