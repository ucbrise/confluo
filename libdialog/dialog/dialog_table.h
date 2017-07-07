#ifndef DIALOG_DIALOG_STORE_H_
#define DIALOG_DIALOG_STORE_H_

#include <functional>
#include <numeric>
#include <thread>

#include "storage.h"
#include "monolog.h"
#include "time_utils.h"
#include "string_utils.h"
#include "filter.h"
#include "filter_info.h"
#include "trigger_info.h"
#include "schema.h"
#include "expression_compiler.h"
#include "read_tail.h"

using namespace ::dialog::monolog;
using namespace ::dialog::monitor;
using namespace ::utils;

// TODO: Add more tests
// TODO: Improve documentation

namespace dialog {

/**
 * Management exception.
 */
class management_exception : public std::exception {
 public:
  management_exception(const std::string& msg)
      : msg_(msg) {
  }

  const char* what() const noexcept {
    return msg_.c_str();
  }

 private:
  const std::string msg_;
};

template<class storage_mode = storage::in_memory>
class dialog_table {
 public:
  template<typename T, class sm>
  using aux_log_t = monolog_linear<T, 256, 65536, 0, sm>;

  typedef monolog::monolog_exp2<uint64_t, 24> reflog;
  typedef index::tiered_index<reflog, 2, 1> idx_bool_t;
  typedef index::tiered_index<reflog, 256, 1> idx1_t;
  typedef index::tiered_index<reflog, 256, 2> idx2_t;
  typedef index::tiered_index<reflog, 256, 4> idx4_t;
  typedef index::tiered_index<reflog, 256, 8> idx8_t;

  dialog_table(const std::vector<column_t>& table_schema,
               const std::string& path = ".")
      : rt_(path),
        schema_(path, table_schema) {
    data_log_.init("data_log", path);
    filter_info_.init("filters", path);
    trigger_info_.init("triggers", path);
  }

  dialog_table(const schema_builder& builder, const std::string& path = ".")
      : rt_(path),
        schema_(path, builder.get_schema()) {
    data_log_.init("data_log", path);
    filter_info_.init("filters", path);
    trigger_info_.init("triggers", path);
  }

  // Management interface
  void add_index(const std::string& field_name) {
    uint16_t idx;
    try {
      idx = schema_.name_map.at(string_utils::to_upper(field_name));
    } catch (std::exception& e) {
      throw management_exception(
          "Could not add index for " + field_name + " : " + e.what());
    }
    bool success = schema_.columns[idx].set_indexing();
    if (success) {
      uint16_t index_id;
      switch (schema_.columns[idx].type().size) {
        case 1:
          index_id = idx1_.push_back(new idx1_t);
          break;
        case 2:
          index_id = idx2_.push_back(new idx2_t);
          break;
        case 4:
          index_id = idx4_.push_back(new idx4_t);
          break;
        case 8:
          index_id = idx8_.push_back(new idx8_t);
          break;
        default:
          schema_.columns[idx].set_unindexed();
          throw management_exception("Index not supported for field type");
      }
      schema_.columns[idx].set_indexed(index_id);
    } else {
      throw management_exception(
          "Could not add index for " + field_name
              + ": Already indexed or indexing");
    }
  }

  void remove_index(const std::string& field_name) {
    uint16_t idx;
    try {
      idx = schema_.name_map.at(string_utils::to_upper(field_name));
    } catch (std::exception& e) {
      throw management_exception(
          "Could not remove index for " + field_name + " : " + e.what());
    }

    if (!schema_.columns[idx].disable_indexing()) {
      throw management_exception(
          "Could not remove index for " + field_name + ": No index exists");
    }
  }

  uint32_t add_filter(const std::string& expression, size_t monitor_ms) {
    auto cexpr = expression_compiler::compile(expression, schema_);
    filter_info f_info;
    f_info.set_expression(expression);
    f_info.filter_id = filter_info_.reserve(1);
    filter_info_[f_info.filter_id] = f_info;
    filter_info_.flush(f_info.filter_id, 1);
    filters_[f_info.filter_id] = new filter(cexpr, monitor_ms);
    return f_info.filter_id;
  }

  uint32_t add_trigger(uint32_t filter_id, const std::string& field_name,
                       aggregate_id agg, field_t threshold) {
    trigger_info t_info;
    t_info.filter_id = filter_id;
    t_info.agg_type = agg;
    t_info.col = schema_.lookup(field_name);
    t_info.trigger_id = trigger_info_.reserve(1);
    trigger_info_[t_info.trigger_id] = t_info;
    trigger_info_.flush(t_info.trigger_id, 1);
    return t_info.trigger_id;
  }

  uint64_t append(const void* data, size_t length, uint64_t ts =
                      time_utils::cur_ns()) {
    uint64_t offset = data_log_.append((const uint8_t*) data, length);
    record_t r = schema_.apply(offset, data, length, ts);

    size_t nfilters = filters_.size();
    for (size_t i = 0; i < nfilters; i++)
      filters_.at(i)->update(r);

    for (const field_t& f : r) {
      if (f.is_indexed()) {
        switch (f.type().size) {
          case 1:
            (*idx1_.at(f.index_id()))[f.get_uint64<uint8_t>()]->push_back(
                offset);
            break;
          case 2:
            (*idx2_.at(f.index_id()))[f.get_uint64<uint16_t>()]->push_back(
                offset);
            break;
          case 4:
            (*idx2_.at(f.index_id()))[f.get_uint64<uint32_t>()]->push_back(
                offset);
            break;
          case 8:
            (*idx8_.at(f.index_id()))[f.get_uint64<uint64_t>()]->push_back(
                offset);
            break;
          default:
            throw management_exception("Field type should not be indexed");
        }
      }
    }

    data_log_.flush(offset, length);
    rt_.advance(offset, length);
    return offset;
  }

  uint64_t append_batch(const std::vector<void*>& batch,
                        std::vector<size_t>& lengths, uint64_t ts =
                            time_utils::cur_ns()) {
    size_t length = std::accumulate(lengths.begin(), lengths.end(), 0);
    uint64_t offset = data_log_.reserve(length);
    uint64_t off = offset;
    for (size_t i = 0; i < batch.size(); i++) {
      data_log_.write(off, (const uint8_t*) batch.at(i), lengths.at(i));
      off += lengths.at(i);
    }
    data_log_.flush(offset, length);
    rt_.advance(offset, length);
    return offset;
  }

  void* ptr(uint64_t offset, uint64_t tail) const {
    if (offset < tail)
      return data_log_.cptr(offset);
    return nullptr;
  }

  void* ptr(uint64_t offset) const {
    return ptr(offset, rt_.get());
  }

  bool read(uint64_t offset, void* data, size_t length, uint64_t tail) const {
    if (offset < tail) {
      data_log_.read(offset, (uint8_t*) data, length);
      return true;
    }
    return false;
  }

  bool get(uint64_t offset, uint8_t* data, size_t length) const {
    return read(offset, (uint8_t*) data, length, rt_.get());
  }

  size_t num_records() const {
    return rt_.get();
  }

 protected:
  monolog_linear<uint8_t, 65536, 1073741824, 1048576, storage_mode> data_log_;
  read_tail<storage_mode> rt_;
  schema_t<storage_mode> schema_;
  aux_log_t<filter_info, storage_mode> filter_info_;
  aux_log_t<trigger_info, storage_mode> trigger_info_;
  aux_log_t<filter*, storage::in_memory> filters_;
  aux_log_t<idx_bool_t*, storage::in_memory> idx_bool_;
  aux_log_t<idx1_t*, storage::in_memory> idx1_;
  aux_log_t<idx2_t*, storage::in_memory> idx2_;
  aux_log_t<idx4_t*, storage::in_memory> idx4_;
  aux_log_t<idx8_t*, storage::in_memory> idx8_;
};

}

#endif /* DIALOG_DIALOG_STORE_H_ */
