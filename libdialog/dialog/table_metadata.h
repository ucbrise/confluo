#ifndef DIALOG_TABLE_METADATA_H_
#define DIALOG_TABLE_METADATA_H_

#include <cstdint>
#include <string>
#include <fstream>

#include "schema.h"
#include "storage.h"
#include "aggregate_types.h"
#include "io_utils.h"
#include "types/numeric.h"
#include "types/type_manager.h"

using namespace utils;

namespace confluo {

enum metadata_type
  : uint32_t {
    D_SCHEMA_METADATA = 0,
  D_INDEX_METADATA = 1,
  D_FILTER_METADATA = 2,
  D_TRIGGER_METADATA = 3
};

struct index_info {
 public:
  index_info(const std::string& field_name, double bucket_size)
      : field_name_(field_name),
        bucket_size_(bucket_size) {
  }

  std::string field_name() const {
    return field_name_;
  }

  double bucket_size() const {
    return bucket_size_;
  }

 private:
  std::string field_name_;
  double bucket_size_;
};

struct filter_info {
 public:
  filter_info(const std::string& filter_name, const std::string& expr)
      : filter_name_(filter_name),
        expr_(expr) {
  }

  const std::string& filter_name() const {
    return filter_name_;
  }

  const std::string& expr() const {
    return expr_;
  }

 private:
  std::string filter_name_;
  std::string expr_;
};

struct trigger_info {
 public:
  trigger_info(const std::string& trigger_name, const std::string& filter_name,
               aggregate_id agg_id, const std::string& field_name,
               reational_op_id op, const numeric& threshold,
               uint64_t periodicity_ms)
      : trigger_id_(trigger_name),
        filter_id_(filter_name),
        agg_id_(agg_id),
        field_name_(field_name),
        op_(op),
        threshold_(threshold),
        periodicity_ms_(periodicity_ms) {
  }

  const std::string& trigger_name() const {
    return trigger_id_;
  }

  const std::string& filter_name() const {
    return filter_id_;
  }

  aggregate_id agg_id() const {
    return agg_id_;
  }

  reational_op_id op() const {
    return op_;
  }

  const std::string& field_name() const {
    return field_name_;
  }

  const numeric& threshold() const {
    return threshold_;
  }

  uint64_t periodicity_ms() const {
    return periodicity_ms_;
  }

 private:
  std::string trigger_id_;
  std::string filter_id_;
  aggregate_id agg_id_;
  std::string field_name_;
  reational_op_id op_;
  numeric threshold_;
  uint64_t periodicity_ms_;
};

class metadata_writer {
 public:
  /**
   * Constructor that initializes metadata writer
   * @param path The path of where the metadata is
   * @param id The id of the storage type 
   */
  metadata_writer(const std::string& path, storage::storage_id id)
      : filename_(path + "/metadata"),
        id_(id) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      out_.open(filename_);
    }
  }

  void write_schema(const schema_t& schema) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      metadata_type type = metadata_type::D_SCHEMA_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, schema.columns().size());
      for (auto& col : schema.columns()) {
        io_utils::write(out_, col.name());
        col.type().serialize(out_);
      }
      io_utils::flush(out_);
    }
  }

  void write_index_info(const std::string& name, double bucket_size) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      metadata_type type = metadata_type::D_INDEX_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, name);
      io_utils::write(out_, bucket_size);
      io_utils::flush(out_);
    }
  }

  void write_filter_info(const std::string& filter_name,
                         const std::string& expr) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      metadata_type type = metadata_type::D_FILTER_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, filter_name);
      io_utils::write(out_, expr);
      io_utils::flush(out_);
    }
  }

  void write_trigger_info(const std::string& trigger_name,
                          const std::string& filter_name, aggregate_id agg_id,
                          const std::string& field_name, reational_op_id op,
                          const numeric& threshold,
                          const uint64_t periodicity_ms) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      metadata_type type = metadata_type::D_TRIGGER_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, trigger_name);
      io_utils::write(out_, filter_name);
      io_utils::write(out_, agg_id);
      io_utils::write(out_, field_name);
      io_utils::write(out_, op);
      threshold.type().serialize(out_);
      threshold.type().serialize_op()(out_, threshold.to_data());
      io_utils::write(out_, periodicity_ms);
      io_utils::flush(out_);
    }
  }

 private:
  std::string filename_;
  std::ofstream out_;
  storage::storage_id id_;
};

class metadata_reader {
 public:
  metadata_reader(const std::string& path)
      : filename_(path + "/metadata"),
        in_(filename_) {
  }

  metadata_type next_type() {
    return io_utils::read<metadata_type>(in_);
  }

  schema_t next_schema() {
    size_t ncolumns = io_utils::read<size_t>(in_);
    schema_builder builder;
    for (size_t i = 0; i < ncolumns; i++) {
      std::string name = io_utils::read<std::string>(in_);
      data_type type = data_type::deserialize(in_);
      builder.add_column(type, name);
    }
    return schema_t(builder.get_columns());
  }

  index_info next_index_info() {
    std::string field_name = io_utils::read<std::string>(in_);
    double bucket_size = io_utils::read<double>(in_);
    return index_info(field_name, bucket_size);
  }

  filter_info next_filter_info() {
    std::string filter_name = io_utils::read<std::string>(in_);
    std::string expr = io_utils::read<std::string>(in_);
    return filter_info(filter_name, expr);
  }

  trigger_info next_trigger_info() {
    std::string trigger_name = io_utils::read<std::string>(in_);
    std::string filter_name = io_utils::read<std::string>(in_);
    aggregate_id agg_id = io_utils::read<aggregate_id>(in_);
    std::string field_name = io_utils::read<std::string>(in_);
    reational_op_id op = io_utils::read<reational_op_id>(in_);
    data_type type = data_type::deserialize(in_);
    mutable_raw_data threshold_data(type.size);
    type.deserialize_op()(in_, threshold_data);
    numeric threshold(type, threshold_data.ptr);
    uint64_t periodicity_ms = io_utils::read<uint64_t>(in_);
    return trigger_info(trigger_name, filter_name, agg_id, field_name, op,
                        threshold, periodicity_ms);
  }

 private:
  std::string filename_;
  std::ifstream in_;
};

}

#endif /* DIALOG_TABLE_METADATA_H_ */
