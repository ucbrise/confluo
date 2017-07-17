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
#include "table_metadata.h"
#include "data_log.h"
#include "expression_compiler.h"
#include "query_planner.h"
#include "record_stream.h"
#include "radix_tree.h"
#include "tiered_index.h"
#include "filter.h"
#include "trigger.h"
#include "exceptions.h"

#include "time_utils.h"
#include "string_utils.h"

using namespace ::dialog::monolog;
using namespace ::dialog::index;
using namespace ::dialog::monitor;
using namespace ::utils;

// TODO: Add more tests
// TODO: Improve documentation

namespace dialog {

template<class storage_mode = storage::in_memory>
class dialog_table {
 public:
  typedef data_log<storage_mode> data_log_type;
  typedef schema_t<storage_mode> schema_type;
  typedef read_tail<storage_mode> read_tail_type;
  typedef metadata_writer<storage_mode> metadata_writer_type;
  typedef radix_tree::range_iterator rt_offset_it;

  typedef record_stream<rt_offset_it, schema_type, data_log_type> rstream_type;
  typedef filtered_record_stream<rstream_type> fstream_type;
  typedef union_record_stream<fstream_type> filter_result_type;

  dialog_table(const std::vector<column_t>& table_schema,
               const std::string& path = ".")
      : data_log_("data_log", path),
        rt_(path),
        schema_(path, table_schema),
        metadata_(path) {
  }

  dialog_table(const schema_builder& builder, const std::string& path = ".")
      : data_log_("data_log", path),
        rt_(path),
        schema_(path, builder.get_columns()),
        metadata_(path) {
  }

  // Management interface
  void add_index(const std::string& field_name, double bucket_size) {
    uint16_t idx;
    try {
      idx = schema_.name_map.at(string_utils::to_upper(field_name));
    } catch (std::exception& e) {
      THROW(management_exception,
            "Could not add index for " + field_name + " : " + e.what());
    }

    column_t& col = schema_[idx];
    bool success = col.set_indexing();
    if (success) {
      uint16_t index_id;
      switch (col.type().id) {
        case type_id::D_BOOL: {
          index_id = idx_.push_back(new radix_tree(1, 2));
          break;
        }
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
          THROW(management_exception, "Index not supported for field type");
      }
      col.set_indexed(index_id, bucket_size);
      metadata_.write_index_info(index_id, field_name, bucket_size);
    } else {
      THROW(management_exception,
            "Could not index " + field_name + ": already indexed/indexing");
    }
  }

  void remove_index(const std::string& field_name) {
    uint16_t idx;
    try {
      idx = schema_.name_map.at(string_utils::to_upper(field_name));
    } catch (std::exception& e) {
      THROW(management_exception,
            "Could not remove index for " + field_name + " : " + e.what());
    }

    if (!schema_.columns[idx].disable_indexing()) {
      THROW(management_exception,
            "Could not remove index for " + field_name + ": No index exists");
    }
  }

  uint32_t add_filter(const std::string& expression, size_t monitor_ms) {
    auto cexpr = expression_compiler::compile(expression, schema_);
    uint32_t filter_id = filters_.push_back(new filter(cexpr, monitor_ms));
    metadata_.write_filter_info(filter_id, expression);
    return filter_id;
  }

  uint32_t add_trigger(uint32_t filter_id, const std::string& field_name,
                       aggregate_id agg, relop_id op,
                       const mutable_value_t& threshold) {
    trigger *t = new trigger(filter_id, op, threshold);
    uint32_t trigger_id = triggers_.push_back(t);
    metadata_.write_trigger_info(trigger_id, filter_id, agg, field_name, op,
                                 threshold);
    return trigger_id;
  }

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

    for (const field_t& f : r)
      if (f.is_indexed())
        idx_.at(f.index_id())->insert(f.get_key(), offset);

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
    query_plan plan = query_planner::plan(cexpr, idx_);
    std::vector<fstream_type> fstreams;
    for (minterm_plan& mplan : plan) {
      const index_filter& f = mplan.idx_filter();
      auto mres = idx_.at(f.index_id())->range_lookup(f.kbegin(), f.kend());
      rstream_type rs(version, mres.begin(), mres.end(), schema_, data_log_);
      fstreams.push_back(fstream_type(rs, mplan.data_filter()));
    }
    return filter_result_type(fstreams);
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
};

}

#endif /* DIALOG_DIALOG_STORE_H_ */
