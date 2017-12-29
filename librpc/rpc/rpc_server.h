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

using boost::shared_ptr;

namespace confluo {
namespace rpc {

class rpc_service_handler : virtual public rpc_serviceIf {
 public:
  typedef std::map<rpc_iterator_id, lazy::stream<record_t>> adhoc_map;
  typedef std::map<rpc_iterator_id, lazy::stream<record_t>> predef_map;
  typedef std::map<rpc_iterator_id, lazy::stream<record_t>> combined_map;
  typedef std::map<rpc_iterator_id, lazy::stream<alert>> alerts_map;

  rpc_service_handler(confluo_store* store)
      : handler_id_(-1),
        store_(store),
        iterator_id_(0) {
  }

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

  void get_atomic_multilog_info(rpc_atomic_multilog_info& _return, const std::string& name) {
    _return.id = store_->get_atomic_multilog_id(name);
    auto dschema = store_->get_atomic_multilog(_return.id)->get_schema().columns();
    _return.schema = rpc_type_conversions::convert_schema(dschema);
  }

  void remove_atomic_multilog(int64_t id) {
    try {
      store_->remove_atomic_multilog(id);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  void add_index(int64_t id, const std::string& field_name, const double bucket_size) {
    try {
      store_->get_atomic_multilog(id)->add_index(field_name, bucket_size);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  void remove_index(int64_t id, const std::string& field_name) {
    try {
      store_->get_atomic_multilog(id)->remove_index(field_name);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

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

  void remove_filter(int64_t id, const std::string& filter_name) {
    try {
      store_->get_atomic_multilog(id)->remove_filter(filter_name);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

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

  void remove_aggregate(int64_t id, const std::string& aggregate_name) {
    try {
      store_->get_atomic_multilog(id)->remove_aggregate(aggregate_name);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

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

  void remove_trigger(int64_t id, const std::string& trigger_name) {
    try {
      store_->get_atomic_multilog(id)->remove_trigger(trigger_name);
    } catch(management_exception& ex) {
      rpc_management_exception e;
      e.msg = ex.what();
      throw e;
    }
  }

  int64_t append(int64_t id, const std::string& data) {
    void* buf = (char*) &data[0];  // XXX: Fix
    return store_->get_atomic_multilog(id)->append(buf);
  }

  int64_t append_batch(int64_t id, const rpc_record_batch& batch) {
    record_batch rbatch = rpc_type_conversions::convert_batch(batch);
    return store_->get_atomic_multilog(id)->append_batch(rbatch);
  }

  void read(std::string& _return, int64_t id, const int64_t offset,
      const int64_t nrecords) {
    uint64_t limit;
    ro_data_ptr ptr;
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    mlog->read(offset, limit, ptr);
    char* data = reinterpret_cast<char*>(ptr.get());
    size_t size = std::min(static_cast<size_t>(limit - offset),
        static_cast<size_t>(nrecords * mlog->record_size()));
    _return.assign(data, size);
  }

  void query_aggregate(std::string& _return, int64_t id,
      const std::string& aggregate_name,
      const int64_t begin_ms,
      const int64_t end_ms) {
    atomic_multilog* m = store_->get_atomic_multilog(id);
    _return = m->get_aggregate(aggregate_name, begin_ms, end_ms).to_string();
  }

  void adhoc_filter(rpc_iterator_handle& _return,int64_t id,
      const std::string& filter_expr) {
    bool success = false;
    rpc_iterator_id it_id = new_iterator_id();
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    try {
      auto res = mlog->execute_filter(filter_expr);
      auto ret = adhoc_.insert(std::make_pair(it_id, res));
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

  void predef_filter(rpc_iterator_handle& _return, int64_t id,
      const std::string& filter_name, const int64_t begin_ms,
      const int64_t end_ms) {
    rpc_iterator_id it_id = new_iterator_id();
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    auto res = mlog->query_filter(filter_name, begin_ms, end_ms);
    auto ret = predef_.insert(std::make_pair(it_id, res));
    if (!ret.second) {
      rpc_invalid_operation e;
      e.msg = "Duplicate rpc_iterator_id assigned";
      throw e;
    }

    predef_more(_return, mlog->record_size(), it_id);
  }

  void combined_filter(rpc_iterator_handle& _return, int64_t id,
      const std::string& filter_name,
      const std::string& filter_expr, const int64_t begin_ms,
      const int64_t end_ms) {
    bool success = false;
    rpc_iterator_id it_id = new_iterator_id();
    atomic_multilog* mlog = store_->get_atomic_multilog(id);
    try {
      auto res = mlog->query_filter(filter_name, begin_ms, end_ms, filter_expr);
      auto ret = combined_.insert(std::make_pair(it_id, res));
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

  void alerts_by_time(rpc_iterator_handle& _return, int64_t id,
      const int64_t begin_ms, const int64_t end_ms) {
    rpc_iterator_id it_id = new_iterator_id();
    auto alerts = store_->get_atomic_multilog(id)->get_alerts(begin_ms, end_ms);
    auto ret = alerts_.insert(std::make_pair(it_id, alerts));
    if (!ret.second) {
      rpc_invalid_operation e;
      e.msg = "Duplicate rpc_iterator_id assigned";
      throw e;
    }

    alerts_more(_return, it_id);
  }

  void alerts_by_trigger_and_time(rpc_iterator_handle& _return, int64_t id,
      const std::string& trigger_name, const int64_t begin_ms,
      const int64_t end_ms) {
    rpc_iterator_id it_id = new_iterator_id();
    auto alerts = store_->get_atomic_multilog(id)->get_alerts(begin_ms, end_ms, trigger_name);
    auto ret = alerts_.insert(std::make_pair(it_id, alerts));
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

  // TODO: get alerts by time and trigger name
  // TODO: subscribe to alerts
  // TODO: active alerts

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
      for (; !res.empty() && i < to_read; ++i, res = res.tail()) {
        record_t rec = res.head();
        _return.data.append(reinterpret_cast<const char*>(rec.data()), rec.length());
      }
      _return.num_entries = i;
      _return.has_more = !res.empty();
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
      for (; !res.empty() && i < to_read; ++i, res = res.tail()) {
        record_t rec = res.head();
        _return.data.append(reinterpret_cast<const char*>(rec.data()), rec.length());
      }
      _return.num_entries = i;
      _return.has_more = !res.empty();
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
      for (; !res.empty() && i < to_read; ++i, res = res.tail()) {
        record_t rec = res.head();
        _return.data.append(reinterpret_cast<const char*>(rec.data()), rec.length());
      }
      _return.num_entries = i;
      _return.has_more = !res.empty();
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
      for (; !res.empty() && i < to_read; ++i, res = res.tail()) {
        alert a = res.head();
        _return.data.append(a.to_string());
        _return.data.push_back('\n');
      }
      _return.num_entries = i;
      _return.has_more = !res.empty();
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

class rpc_clone_factory : public rpc_serviceIfFactory {
 public:
  rpc_clone_factory(confluo_store* store)
      : store_(store) {
  }

  virtual ~rpc_clone_factory() {
  }

  virtual rpc_serviceIf* getHandler(const TConnectionInfo& conn_info) {
    shared_ptr<TSocket> sock = boost::dynamic_pointer_cast<TSocket>(
        conn_info.transport);
    LOG_INFO<< "Incoming connection\n"
    << "\t\t\tSocketInfo: " << sock->getSocketInfo() << "\n"
    << "\t\t\tPeerHost: " << sock->getPeerHost() << "\n"
    << "\t\t\tPeerAddress: " << sock->getPeerAddress() << "\n"
    << "\t\t\tPeerPort: " << sock->getPeerPort();
    return new rpc_service_handler(store_);
  }

  virtual void releaseHandler(rpc_serviceIf* handler) {
    delete handler;
  }

 private:
  confluo_store* store_;
};

class rpc_server {
 public:
  static shared_ptr<TThreadedServer> create(confluo_store* store,
                                            const std::string& address,
                                            int port) {
    shared_ptr<rpc_clone_factory> clone_factory(new rpc_clone_factory(store));
    shared_ptr<rpc_serviceProcessorFactory> proc_factory(
        new rpc_serviceProcessorFactory(clone_factory));
    shared_ptr<TServerSocket> sock(new TServerSocket(address, port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TBinaryProtocolFactory> protocol_factory(
        new TBinaryProtocolFactory());
    shared_ptr<TThreadedServer> server(
        new TThreadedServer(proc_factory, sock, transport_factory,
                            protocol_factory));
    return server;
  }
};

}
}

#endif /* RPC_RPC_SERVER_H_ */
