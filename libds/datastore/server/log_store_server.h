#ifndef DATASTORE_SERVER_LOG_STORE_SERVER_H_
#define DATASTORE_SERVER_LOG_STORE_SERVER_H_

#include <algorithm>
#include <numeric>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "server/log_store_service.h"

#include "logger.h"

#include "log_store.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace datastore {

template<typename storage, typename concurrency_control, typename aux_data>
class log_store_service : virtual public log_store_serviceIf {
 public:
  typedef log_store<storage, concurrency_control, aux_data> data_store;
  log_store_service(data_store& store)
      : store_(store) {
  }

  int64_t append(const std::string& data) {
    return store_.append(data);
  }

  void multi_append(std::vector<int64_t> & _return, const std::vector<std::string> & data) {
    uint64_t start_id = store_.append_batch(data);
    _return.resize(data.size());
    std::iota(_return.begin(), _return.end(), start_id);
  }

  void get(std::string& _return, const int64_t id) {
    char buf[UINT16_MAX];
    size_t length;
    bool success = store_.get(id, (uint8_t*) buf, length);
    if (success)
      _return.assign(buf, length);
  }

  bool update(const int64_t id, const std::string& data) {
    return store_.update(id, data);
  }

  bool invalidate(const int64_t id) {
    return store_.invalidate(id);
  }

  int64_t begin_snapshot() {
    return store_.begin_snapshot();
  }

  bool end_snapshot(const int64_t id) {
    return store_.end_snapshot(id);
  }

  int64_t num_records() {
    return store_.num_records();
  }

 private:
  data_store& store_;
};

template<typename storage, typename concurrency_control, typename aux_data>
class ls_processor_factory : public TProcessorFactory {
 public:
  typedef log_store<storage, concurrency_control, aux_data> data_store;
  typedef log_store_service<storage, concurrency_control, aux_data> data_store_service;

  ls_processor_factory(data_store& store)
      : store_(store) {
    LOG_INFO<< "Initializing processor factory...";
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    LOG_INFO << "Creating new processor...";
    boost::shared_ptr<data_store_service> handler(
        new data_store_service(store_));
    boost::shared_ptr<TProcessor> processor(
        new log_store_serviceProcessor(handler));
    return processor;
  }

private:
  data_store& store_;
};

class log_store_server {
 public:
  template<typename storage, typename concurrency_control, typename aux_data>
  static void start(log_store<storage, concurrency_control, aux_data>& store,
                    int port) {

    typedef ls_processor_factory<storage, concurrency_control, aux_data> data_store_processor_factory;
    try {
      shared_ptr<data_store_processor_factory> proc_factory(
          new data_store_processor_factory(store));
      shared_ptr<TServerSocket> socket(new TServerSocket(port));
      shared_ptr<TBufferedTransportFactory> transport_factory(
          new TBufferedTransportFactory());
      shared_ptr<TProtocolFactory> prot_factory(new TBinaryProtocolFactory());
      TThreadedServer server(proc_factory, socket, transport_factory,
                             prot_factory);

      LOG_INFO<< "Listening for connections on port " << port;
      server.run();
    } catch (std::exception& e) {
      LOG_ERROR<<"Could not start server listening on port " << port << ":" << e.what();
    }
  }
};

}

#endif /* DATASTORE_SERVER_LOG_STORE_SERVER_H_ */
