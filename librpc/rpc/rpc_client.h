#ifndef RPC_RPC_CLIENT_H_
#define RPC_RPC_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/server/TServer.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "parser/schema_parser.h"

#include "rpc_service.h"
#include "rpc_configuration_params.h"
#include "rpc_types.h"
#include "rpc_type_conversions.h"
#include "rpc_record_batch_builder.h"
#include "rpc_record_stream.h"
#include "rpc_alert_stream.h"

#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

namespace confluo {
namespace rpc {

/**
 * RPC interface for the atomic multilog
 *
 */
class rpc_client {
 public:
  /** The thrift client type */
  typedef rpc_serviceClient thrift_client;

  /**
   * Constructs a new rpc client
   */
  rpc_client();

  /**
   * Constructs an rpc client for the given host and port
   *
   * @param host The host for the rpc client
   * @param port The port for the rpc client
   */
  rpc_client(const std::string &host, int port);

  /**
   * Destructs the rpc client
   */
  ~rpc_client();

  virtual /**
   * Disconnects the rpc client
   */
  void disconnect();

  /**
   * Connects the rpc client to the specified host and port
   *
   * @param host The host to connect to 
   * @param port The port to use
   */
  void connect(const std::string &host, int port);

  /**
   * Creates an atomic multilog with the given name, schema, and storage
   * mode
   *
   * @param name The name of the atomic multilog
   * @param schema The schema of the atomic multilog
   * @param mode The storage mode of the atomic multilog
   */
  void create_atomic_multilog(const std::string &name, const schema_t &schema, const storage::storage_mode mode);

  /**
   * Sets the current atomic multilog to the name of the desired atomic
   * multilog
   *
   * @param name The name of the atomic multilog to set the current atomic
   * multilog to
   */
  void set_current_atomic_multilog(const std::string &name);

  /**
   * Gets the current schema
   *
   * @return The current schema for the atomic multilog
   */
  schema_t const &current_schema() const;

  /**
   * Removes the atomic multilog associated with this client
   * @throw illegal_state_exception
   */
  void remove_atomic_multilog();

  /**
   * Adds an index to the particular field
   *
   * @param field_name The name of the field to add the index to 
   * @param bucket_size The size of the bucket for lookup
   * @throw illegal_state_exception
   */
  void add_index(const std::string &field_name, const double bucket_size = 1.0);

  /**
   * Removes an index associated with a field
   *
   * @param field_name The name of the field
   * @throw illegal_state_exception
   */
  void remove_index(const std::string &field_name);

  /**
   * Adds a filter to the client
   *
   * @param filter_name The name of the filter
   * @param filter_expr The filter expression
   * @throw illegal_state_exception
   */
  void add_filter(const std::string &filter_name, const std::string &filter_expr);

  /**
   * Removes a filter from the client
   *
   * @param filter_name The name of the filter to remove
   * @throw illegal_state_exception
   */
  void remove_filter(const std::string &filter_name);

  /**
   * Adds an aggregate to the rpc client
   *
   * @param aggregate_name The name of the aggregate
   * @param filter_name The name of the filter
   * @param aggregate_expr The aggregate expression
   * @throw illegal_state_exception
   */
  void add_aggregate(const std::string &aggregate_name,
                     const std::string &filter_name,
                     const std::string &aggregate_expr);

  /**
   * Removes an aggregate from the client
   *
   * @param aggregate_name The name of the aggregate to remove
   * @throw illegal_state_exception
   */
  void remove_aggregate(const std::string &aggregate_name);

  /**
   * Adds a trigger to the client
   *
   * @param trigger_name The name of the trigger
   * @param trigger_expr The trigger expression
   * @throw illegal_state_exception
   */
  void install_trigger(const std::string &trigger_name, const std::string &trigger_expr);

  /**
   * Removes a trigger from the client
   *
   * @param trigger_name The name of the trigger to remove
   */
  void remove_trigger(const std::string &trigger_name);

  /** Query ops **/
  // Write ops
  /**
   * Gets a batch builder for the current schema
   *
   * @return The record batch builder associated with the schema
   */
  rpc_record_batch_builder get_batch_builder() const;

  /**
   * Appends a batch to this client's record batch
   *
   * @param batch The batch of records to append
   */
  void append_batch(const rpc_record_batch &batch);

  /**
   * Appends a record to the client
   *
   * @param record The record to append
   */
  void append(const record_data &record);

  /**
   * Appends a json record to the client
   *
   * @param record The record to append
   */
  void append_json(const std::string &json_record);

  /**
   * Appends a vector of records to the client
   *
   * @param record The vector of records to append
   */
  void append(const std::vector<std::string> &record);

  // Read ops
  /**
   * Reads data from a specified offset
   *
   * @param _return The data that is read
   * @param offset The offset from the log to read from
   */
  void read(record_data &_return, int64_t offset);

  /**
   * Reads json data from a specified offset
   *
   * @param _return The data that is read
   * @param offset The offset from the log to read from
   */
  void read_json(std::string &_return, int64_t offset);

  /**
   * Reads data from the log at a specified offset
   *
   * @param offset The offset to read the log from
   *
   * @return A vector of records
   */
  std::vector<std::string> read(int64_t offset);

  /**
   * Reads the batch of data from an offset
   *
   * @param _return The data that is read
   * @param offset The offset from the log
   * @param nrecords The number of records to read
   */
  void read_batch(record_data &_return, int64_t offset, size_t nrecords);

  /**
   * Reads the batch of json data from an offset
   *
   * @param _return The data that is read
   * @param offset The offset from the log
   * @param nrecords The number of records to read
   */
  void read_batch_json(std::string &_return, int64_t offset, size_t nrecords);

  /**
   * Reads a batch from the specified offset
   *
   * @param offset The offset from the log
   * @param nrecords The number of records to read
   *
   * @return A vector containing the data read
   */
  std::vector<std::vector<std::string>> read_batch(int64_t offset, size_t nrecords);

  /**
   * Gets an aggregate from the client
   *
   * @param aggregate_name The name of the aggregate to get
   * @param begin_ms The beginning time 
   * @param end_ms The end time
   *
   * @return The aggregated statistic
   */
  std::string get_aggregate(const std::string &aggregate_name, int64_t begin_ms, int64_t end_ms);

  // TODO: Add tests
  /**
   * Executes an aggregate
   *
   * @param aggregate_expr The aggregate expression
   * @param filter_expr The filter expression
   *
   * @return A string containing the aggregate
   */
  std::string execute_aggregate(const std::string &aggregate_expr, const std::string &filter_expr);

  /**
   * Executes a filter on the client
   *
   * @param filter_expr The filter expression to execute
   *
   * @return The stream containing the results of the filter
   */
  rpc_record_stream execute_filter(const std::string &filter_expr);

  /**
   * Queries the filter for this client
   *
   * @param filter_name The name of the filter
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   *
   * @return Record stream containing the result of the query
   */
  rpc_record_stream query_filter(const std::string &filter_name, const int64_t begin_ms, const int64_t end_ms);

  /**
   * Queries the filter for this client
   *
   * @param filter_name The name of the filter
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @param additional_filter_expr The additional filter expression
   *
   * @return Record stream containing the result of the filter expression
   */
  rpc_record_stream query_filter(const std::string &filter_name,
                                 const int64_t begin_ms,
                                 const int64_t end_ms, const std::string &additional_filter_expr);

  /**
   * Gets the alerts between certain times
   *
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   *
   * @return Stream containing the alerts
   */
  rpc_alert_stream get_alerts(const int64_t begin_ms, const int64_t end_ms);

  /**
   * Gets the alerts from a specific trigger
   *
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @param trigger_name The name of the trigger
   *
   * @return Stream containing all of the alerts for the trigger
   */
  rpc_alert_stream get_alerts(const int64_t begin_ms, const int64_t end_ms, const std::string &trigger_name);

  /**
   * Gets the number of records in the client
   *
   * @return The number of records
   */
  int64_t num_records();

  /** Asynchronous calls for query ops **/
  // TODO: Add tests

  /**
   * Asynchronous call the append batch function
   * @throw illegal_state_exception
   *
   * @param batch The batch argument to the method
   */
  void send_append_batch(rpc_record_batch &batch);

  /**
   * Asynchronous receiving of the append batch
   *
   * @return The success of receiving the batch
   */
  int64_t recv_append_batch();

  /**
   * Asynchronous call the append function
   * @throw illegal_state_exception
   *
   * @param record The record argument to the method
   */
  void send_append(const record_data &record);

  /**
   * Asynchronous call the append function
   * @throw illegal_state_exception
   *
   * @param record The record argument to the method
   */
  void send_append(const std::vector<std::string> &record);

  /**
   * Asynchronous receiving of the append operation
   *
   * @return The success of receiving 
   */
  int64_t recv_append();

  /**
   * Asynchronous call the read batch function
   * @throw illegal_state_exception
   *
   * @param offset The offset argument to the method
   * @param nrecords The number of records argument to the method
   */
  void send_read_batch(int64_t offset, size_t nrecords);

  /**
   * Asynchronous call the read  function
   *
   * @param _return The records that are read
   * @param offset The offset to read from
   */
  void send_read(record_data &_return, int64_t offset);

  /**
   * Sends the read data method
   *
   * @param offset The offset to read from
   */
  void send_read(int64_t offset);

  /**
   * Receives the record batch data that is read
   *
   * @param data The data to be read
   */
  void recv_read_batch(record_data &data);

  /**
   * Receives the record  data that is read
   *
   * @param data The data to be read
   */
  void recv_read(record_data &data);

  /**
   * Receives the data that is read
   *
   * @return A vector containing the records that are read
   */
  std::vector<std::string> recv_read();

  /**
   * Receives the batch that is read
   *
   * @return A vector containing the batch of records that are read
   */
  std::vector<std::vector<std::string>> recv_read_batch();

  /**
   * Sends the aggregate from the client
   *
   * @param aggregate_name The name of the aggregate
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   */
  void send_get_aggregate(const std::string &aggregate_name, int64_t begin_ms, int64_t end_ms);

  /**
   * Receives an aggregate
   *
   * @return String containing the aggregate
   */
  std::string recv_get_aggregate();

  /**
   * Sends the aggregate from the client
   *
   * @param aggregate_expr The aggregate expression
   * @param filter_expr The filter expression
   * @throw illegal_state_exception
   */
  void send_execute_aggregate(const std::string &aggregate_expr, const std::string &filter_expr);

  /**
   * Receives the aggregate
   *
   * @return String containing the aggregate
   */
  std::string recv_execute_aggregate();

  /**
   * Sends the filter from the client
   *
   * @param filter_expr The filter expression
   * @throw illegal_state_exception
   */
  void send_execute_filter(const std::string &filter_expr);

  /**
   * Receives the records after executing the filter
   *
   * @return Stream containing the records as a result of the filter
   */
  rpc_record_stream recv_execute_filter();

  /**
   * Sends query filter from the client
   *
   * @param filter_name The name of the filter
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @throw illegal_state_exception
   */
  void send_query_filter(const std::string &filter_name, const int64_t begin_ms, const int64_t end_ms);

  /**
   * Receives the query filter
   *
   * @return Stream containing the records that filter returns
   */
  rpc_record_stream recv_query_filter();

  /**
   * Sends additional filter from the client
   *
   * @param filter_name The name of the filter
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @param additional_filter_expr The additional filter expression
   */
  void send_query_filter_additional_filter(const std::string &filter_name, const int64_t begin_ms, const int64_t end_ms,
                                           const std::string &additional_filter_expr);

  /**
   * Receives additional filter
   *
   * @return Records that filter returns
   */
  rpc_record_stream recv_query_filter_additional_filter();

  /**
   * Sends the alerts by the time from the client
   *
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @throw illegal_state_exception
   */
  void send_get_alerts(const int64_t begin_ms, const int64_t end_ms);

  /**
   * Receives the alerts
   *
   * @return Stream containing the alerts
   */
  rpc_alert_stream recv_get_alerts();

  /**
   * Sends the alerts by the trigger from the client
   *
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @param trigger_name The name of the trigger
   * @throw illegal_state_exception
   */
  void send_get_alerts_by_trigger(const int64_t begin_ms, const int64_t end_ms, const std::string &trigger_name);

  /**
   * Recieves the alerts by the trigger
   *
   * @return Stream containing the alerts
   */
  rpc_alert_stream recv_get_alerts_by_trigger();

  /**
   * Asynchronous call to send the number of records to the client
   */
  void send_num_records();

  /**
   * Asynchronous call to get the number of records
   *
   * @return The number of records in the atomic multilog
   */
  int64_t recv_num_records();

 protected:
  /** The multilog identifier for the client */
  int64_t cur_multilog_id_;
  /** The schema of the multilog */
  schema_t cur_schema_;

  /** The socket for the client to connect to */
  std::shared_ptr<TSocket> socket_;
  /** The client transport */
  std::shared_ptr<TTransport> transport_;
  /** The client protocol */
  std::shared_ptr<TProtocol> protocol_;
  /** The thrift client */
  std::shared_ptr<thrift_client> client_;
};

}
}

#endif /* RPC_RPC_CLIENT_H_ */
