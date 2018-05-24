#ifndef CONFLUO_SCHEMA_RECORD_BATCH_H_
#define CONFLUO_SCHEMA_RECORD_BATCH_H_

#include <map>
#include <vector>
#include <sstream>

#include "schema/schema.h"

namespace confluo {

/**
 * A block of records
 */
struct record_block {
  /** The time associated with the block */
  int64_t time_block;
  /** The data in the block */
  std::string data;
  /** The number of records in the block */
  size_t nrecords;
};

/**
 * Batch of records, maintains lookup of records
 */
struct record_batch {
  /** Vector of record blocks */
  std::vector<record_block> blocks;
  /** The number of records */
  size_t nrecords;

  /**
   * Gets the start time of the record block
   *
   * @return The start time of the record block
   */
  int64_t start_time_block() const;

  /**
   * Gets the end time of the record block
   *
   * @return The end time of the record block
   */
  int64_t end_time_block() const;
};

/**
 * Builder of record batches. Contains functionality to add records
 * for use in building a schema.
 */
class record_batch_builder {
 public:
  /** The time block */
  static const int64_t TIME_BLOCK = static_cast<const int64_t>(1e6);

  /**
   * Constructor for record batch builder
   * @param schema Schema for the atomic multilog
   */
  record_batch_builder(const schema_t& schema);

  /**
   * Adds record data to the batch
   * @param data The record data to be added
   */
  void add_record(const void* data);

  /**
   * Adds record to the batch
   * @param rec The record to be added
   */
  void add_record(const std::vector<std::string>& rec);

  /**
   * Gets the batch of records
   * @return The batch
   */
  record_batch get_batch();

 private:
  std::map<int64_t, size_t> batch_sizes_;
  std::map<int64_t, std::stringstream> batch_;
  const schema_t& schema_;
};

}

#endif /* CONFLUO_SCHEMA_RECORD_BATCH_H_ */
