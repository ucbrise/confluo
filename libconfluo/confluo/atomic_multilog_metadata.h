#ifndef CONFLUO_ATOMIC_MULTILOG_METADATA_H_
#define CONFLUO_ATOMIC_MULTILOG_METADATA_H_

#include <cstdint>
#include <string>
#include <fstream>

#include "types/numeric.h"
#include "types/type_manager.h"
#include "io_utils.h"
#include "schema/schema.h"
#include "storage/storage.h"

using namespace utils;

namespace confluo {

/**
 * Different types of metadata
 */
enum metadata_type
  : uint32_t {
    D_SCHEMA_METADATA = 0,
  D_INDEX_METADATA = 1,
  D_FILTER_METADATA = 2,
  D_AGGREGATE_METADATA = 3,
  D_TRIGGER_METADATA = 4
};

/**
 * Efficient lookup for metadata
 */
struct index_metadata {
 public:
  /**
   * @param
   */
  index_metadata(const std::string& field_name, double bucket_size)
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

struct filter_metadata {
 public:
  filter_metadata(const std::string& filter_name, const std::string& expr)
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

struct aggregate_metadata {
 public:
  aggregate_metadata(const std::string& name, const std::string& filter_name,
                     const std::string& expr)
      : name_(name),
        filter_name_(filter_name),
        expr_(expr) {
  }

  const std::string& aggregate_name() const {
    return name_;
  }

  const std::string& filter_name() const {
    return filter_name_;
  }

  const std::string& aggregate_expression() const {
    return expr_;
  }

 private:
  std::string name_;
  std::string filter_name_;
  std::string expr_;
};

struct trigger_metadata {
 public:
  trigger_metadata(const std::string& name, const std::string& expr,
                   uint64_t periodicity_ms)
      : name_(name),
        expr_(expr),
        periodicity_ms_(periodicity_ms) {
  }

  const std::string& trigger_name() const {
    return name_;
  }

  const std::string& trigger_expression() const {
    return expr_;
  }

  uint64_t periodicity_ms() const {
    return periodicity_ms_;
  }

 private:
  std::string name_;
  std::string expr_;
  uint64_t periodicity_ms_;
};

class metadata_writer {
 public:
  /**
   * Constructor that initializes metadata writer
   * @param path The path of where the metadata is
   * @param id The id of the storage type 
   */
  metadata_writer(const std::string& path, storage::storage_mode id)
      : filename_(path + "/metadata"),
        id_(id) {
    if (id_ != storage::storage_mode::IN_MEMORY) {
      out_.open(filename_);
    }
  }

  void write_schema(const schema_t& schema) {
    if (id_ != storage::storage_mode::IN_MEMORY) {
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

  void write_index_metadata(const std::string& name, double bucket_size) {
    if (id_ != storage::storage_mode::IN_MEMORY) {
      metadata_type type = metadata_type::D_INDEX_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, name);
      io_utils::write(out_, bucket_size);
      io_utils::flush(out_);
    }
  }

  void write_filter_metadata(const std::string& name, const std::string& expr) {
    if (id_ != storage::storage_mode::IN_MEMORY) {
      metadata_type type = metadata_type::D_FILTER_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, name);
      io_utils::write(out_, expr);
      io_utils::flush(out_);
    }
  }

  void write_aggregate_metadata(const std::string& name,
                                const std::string& filter_name,
                                const std::string& expr) {
    if (id_ != storage::storage_mode::IN_MEMORY) {
      metadata_type type = metadata_type::D_AGGREGATE_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, name);
      io_utils::write(out_, filter_name);
      io_utils::write(out_, expr);
      io_utils::flush(out_);
    }
  }

  void write_trigger_metadata(const std::string& trigger_name,
                              const std::string& trigger_expr,
                              const uint64_t periodicity_ms) {
    if (id_ != storage::storage_mode::IN_MEMORY) {
      metadata_type type = metadata_type::D_TRIGGER_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, trigger_name);
      io_utils::write(out_, trigger_expr);
      io_utils::write(out_, periodicity_ms);
      io_utils::flush(out_);
    }
  }

 private:
  std::string filename_;
  std::ofstream out_;
  storage::storage_mode id_;
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

  index_metadata next_index_metadata() {
    std::string field_name = io_utils::read<std::string>(in_);
    double bucket_size = io_utils::read<double>(in_);
    return index_metadata(field_name, bucket_size);
  }

  filter_metadata next_filter_metadata() {
    std::string filter_name = io_utils::read<std::string>(in_);
    std::string expr = io_utils::read<std::string>(in_);
    return filter_metadata(filter_name, expr);
  }

  aggregate_metadata next_aggregate_metadata() {
    std::string name = io_utils::read<std::string>(in_);
    std::string filter_name = io_utils::read<std::string>(in_);
    std::string expr = io_utils::read<std::string>(in_);
    return aggregate_metadata(name, filter_name, expr);
  }

  trigger_metadata next_trigger_metadata() {
    std::string trigger_name = io_utils::read<std::string>(in_);
    std::string trigger_expr = io_utils::read<std::string>(in_);
    uint64_t periodicity_ms = io_utils::read<uint64_t>(in_);
    return trigger_metadata(trigger_name, trigger_expr, periodicity_ms);
  }

 private:
  std::string filename_;
  std::ifstream in_;
};

}

#endif /* CONFLUO_ATOMIC_MULTILOG_METADATA_H_ */
