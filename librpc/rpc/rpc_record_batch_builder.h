#ifndef RPC_RPC_RECORD_BATCH_BUILDER_H_
#define RPC_RPC_RECORD_BATCH_BUILDER_H_

#include <map>
#include <vector>

#include "schema/schema.h"
#include "conf/configuration_params.h"
#include "rpc_types.h"

namespace confluo {
namespace rpc {

/**
 * Contains data from the record
 */
struct record_data : public std::string {
  /**
   * Constructs record data from the data pointer and size
   * @param data The data to initialize the record data to
   * @param size The size of the data
   */
  record_data(const void *data, size_t size);

  /**
   * Constructs an empty record data
   */
  record_data();
};

/**
 * Builder of record batches over RPC
 */
class rpc_record_batch_builder {
 public:
  /**
   * Constructs a record batch builder from a given schema
   * @param schema The schema to add record batches to
   */
  rpc_record_batch_builder(const schema_t &schema);

  /**
   * Adds a record to the record batch
   *
   * @param rec The record to add
   */
  void add_record(const record_data &rec);

  /**
   * Adds a vector of string records to the batch
   *
   * @param rec The vector of records to add
   */
  void add_record(const std::vector<std::string> &rec);

  /**
   * Gets the record batch with all of the records added
   *
   * @return The record batch
   */
  rpc_record_batch get_batch();

  /**
   * Clears the record batch to have no records
   */
  void clear();

  /**
   * Gets the number of records in the record batch
   *
   * @return The number of records
   */
  size_t num_records() const;

 private:
  size_t nrecords_;
  std::map<int64_t, size_t> batch_sizes_;
  std::map<int64_t, std::stringstream> batch_;
  const schema_t &schema_;
};

}
}

#endif /* RPC_RPC_RECORD_BATCH_BUILDER_H_ */
