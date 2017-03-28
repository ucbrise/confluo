#ifndef DATASTORE_COORDINATOR_COORDINATOR_SERVER_H_
#define DATASTORE_COORDINATOR_COORDINATOR_SERVER_H_

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "coordinator/coordinator_service.h"

#include "logger.h"

#include "coordinator.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace datastore {

template<typename data_store>
class coordinator_service : virtual public coordinator_serviceIf {
 public:
  coordinator_service(coordinator<data_store>& coord)
      : coord_(coord) {
  }

  void get_snapshot(std::vector<int64_t> & _return) {
    snapshot s = coord_.get_snapshot();
    for (auto t : s.tails)
      _return.push_back(t);
  }

 private:
  coordinator<data_store>& coord_;
};

template<typename data_store>
class coord_processor_factory : public TProcessorFactory {
 public:
  coord_processor_factory(coordinator<data_store>& coord)
      : coord_(coord) {
    LOG_INFO<< "Initializing processor factory...";
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    LOG_INFO << "Creating new processor...";
    boost::shared_ptr<coordinator_service<data_store>> handler(
        new coordinator_service<data_store>(coord_));
    boost::shared_ptr<TProcessor> processor(
        new coordinator_serviceProcessor(handler));
    return processor;
  }

private:
  coordinator<data_store>& coord_;
};

class coordinator_server {
 public:
  template<typename data_store>
  static void start(coordinator<data_store>& coord, int port) {
    typedef coord_processor_factory<data_store> processor_factory;
    try {
      shared_ptr<processor_factory> proc_factory(new processor_factory(coord));
      shared_ptr<TServerSocket> transport(new TServerSocket(port));
      shared_ptr<TBufferedTransportFactory> transport_factory(
          new TBufferedTransportFactory());
      shared_ptr<TProtocolFactory> prot_factory(new TBinaryProtocolFactory());
      TThreadedServer server(proc_factory, transport, transport_factory,
                             prot_factory);

      LOG_INFO<< "Listening for connections on port " << port;
      server.run();
    } catch (std::exception& e) {
      LOG_ERROR<<"Could not start server listening on port " << port << ":" << e.what();
    }
  }
};

}

#endif /* DATASTORE_COORDINATOR_COORDINATOR_SERVER_H_ */
