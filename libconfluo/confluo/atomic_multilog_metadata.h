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
    /** Metadata for the schema */
    D_SCHEMA_METADATA = 0,
  /** Metadata for the index */
  D_INDEX_METADATA = 1,
  /** Metadata for filters */
  D_FILTER_METADATA = 2,
  /** Metadata for aggregates */
  D_AGGREGATE_METADATA = 3,
  /** Metadata for triggers */
  D_TRIGGER_METADATA = 4
};

/**
 * Efficient lookup for metadata
 */
struct index_metadata {
 public:
  /**
   * Constructs an index for the specified field name
   *
   * @param field_name The field_name to create an index for
   * @param bucket_size The bucket_size for lookup
   */
  index_metadata(const std::string& field_name, double bucket_size)
      : field_name_(field_name),
        bucket_size_(bucket_size) {
  }

  /**
   * Gets the field name
   *
   * @return The field name for the metadata
   */
  std::string field_name() const {
    return field_name_;
  }

  /**
   * Gets the bucket size
   *
   * @return The bucket size used for lookup
   */
  double bucket_size() const {
    return bucket_size_;
  }

 private:
  std::string field_name_;
  double bucket_size_;
};

/**
 * Metadata for the filter
 */
struct filter_metadata {
 public:
  /**
   * Constructs metadata for a filter
   *
   * @param filter_name The name of the filter
   * @param expr The filter expression
   */
  filter_metadata(const std::string& filter_name, const std::string& expr)
      : filter_name_(filter_name),
        expr_(expr) {
  }

  /**
   * Gets the filter name
   *
   * @return The name of the filter
   */
  const std::string& filter_name() const {
    return filter_name_;
  }

  /**
   * Gets the filter expression
   *
   * @return The filter expression
   */
  const std::string& expr() const {
    return expr_;
  }

 private:
  std::string filter_name_;
  std::string expr_;
};

/**
 * Metadata for the aggregate
 */
struct aggregate_metadata {
 public:
  /**
   * Constructs metadata for an aggregate
   *
   * @param name The name of the aggregate
   * @param filter_name The name of the associated filter
   * @param expr The expression of the associated filter
   */
  aggregate_metadata(const std::string& name, const std::string& filter_name,
                     const std::string& expr)
      : name_(name),
        filter_name_(filter_name),
        expr_(expr) {
  }

  /**
   * Gets the name of the aggregate
   *
   * @return The string containing the name of the aggregate
   */
  const std::string& aggregate_name() const {
    return name_;
  }

  /**
   * Gets the name of the filter
   *
   * @return The filter name
   */
  const std::string& filter_name() const {
    return filter_name_;
  }

  /**
   * Gets the aggregate expression
   *
   * @return A string containing the expression for the aggregate
   */
  const std::string& aggregate_expression() const {
    return expr_;
  }

 private:
  std::string name_;
  std::string filter_name_;
  std::string expr_;
};

/**
 * Metadata for triggers
 */
struct trigger_metadata {
 public:
     /**
      * Constructs metadata for the trigger
      *
      * @param name The name of the trigger
      * @param expr The epxression
      * @param periodicity_ms The periodicity of the trigger measured in
      * milliseconds
      */
  trigger_metadata(const std::string& name, const std::string& expr,
                   uint64_t periodicity_ms)
      : name_(name),
        expr_(expr),
        periodicity_ms_(periodicity_ms) {
  }

  /**
   * Gets the name of the trigger
   *
   * @return The trigger name
   */
  const std::string& trigger_name() const {
    return name_;
  }

  /**
   * Gets the expression for the trigger
   *
   * @return The trigger expression
   */
  const std::string& trigger_expression() const {
    return expr_;
  }

  /**
   * Gets the periodicity of the trigger
   *
   * @return The trigger periodicity in milliseconds
   */
  uint64_t periodicity_ms() const {
    return periodicity_ms_;
  }

 private:
  std::string name_;
  std::string expr_;
  uint64_t periodicity_ms_;
};

/**
 * Writer for metadata
 */
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

  /**
   * Writes the schema metadata
   *
   * @param schema The schema
   */
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

  /**
   * Writes metadata about an index
   *
   * @param name The name of the index
   * @param bucket_size The bucket_size used for lookup
   */
  void write_index_metadata(const std::string& name, double bucket_size) {
    if (id_ != storage::storage_mode::IN_MEMORY) {
      metadata_type type = metadata_type::D_INDEX_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, name);
      io_utils::write(out_, bucket_size);
      io_utils::flush(out_);
    }
  }

  /**
   * Writes the metadata for a specified filter
   *
   * @param name The name of the filter
   * @param expr The filter expression
   */
  void write_filter_metadata(const std::string& name, const std::string& expr) {
    if (id_ != storage::storage_mode::IN_MEMORY) {
      metadata_type type = metadata_type::D_FILTER_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, name);
      io_utils::write(out_, expr);
      io_utils::flush(out_);
    }
  }

  /**
   * Writes the metadata for aggregates
   *
   * @param name The name of the aggregate
   * @param filter_name The name of the filter
   * @param expr The filter expression
   */
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

  /**
   * Writes metadata for triggers
   *
   * @param trigger_name The name of the trigger
   * @param trigger_expr The trigger expression
   * @param periodicity_ms The periodicity of the trigger measured in
   * milliseconds
   */
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

/**
 * Reader metadata
 */
class metadata_reader {
 public:
  /**
   * Constructs an object to read metadata
   *
   * @param path The path of the file to read the metadata from
   */
  metadata_reader(const std::string& path)
      : filename_(path + "/metadata"),
        in_(filename_) {
  }

  /**
   * Reads the next metadata type
   *
   * @return The metadata type that was read
   */
  metadata_type next_type() {
    return io_utils::read<metadata_type>(in_);
  }

  /**
   * Reads the next schema
   *
   * @return The schema that was read
   */
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

  /**
   * Reads the next metadata for an index
   *
   * @return The index metadata that was read
   */
  index_metadata next_index_metadata() {
    std::string field_name = io_utils::read<std::string>(in_);
    double bucket_size = io_utils::read<double>(in_);
    return index_metadata(field_name, bucket_size);
  }

  /**
   * Reads the next metadata for a filter
   *
   * @return The filter metadata that was read
   */
  filter_metadata next_filter_metadata() {
    std::string filter_name = io_utils::read<std::string>(in_);
    std::string expr = io_utils::read<std::string>(in_);
    return filter_metadata(filter_name, expr);
  }

  /**
   * Reads the next metadata for an aggregate
   *
   * @return The aggregate metadata that was read
   */
  aggregate_metadata next_aggregate_metadata() {
    std::string name = io_utils::read<std::string>(in_);
    std::string filter_name = io_utils::read<std::string>(in_);
    std::string expr = io_utils::read<std::string>(in_);
    return aggregate_metadata(name, filter_name, expr);
  }

  /**
   * Reads the next metadata for a trigger
   *
   * @return The trigger metadata that was read
   */
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
