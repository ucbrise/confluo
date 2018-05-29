#ifndef RPC_RPC_RECORD_STREAM_H_
#define RPC_RPC_RECORD_STREAM_H_

#include "schema/schema.h"
#include "rpc_service.h"

namespace confluo {
namespace rpc {

/**
 * Container for a stream of records
 */
class rpc_record_stream {
 public:
  /** The thrift client over rpc */
  typedef rpc_serviceClient thrift_client;

  /**
   * Constructs a record stream from the specified parameters
   *
   * @param multilog_id The identifier of the atomic multilog
   * @param schema The schema for the records
   * @param client The thrift client
   * @param handle The r value handle iterator
   */
  rpc_record_stream(int64_t multilog_id, const schema_t &schema,
                    std::shared_ptr<thrift_client> client,
                    rpc_iterator_handle &&handle);

  /**
   * Gets the record data from the schema
   *
   * @return The record at the offset
   */
  record_t get();

  /**
   * Advances the stream to get the next record
   *
   * @return This record stream advanced
   */
  rpc_record_stream &operator++();

  /**
   * Checks to see if the record stream has any more elements
   *
   * @return True if there are any elements left, false otherwise
   */
  bool has_more() const;

  /**
   * Checks to see if the record stream is empty
   *
   * @return True if the record stream is empty, false otherwise
   */
  bool empty() const;

 private:
  int64_t multilog_id_;
  schema_t schema_;
  rpc_iterator_handle handle_;
  size_t cur_off_;
  std::shared_ptr<thrift_client> client_;
};

}
}

#endif /* RPC_RPC_RECORD_STREAM_H_ */
