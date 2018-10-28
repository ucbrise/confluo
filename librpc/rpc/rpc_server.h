#ifndef RPC_RPC_SERVER_H_
#define RPC_RPC_SERVER_H_

#include "rpc_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PlatformThreadFactory.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>

#include <thread>

#include "atomic_multilog.h"
#include "confluo_store.h"
#include "rpc_type_conversions.h"
#include "rpc_configuration_params.h"
#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

namespace confluo {
namespace rpc {

/**
 * Handler of RPC services
 */
class rpc_service_handler : virtual public rpc_serviceIf {
 public:
  /** The adhoc_map type */
  typedef std::map<rpc_iterator_id, std::unique_ptr<record_cursor>> adhoc_map;
  /** The adhoc entry type */
  typedef std::pair<rpc_iterator_id, std::unique_ptr<record_cursor>> adhoc_entry;
  /** The adhoc status type */
  typedef std::pair<adhoc_map::iterator, bool> adhoc_status;
  /** The map type */
  typedef std::map<rpc_iterator_id, std::unique_ptr<record_cursor>> predef_map;
  /** The entry type */
  typedef std::pair<rpc_iterator_id, std::unique_ptr<record_cursor>> predef_entry;
  /** The status type */
  typedef std::pair<predef_map::iterator, bool> predef_status;
  /** The combined map type */
  typedef std::map<rpc_iterator_id, std::unique_ptr<record_cursor>> combined_map;
  /** The combined map entry type */
  typedef std::pair<rpc_iterator_id, std::unique_ptr<record_cursor>> combined_entry;
  /** The combined status type */
  typedef std::pair<combined_map::iterator, bool> combined_status;
  /** The alerts map type */
  typedef std::map<rpc_iterator_id, std::unique_ptr<alert_cursor>> alerts_map;
  /** The alerts entry type */
  typedef std::pair<rpc_iterator_id, std::unique_ptr<alert_cursor>> alerts_entry;
  /** The alerts status type */
  typedef std::pair<alerts_map::iterator, bool> alerts_status;

  /**
   * Constructs an RPC service handler from a confluo store
   *
   * @param store The confluo store used to initialize the service
   * handler
   */
  rpc_service_handler(confluo_store *store);

  /**
   * Registers this service handler on a new thread
   * @throw management_expcetion If this service handler could not
   * be registered
   */
  void register_handler();

  /**
   * Deregisters this service handler and its associated thread
   */
  void deregister_handler();

  /**
   * Creates an atomic multilog from the given name, schema, and storage
   * mode
   *
   * @param name The name of the atomic multilog
   * @param schema The schema of the atomic multilog
   * @param mode The storage mode of the atomic multilog
   * @throw management_exception If the atomic multilog could not be
   * created
   * @return ID associated with the atomic multilog, -1 if it could
   * not be created
   */
  int64_t create_atomic_multilog(const std::string &name, const rpc_schema &schema, const rpc_storage_mode mode);

  /**
   * Gets information about the atomic multilog
   *
   * @param _return The info about the atomic multilog that is filled up
   * @param name The name of the atomic multilog
   */
  void get_atomic_multilog_info(rpc_atomic_multilog_info &_return, const std::string &name);

  /**
   * Removes the atomic multilog with the matching ID
   *
   * @param id The identifier of the atomic multilog
   * @throw management_excpetion If the atomic multilog could not be
   * removed
   */
  void remove_atomic_multilog(int64_t id);

  /**
   * Adds an index to a field in the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param field_name The name of the field to add the index to
   * @param bucket_size The size of the bucket
   * @throw managmeent_exception If the index could not be added
   */
  void add_index(int64_t id, const std::string &field_name, const double bucket_size);

  /**
   * Removes an index from a field in the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param field_name The name of the field in the atomic multilog
   * @throw management_exception If the index could not be removed
   */
  void remove_index(int64_t id, const std::string &field_name);

  /**
   * Adds a filter to the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param filter_name The name of the filter
   * @param filter_expr The filter expression
   * @throw managment_exception If the filter could not be added
   * @throw parse_exception If there was an error parsing the filter
   * expression
   */
  void add_filter(int64_t id, const std::string &filter_name,
                  const std::string &filter_expr);

  /**
   * Removes a filter from the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param filter_name The name of the filter
   * @throw managment_exception If the filter could not be removed
   */
  void remove_filter(int64_t id, const std::string &filter_name);

  /**
   * Adds an aggregate to the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param aggregate_name The name of the aggregate
   * @param filter_name The name of the filter
   * @param aggregate_expr The aggregate expression
   * @throw management_exception If the aggregate could not be added
   * @throw parse_exception If the filter expression could not be parsed
   */
  void add_aggregate(int64_t id,
                     const std::string &aggregate_name,
                     const std::string &filter_name,
                     const std::string &aggregate_expr);

  /**
   * Removes an aggregate from the atomic multilog
   *
   * @param id The identifier for the atomic multilog
   * @param aggregate_name The name of the aggregate
   * @throw management_exception If the aggregate could not be removed
   */
  void remove_aggregate(int64_t id, const std::string &aggregate_name);

  /**
   * Adds a trigger to the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param trigger_name The name of the trigger
   * @param trigger_expr The trigger expression
   * @throw management_exception If the trigger could not be added
   * @throw parse_exception If the trigger expression could not be
   * parsed
   */
  void add_trigger(int64_t id, const std::string &trigger_name, const std::string &trigger_expr);

  /**
   * Removes a trigger from the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param trigger_name The name of the trigger
   * @throw management_exception If the trigger could not be removed
   */
  void remove_trigger(int64_t id, const std::string &trigger_name);

  /**
   * Appends string data to the atomic multilog
   *
   * @param id The identifier for the atomic multilog
   * @param data The data to be added
   *
   * @return The offset to where the data is
   */
  int64_t append(int64_t id, const std::string &data);

  /**
   * Appends string json-formatted data to the atomic multilog
   *
   * @param id The identifier for the atomic multilog
   * @param json_data The data to be added
   *
   * @return The offset to where the data is
   */
  int64_t append_json(int64_t id, const std::string &json_data);

  /**
   * Appends a record batch to the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param batch The record batch to add to the atomic multilog
   *
   * @return The offset where the batch is located
   */
  int64_t append_batch(int64_t id, const rpc_record_batch &batch);

  /**
   * Reads n record strings from the atomic multilog
   *
   * @param _return The record string read
   * @param id The identifier of the atomic multilog
   * @param offset The offset to read from
   * @param nrecords The number of records to read
   */
  void read(std::string &_return, int64_t id, const int64_t offset, const int64_t nrecords);

  /**
   * Reads n json-formatted record strings from the atomic multilog
   *
   * @param _return The json-formatted record string read
   * @param id The identifier of the atomic multilog
   * @param offset The offset to read from
   * @param nrecords The number of records to read
   */
  void read_json(std::string &_return, int64_t id, const int64_t offset, const int64_t nrecords);

  /**
   * Queries an aggregate from the atomic multilog
   *
   * @param _return The string containing the aggregate
   * @param id The identifier of the atomic multilog
   * @param aggregate_name The name of the aggregate
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   */
  void query_aggregate(std::string &_return,
                       int64_t id,
                       const std::string &aggregate_name,
                       int64_t begin_ms,
                       int64_t end_ms);

  // TODO: Add tests
  /**
   * Sets the adhoc aggregate
   *
   * @param _return The return value containing the aggregate
   * @param id The identifier for the multilog
   * @param aggregate_expr The aggregate expression 
   * @param filter_expr The filter expression
   */
  void adhoc_aggregate(std::string &_return,
                       int64_t id,
                       const std::string &aggregate_expr,
                       const std::string &filter_expr);

  /**
   * Executes an ad hoc filter
   *
   * @param _return The result of the filter execution
   * @param id The identifier of the atomic multilog
   * @param filter_expr The filter expression
   */
  void adhoc_filter(rpc_iterator_handle &_return, int64_t id, const std::string &filter_expr);

  /**
   * Queries a predefined filter
   *
   * @param _return The return handler iterator
   * @param id The identifier of the atomic multilog
   * @param filter_name The name of the filter
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @throw rpc_invalid_exception If there was a duplicate rpc iterator
   */
  void predef_filter(rpc_iterator_handle &_return,
                     int64_t id,
                     const std::string &filter_name,
                     const int64_t begin_ms,
                     const int64_t end_ms);

  /**
   * Queries a combined filter
   *
   * @param _return The return handler iterator
   * @param id The identifier of the atomic multilog
   * @param filter_name The name of the filter
   * @param filter_expr The filter expression
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @throw rpc_invalid_exception If there was a duplicate rpc iterator
   */
  void combined_filter(rpc_iterator_handle &_return,
                       int64_t id,
                       const std::string &filter_name,
                       const std::string &filter_expr,
                       const int64_t begin_ms,
                       const int64_t end_ms);

  /**
   * Gets the alerts from a time range
   *
   * @param _return The rpc iterator handle containing the alerts
   * @param id The identifier of the atomic multilog
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @throw rpc_invalid_exception If there was a duplicate rpc iterator
   */
  void alerts_by_time(rpc_iterator_handle &_return, int64_t id, const int64_t begin_ms, const int64_t end_ms);

  /**
   * Gets the alerts from a time range and by trigger
   *
   * @param _return The rpc iterator handle containing the alerts
   * @param id The identifier of the atomic multilog
   * @param trigger_name The name of the trigger
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @throw rpc_invalid_exception If there was a duplicate rpc iterator
   */
  void alerts_by_trigger_and_time(rpc_iterator_handle &_return,
                                  int64_t id,
                                  const std::string &trigger_name,
                                  const int64_t begin_ms,
                                  const int64_t end_ms);

  /**
   * Gets more from the map
   *
   * @param _return The The iterator handle 
   * @param id The identifier
   * @param desc The iterator description
   */
  void get_more(rpc_iterator_handle &_return, int64_t id, const rpc_iterator_descriptor &desc);

  /**
   * Gets the number of records from the store
   *
   * @param id The identifier of the multilog
   *
   * @return The number of records
   */
  int64_t num_records(int64_t id);

 private:
  rpc_iterator_id new_iterator_id();

  void adhoc_more(rpc_iterator_handle &_return, size_t record_size, rpc_iterator_id it_id);

  void predef_more(rpc_iterator_handle &_return, size_t record_size, rpc_iterator_id it_id);

  void combined_more(rpc_iterator_handle &_return, size_t record_size, rpc_iterator_id it_id);

  void alerts_more(rpc_iterator_handle &_return, rpc_iterator_id it_id);

  rpc_handler_id handler_id_;
  confluo_store *store_;

  // Iterator management
  rpc_iterator_id iterator_id_;
  adhoc_map adhoc_;
  predef_map predef_;
  combined_map combined_;
  alerts_map alerts_;
};

/**
 * Factory for constructing an rpc service
 */
class rpc_clone_factory : public rpc_serviceIfFactory {
 public:
  /**
   * Constructs a rpc factory from the given confluo store
   *
   * @param store The confluo store for the rpc clone
   */
  rpc_clone_factory(confluo_store *store);

  /**
   * Destructs the rpc clone factory
   */
  virtual ~rpc_clone_factory();

  /**
   * Gets the service handler for the rpc connection
   *
   * @param conn_info The information that defines the connection
   *
   * @return An rpc service handler for the confluo store
   */
  virtual rpc_serviceIf *getHandler(const TConnectionInfo &conn_info);

  /**
   * Destructs the handler
   *
   * @param handler The handler to destruct
   */
  virtual void releaseHandler(rpc_serviceIf *handler);

 private:
  confluo_store *store_;
};

/**
 * The rpc server the client connects to
 */
class rpc_server {
 public:
  /**
   * Creates an rpc server from the given parameters
   *
   * @param store The confluo store 
   * @param address The address of the server
   * @param port The port the server is on
   *
   * @return A pointer to the server
   */
  static std::shared_ptr<TThreadedServer> create(confluo_store *store, const std::string &address, int port);
};

}
}

#endif /* RPC_RPC_SERVER_H_ */
