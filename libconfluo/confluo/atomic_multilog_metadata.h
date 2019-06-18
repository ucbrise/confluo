#ifndef CONFLUO_ATOMIC_MULTILOG_METADATA_H_
#define CONFLUO_ATOMIC_MULTILOG_METADATA_H_

#include <cstdint>
#include <string>
#include <fstream>

#include "archival/archival_mode.h"
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
      D_TRIGGER_METADATA = 4,
  /** Metadata for storage mode */
      D_STORAGE_MODE_METADATA = 5,
  /** Metadata for archival mode */
      D_ARCHIVAL_MODE_METADATA = 6
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
  index_metadata(const std::string &field_name, double bucket_size);

  /**
   * Gets the field name
   *
   * @return The field name for the metadata
   */
  std::string field_name() const;

  /**
   * Gets the bucket size
   *
   * @return The bucket size used for lookup
   */
  double bucket_size() const;

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
  filter_metadata(const std::string &filter_name, const std::string &expr);

  /**
   * Gets the filter name
   *
   * @return The name of the filter
   */
  const std::string &filter_name() const;

  /**
   * Gets the filter expression
   *
   * @return The filter expression
   */
  const std::string &expr() const;

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
  aggregate_metadata(const std::string &name, const std::string &filter_name,
                     const std::string &expr);

  /**
   * Gets the name of the aggregate
   *
   * @return The string containing the name of the aggregate
   */
  const std::string &aggregate_name() const;

  /**
   * Gets the name of the filter
   *
   * @return The filter name
   */
  const std::string &filter_name() const;

  /**
   * Gets the aggregate expression
   *
   * @return A string containing the expression for the aggregate
   */
  const std::string &aggregate_expression() const;

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
  trigger_metadata(const std::string &name, const std::string &expr,
                   uint64_t periodicity_ms);

  /**
   * Gets the name of the trigger
   *
   * @return The trigger name
   */
  const std::string &trigger_name() const;

  /**
   * Gets the expression for the trigger
   *
   * @return The trigger expression
   */
  const std::string &trigger_expression() const;

  /**
   * Gets the periodicity of the trigger
   *
   * @return The trigger periodicity in milliseconds
   */
  uint64_t periodicity_ms() const;

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
  typedef bool metadata_writer_state;
  static const metadata_writer_state UNINIT = false;
  static const metadata_writer_state INIT = true;

  /**
   * Default constructor
   */
  metadata_writer();

  /**
   * Constructor that initializes metadata writer
   * @param path The path of where the metadata is
   * @param overwrite Whether or not to overwrite previous contents of file
   */
  explicit metadata_writer(const std::string &path, bool overwrite = true);

  /**
   * Initializes the schema metadata writer
   *
   * @param schema The schema
   */
  metadata_writer(const metadata_writer &other);

  /**
   * Destructor
   */
  ~metadata_writer();

  /**
   * Assignment operator
   *
   * @param other A metadata writer instance
   * @return Updated metadata writer instance
   */
  metadata_writer &operator=(const metadata_writer &other);

  /**
   * Write the storage mode
   *
   * @param mode Storage mode to write
   */
  void write_storage_mode(storage::storage_mode mode);

  /**
   * Write the archival mode
   *
   * @param mode Archival mode to write
   */
  void write_archival_mode(archival::archival_mode mode);

  /**
   * Write the schema
   *
   * @param schema The schema to write
   */
  void write_schema(const schema_t &schema);

  /**
   * Writes metadata about an index
   *
   * @param name The name of the index
   * @param bucket_size The bucket_size used for lookup
   */
  void write_index_metadata(const std::string &name, double bucket_size);

  /**
   * Writes the metadata for a specified filter
   *
   * @param name The name of the filter
   * @param expr The filter expression
   */
  void write_filter_metadata(const std::string &name, const std::string &expr);

  /**
   * Writes the metadata for aggregates
   *
   * @param name The name of the aggregate
   * @param filter_name The name of the filter
   * @param expr The filter expression
   */
  void write_aggregate_metadata(const std::string &name, const std::string &filter_name, const std::string &expr);

  /**
   * Writes metadata for triggers
   *
   * @param trigger_name The name of the trigger
   * @param trigger_expr The trigger expression
   * @param periodicity_ms The periodicity of the trigger measured in
   * milliseconds
   */
  void write_trigger_metadata(const std::string &trigger_name,
                              const std::string &trigger_expr,
                              uint64_t periodicity_ms);
 private:
  std::string filename_;
  std::ofstream out_;
  metadata_writer_state state_;
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
  explicit metadata_reader(const std::string &path);

  /**
   * Checks if the reader has another element to read
   *
   * @return True if the reader has another element, otherwise false.
   */
  bool has_next();

  /**
   * Reads the next metadata type
   *
   * @return The metadata type that was read
   */
  metadata_type next_type();

  /**
   * Reads the next schema
   *
   * @return The schema that was read
   */
  schema_t next_schema();

  /**
   * Reads the next metadata for an index
   *
   * @return The index metadata that was read
   */
  index_metadata next_index_metadata();

  /**
   * Reads the next metadata for a filter
   *
   * @return The filter metadata that was read
   */
  filter_metadata next_filter_metadata();

  /**
   * Reads the next metadata for an aggregate
   *
   * @return The aggregate metadata that was read
   */
  aggregate_metadata next_aggregate_metadata();

  /**
   * Reads the next metadata for a trigger
   *
   * @return The trigger metadata that was read
   */
  trigger_metadata next_trigger_metadata();

  /**
   * Reads the next storage mode
   *
   * @return The next storage mode
   */
  storage::storage_mode next_storage_mode();

  /**
   * Reads the next archival mode
   *
   * @return The next archival mode
   */
  archival::archival_mode next_archival_mode();

 private:
  std::string filename_;
  std::ifstream in_;
};

}

#endif /* CONFLUO_ATOMIC_MULTILOG_METADATA_H_ */
