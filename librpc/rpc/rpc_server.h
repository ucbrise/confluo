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
  typedef std::map<rpc_iterator_id, std::unique_ptr<record_cursor>> adhoc_map;
  typedef std::pair<rpc_iterator_id, std::unique_ptr<record_cursor>> adhoc_entry;
  typedef std::pair<adhoc_map::iterator, bool> adhoc_status;
  typedef std::map<rpc_iterator_id, std::unique_ptr<record_cursor>> predef_map;
  typedef std::pair<rpc_iterator_id, std::unique_ptr<record_cursor>> predef_entry;
  typedef std::pair<predef_map::iterator, bool> predef_status;
  typedef std::map<rpc_iterator_id, std::unique_ptr<record_cursor>> combined_map;
  typedef std::pair<rpc_iterator_id, std::unique_ptr<record_cursor>> combined_entry;
  typedef std::pair<combined_map::iterator, bool> combined_status;
  typedef std::map<rpc_iterator_id, std::unique_ptr<alert_cursor>> alerts_map;
  typedef std::pair<rpc_iterator_id, std::unique_ptr<alert_cursor>> alerts_entry;
  typedef std::pair<alerts_map::iterator, bool> alerts_status;

  /**
   * Constructs an RPC service handler from a confluo store
   *
   * @param store The confluo store used to initialize the service
   * handler
   */
  rpc_service_handler(confluo_store* store)
      : handler_id_(-1),
        store_(store),
        iterator_id_(0) {
  }

  /**
   * Registers this service handler on a new thread
   * @throw management_expcetion If this service handler could not
   * be registered
   */
  void register_handler() {
    handler_id_ = thread_manager::register_thread();
    if (handler_id_ < 0) {
      rpc_management_exception ex;
      ex.msg = "Could not register handler";
      throw ex;
    } else {
      LOG_INFO<< "Registered handler thread " << std::this_thread::get_id() << " as " << handler_id_;
    }
  }

  /**
   * Deregisters this service handler and its associated thread
   */
  void deregister_handler() {
    int ret = thread_manager::deregister_thread();
    if (ret < 0) {
      rpc_management_exception ex;
      ex.msg = "Could not deregister handler";
      throw ex;
    } else {
      LOG_INFO << "Deregistered handler thread " << std::this_thread::get_id()
      << " as " << ret;
    }
  }

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
  int64_t create_atomic_multilog(const std::string& name,
      const rpc_schema& schema,
      const rpc_storage_mode mode) {
    int64_t ret = -1;
    try {
      ret = store_->create_atomic_multilog(name,
          rpc_type_conversions::convert_schema(schema),
          rpc_type_conversions::convert_mode(mode));
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
    return ret;
  }

  /**
   * Gets information about the atomic multilog
   *
   * @param _return The info about the atomic multilog that is filled up
   * @param name The name of the atomic multilog
   */
  void get_atomic_multilog_info(rpc_atomic_multilog_info& _return, const std::string& name) {
    _return.id = store_->get_atomic_multilog_id(name);
    auto dschema = store_->get_atomic_multilog(_return.id)->get_schema().columns();
    _return.schema = rpc_type_conversions::convert_schema(dschema);
  }

  /**
   * Removes the atomic multilog with the matching ID
   *
   * @param id The identifier of the atomic multilog
   * @throw management_excpetion If the atomic multilog could not be
   * removed
   */
  void remove_atomic_multilog(int64_t id) {
    try {
      store_->remove_atomic_multilog(id);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  /**
   * Adds an index to a field in the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param field_name The name of the field to add the index to
   * @param bucket_size The size of the bucket
   * @throw managmeent_exception If the index could not be added
   */
  void add_index(int64_t id, const std::string& field_name, const double bucket_size) {
    try {
      store_->get_atomic_multilog(id)->add_index(field_name, bucket_size);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  /**
   * Removes an index from a field in the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param field_name The name of the field in the atomic multilog
   * @throw management_exception If the index could not be removed
   */
  void remove_index(int64_t id, const std::string& field_name) {
    try {
      store_->get_atomic_multilog(id)->remove_index(field_name);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

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
  void add_filter(int64_t id, const std::string& filter_name,
      const std::string& filter_expr) {
    try {
      store_->get_atomic_multilog(id)->add_filter(filter_name, filter_expr);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    } catch(parse_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  /**
   * Removes a filter from the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param filter_name The name of the filter
   * @throw managment_exception If the filter could not be removed
   */
  void remove_filter(int64_t id, const std::string& filter_name) {
    try {
      store_->get_atomic_multilog(id)->remove_filter(filter_name);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

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
  void add_aggregate(int64_t id, const std::string& aggregate_name,
      const std::string& filter_name,
      const std::string& aggregate_expr) {
    try {
      store_->get_atomic_multilog(id)->add_aggregate(aggregate_name,
          filter_name,
          aggregate_expr);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    } catch(parse_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  /**
   * Removes an aggregate from the atomic multilog
   *
   * @param id The identifier for the atomic multilog
   * @param aggregate_name The name of the aggregate
   * @throw management_exception If the aggregate could not be removed
   */
  void remove_aggregate(int64_t id, const std::string& aggregate_name) {
    try {
      store_->get_atomic_multilog(id)->remove_aggregate(aggregate_name);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

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
  void add_trigger(int64_t id, const std::string& trigger_name,
      const std::string& trigger_expr) {
    try {
      store_->get_atomic_multilog(id)->install_trigger(trigger_name,
          trigger_expr);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    } catch(parse_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  /**
   * Removes a trigger from the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param trigger_name The name of the trigger
   * @throw management_exception If the trigger could not be removed
   */
  void remove_trigger(int64_t id, const std::string& trigger_name) {
    try {
      store_->get_atomic_multilog(id)->remove_trigger(trigger_name);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  /**
   * Appends string data to the atomic multilog
   *
   * @param id The identifier for the atomic multilog
   * @param data The data to be added
   *
   * @return The offset to where the data is
   */
  int64_t append(int64_t id, const std::string& data) {
    void* buf = (char*) &data[0];  // XXX: Fix
    return store_->get_atomic_multilog(id)->append(buf);
  }

  /**
   * Appends a record batch to the atomic multilog
   *
   * @param id The identifier of the atomic multilog
   * @param batch The record batch to add to the atomic multilog
   *
   * @return The offset where the batch is located
   */
  int64_t append_batch(int64_t id, const rpc_record_batch& batch) {
    record_batch rbatch = rpc_type_conversions::convert_batch(batch);
    return store_->get_atomic_multilog(id)->append_batch(rbatch);
  }

  /**
   * Reads a record string from the atomic multilog
   *
   * @param _return The record string read
   * @param id The identifier of the atomic multilog
   * @param offset The offset to read from
   * @param nrecords The number of records to read
   */
    void read(std::string& _return, int64_t id, const int64_t offset, const int64_t nrecords) {
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    uint64_t limit;
    storage::read_only_encoded_ptr<uint8_t> ptr;
    mlog->read(offset, limit, ptr);
    auto decoded_ptr = ptr.decode();
    char* data = reinterpret_cast<char*>(decoded_ptr.get());
    size_t size = std::min(static_cast<size_t>(limit - offset),
                           static_cast<size_t>(nrecords * mlog->record_size()));
    _return.assign(data, size);
  }

  /**
   * Queries an aggregate from the atomic multilog
   *
   * @param _return The string containing the aggregate
   * @param id The identifier of the atomic multilog
   * @param aggregate_name The name of the aggregate
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   */
  void query_aggregate(std::string& _return, int64_t id,
      const std::string& aggregate_name,
      const int64_t begin_ms,
      const int64_t end_ms) {
    atomic_multilog* m = store_->get_atomic_multilog(id);
    _return = m->get_aggregate(aggregate_name, begin_ms, end_ms).to_string();
  }

  // TODO: Add tests
  void adhoc_aggregate(std::string& _return, int64_t id,
      const std::string& aggregate_expr,
      const std::string& filter_expr) {
    atomic_multilog* m = store_->get_atomic_multilog(id);
    _return = m->execute_aggregate(aggregate_expr, filter_expr).to_string();
  }

  /**
   * Executes an ad hoc filter
   *
   * @param _return The result of the filter execution
   * @param id The identifier of the atomic multilog
   * @param filter_expr The filter expression
   */
  void adhoc_filter(rpc_iterator_handle& _return,int64_t id,
      const std::string& filter_expr) {
    bool success = false;
    rpc_iterator_id it_id = new_iterator_id();
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    try {
      adhoc_entry entry(it_id, mlog->execute_filter(filter_expr));
      adhoc_status ret = adhoc_.insert(std::move(entry));
      success = ret.second;
    } catch (parse_exception& ex) {
      rpc_invalid_operation e;
      e.msg = ex.what();
      throw e;
    }

    if (!success) {
      rpc_invalid_operation e;
      e.msg = "Duplicate rpc_iterator_id assigned";
      throw e;
    }

    adhoc_more(_return, mlog->record_size(), it_id);
  }

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
  void predef_filter(rpc_iterator_handle& _return, int64_t id,
      const std::string& filter_name, const int64_t begin_ms,
      const int64_t end_ms) {
    rpc_iterator_id it_id = new_iterator_id();
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    predef_entry entry(it_id, mlog->query_filter(filter_name, begin_ms, end_ms));
    predef_status ret = predef_.insert(std::move(entry));
    if (!ret.second) {
      rpc_invalid_operation e;
      e.msg = "Duplicate rpc_iterator_id assigned";
      throw e;
    }

    predef_more(_return, mlog->record_size(), it_id);
  }

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
  void combined_filter(rpc_iterator_handle& _return, int64_t id,
      const std::string& filter_name,
      const std::string& filter_expr, const int64_t begin_ms,
      const int64_t end_ms) {
    bool success = false;
    rpc_iterator_id it_id = new_iterator_id();
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    try {
      combined_entry entry(it_id, mlog->query_filter(filter_name,
              begin_ms, end_ms, filter_expr));
      combined_status ret = combined_.insert(std::move(entry));
      success = ret.second;
    } catch (parse_exception& ex) {
      rpc_invalid_operation e;
      e.msg = ex.what();
      throw e;
    }
    if (!success) {
      rpc_invalid_operation e;
      e.msg = "Duplicate rpc_iterator_id assigned";
      throw e;
    }

    combined_more(_return, mlog->record_size(), it_id);
  }

  /**
   * Gets the alerts from a time range
   *
   * @param _return The rpc iterator handle containing the alerts
   * @param id The identifier of the atomic multilog
   * @param begin_ms The beginning time in milliseconds
   * @param end_ms The end time in milliseconds
   * @throw rpc_invalid_exception If there was a duplicate rpc iterator
   */
  void alerts_by_time(rpc_iterator_handle& _return, int64_t id,
      const int64_t begin_ms, const int64_t end_ms) {
    rpc_iterator_id it_id = new_iterator_id();
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    alerts_entry entry(it_id, mlog->get_alerts(begin_ms, end_ms));
    alerts_status ret = alerts_.insert(std::move(entry));
    if (!ret.second) {
      rpc_invalid_operation e;
      e.msg = "Duplicate rpc_iterator_id assigned";
      throw e;
    }

    alerts_more(_return, it_id);
  }

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
  void alerts_by_trigger_and_time(rpc_iterator_handle& _return, int64_t id,
      const std::string& trigger_name, const int64_t begin_ms,
      const int64_t end_ms) {
    rpc_iterator_id it_id = new_iterator_id();
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    alerts_entry entry(it_id, mlog->get_alerts(begin_ms, end_ms,
            trigger_name));
    alerts_status ret = alerts_.insert(std::move(entry));
    if (!ret.second) {
      rpc_invalid_operation e;
      e.msg = "Duplicate rpc_iterator_id assigned";
      throw e;
    }

    alerts_more(_return, it_id);
  }

  void get_more(rpc_iterator_handle& _return, int64_t id,
      const rpc_iterator_descriptor& desc) {
    if (desc.handler_id != handler_id_) {
      rpc_invalid_operation ex;
      ex.msg = "handler_id mismatch";
      throw ex;
    }

    size_t record_size = store_->get_atomic_multilog(id)->record_size();

    switch (desc.type) {
      case rpc_iterator_type::RPC_ADHOC: {
        adhoc_more(_return, record_size, desc.id);
        break;
      }
      case rpc_iterator_type::RPC_PREDEF: {
        predef_more(_return, record_size, desc.id);
        break;
      }
      case rpc_iterator_type::RPC_COMBINED: {
        combined_more(_return, record_size, desc.id);
        break;
      }
      case rpc_iterator_type::RPC_ALERTS: {
        alerts_more(_return, desc.id);
        break;
      }
    }
  }

  int64_t num_records(int64_t id) {
    return store_->get_atomic_multilog(id)->num_records();
  }

private:
  rpc_iterator_id new_iterator_id() {
    return iterator_id_++;
  }

  void adhoc_more(rpc_iterator_handle& _return, size_t record_size,
      rpc_iterator_id it_id) {
    // Initialize iterator descriptor
    _return.desc.data_type = rpc_data_type::RPC_RECORD;
    _return.desc.handler_id = handler_id_;
    _return.desc.id = it_id;
    _return.desc.type = rpc_iterator_type::RPC_ADHOC;

    // Read data from iterator
    try {
      auto& res = adhoc_.at(it_id);
      size_t to_read = rpc_configuration_params::ITERATOR_BATCH_SIZE;
      _return.data.reserve(record_size * to_read);
      size_t i = 0;
      for (; res->has_more() && i < to_read; ++i, res->advance()) {
        record_t rec = res->get();
        _return.data.append(reinterpret_cast<const char*>(rec.data()), rec.length());
      }
      _return.num_entries = i;
      _return.has_more = res->has_more();
    } catch (std::out_of_range& ex) {
      rpc_invalid_operation e;
      e.msg = "No such iterator";
      throw e;
    }
  }

  void predef_more(rpc_iterator_handle& _return, size_t record_size,
      rpc_iterator_id it_id) {
    // Initialize iterator descriptor
    _return.desc.data_type = rpc_data_type::RPC_RECORD;
    _return.desc.handler_id = handler_id_;
    _return.desc.id = it_id;
    _return.desc.type = rpc_iterator_type::RPC_PREDEF;

    // Read data from iterator
    try {
      auto& res = predef_.at(it_id);
      size_t to_read = rpc_configuration_params::ITERATOR_BATCH_SIZE;
      _return.data.reserve(record_size * to_read);
      size_t i = 0;
      for (; res->has_more() && i < to_read; ++i, res->advance()) {
        record_t rec = res->get();
        _return.data.append(reinterpret_cast<const char*>(rec.data()), rec.length());
      }
      _return.num_entries = i;
      _return.has_more = res->has_more();
    } catch (std::out_of_range& ex) {
      rpc_invalid_operation e;
      e.msg = "No such iterator";
      throw e;
    }
  }

  void combined_more(rpc_iterator_handle& _return, size_t record_size,
      rpc_iterator_id it_id) {
    // Initialize iterator descriptor
    _return.desc.data_type = rpc_data_type::RPC_RECORD;
    _return.desc.handler_id = handler_id_;
    _return.desc.id = it_id;
    _return.desc.type = rpc_iterator_type::RPC_COMBINED;

    // Read data from iterator
    try {
      auto& res = combined_.at(it_id);
      size_t to_read = rpc_configuration_params::ITERATOR_BATCH_SIZE;
      _return.data.reserve(record_size * to_read);
      size_t i = 0;
      for (; res->has_more() && i < to_read; ++i, res->advance()) {
        record_t rec = res->get();
        _return.data.append(reinterpret_cast<const char*>(rec.data()), rec.length());
      }
      _return.num_entries = i;
      _return.has_more = res->has_more();
    } catch (std::out_of_range& ex) {
      rpc_invalid_operation e;
      e.msg = "No such iterator";
      throw e;
    }
  }

  void alerts_more(rpc_iterator_handle& _return, rpc_iterator_id it_id) {
    // Initialize iterator descriptor
    _return.desc.data_type = rpc_data_type::RPC_ALERT;
    _return.desc.handler_id = handler_id_;
    _return.desc.id = it_id;
    _return.desc.type = rpc_iterator_type::RPC_ALERTS;

    // Read data from iterator
    try {
      auto& res = alerts_.at(it_id);
      size_t to_read = rpc_configuration_params::ITERATOR_BATCH_SIZE;
      size_t i = 0;
      for (; res->has_more() && i < to_read; ++i, res->advance()) {
        alert a = res->get();
        _return.data.append(a.to_string());
        _return.data.push_back('\n');
      }
      _return.num_entries = i;
      _return.has_more = res->has_more();
    } catch (std::out_of_range& ex) {
      rpc_invalid_operation e;
      e.msg = "No such iterator";
      throw e;
    }
  }

  rpc_handler_id handler_id_;
  confluo_store* store_;

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
  rpc_clone_factory(confluo_store* store)
      : store_(store) {
  }

  /**
   * Destructs the rpc clone factory
   */
  virtual ~rpc_clone_factory() {
  }

  /**
   * Gets the service handler for the rpc connection
   *
   * @param conn_info The information that defines the connection
   *
   * @return An rpc service handler for the confluo store
   */
  virtual rpc_serviceIf* getHandler(const TConnectionInfo& conn_info) {
    std::shared_ptr<TSocket> sock = std::dynamic_pointer_cast<TSocket>(
        conn_info.transport);
    LOG_INFO<< "Incoming connection\n"
    << "\t\t\tSocketInfo: " << sock->getSocketInfo() << "\n"
    << "\t\t\tPeerHost: " << sock->getPeerHost() << "\n"
    << "\t\t\tPeerAddress: " << sock->getPeerAddress() << "\n"
    << "\t\t\tPeerPort: " << sock->getPeerPort();
    return new rpc_service_handler(store_);
  }

  /**
   * Destructs the handler
   *
   * @param handler The handler to destruct
   */
  virtual void releaseHandler(rpc_serviceIf* handler) {
    delete handler;
  }

 private:
  confluo_store* store_;
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
  static std::shared_ptr<TThreadedServer> create(confluo_store* store,
                                                 const std::string& address,
                                                 int port) {
    std::shared_ptr<rpc_clone_factory> clone_factory(
        new rpc_clone_factory(store));
    std::shared_ptr<rpc_serviceProcessorFactory> proc_factory(
        new rpc_serviceProcessorFactory(clone_factory));
    std::shared_ptr<TServerSocket> sock(new TServerSocket(address, port));
    std::shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    std::shared_ptr<TBinaryProtocolFactory> protocol_factory(
        new TBinaryProtocolFactory());
    std::shared_ptr<TThreadedServer> server(
        new TThreadedServer(proc_factory, sock, transport_factory,
                            protocol_factory));
    return server;
  }
};

}
}

#endif /* RPC_RPC_SERVER_H_ */
