#ifndef RPC_RPC_TYPE_CONVERSIONS_H_
#define RPC_RPC_TYPE_CONVERSIONS_H_

#include "schema/record_batch.h"
#include "schema/schema.h"
#include "storage/storage.h"
#include "rpc_types.h"


namespace confluo {
namespace rpc {
/**
 * Functionality for type conversions from rpc to confluo and vice versa
 */
class rpc_type_conversions {
 public:
  /**
   * Converts an rpc record batch to a record batch
   * @param rpc_batch The rpc record batch to convert
   * @return The resultant record batch
   */
  static record_batch convert_batch(const rpc_record_batch& rpc_batch);

  /**
   * Converts an rpc schema to a schema
   *
   * @param s The rpc schema for conversion
   *
   * @return Vector of columns defining the schema
   */
  static std::vector<column_t> convert_schema(const rpc_schema& s);

  /**
   * Converts a vector of columns defining the schema to an rpc schema
   *
   * @param s The native schema defined as a vector of columns
   *
   * @return The resultant rpc schema
   */
  static rpc_schema convert_schema(const std::vector<column_t>& s);

  /**
   * Converts an rpc storage mode to a storage mode
   *
   * @param mode The rpc storage mode
   *
   * @return The resultant storage mode
   */
  static storage::storage_mode convert_mode(const rpc_storage_mode& mode);

  /**
   * Converts a storage mode to an rpc storage mode
   *
   * @param mode The storage mode used for conversion
   *
   * @return The resultant rpc storage mode
   */
  static rpc_storage_mode convert_mode(const storage::storage_mode& mode);
};
}
}

#endif /* RPC_RPC_TYPE_CONVERSIONS_H_ */
