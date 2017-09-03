#ifndef RPC_DIALOG_CLIENT_H_
#define RPC_DIALOG_CLIENT_H_

#include "dialog_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PlatformThreadFactory.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>

#include "rpc_endpoint.h"
#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace dialog {
namespace rpc {
class dialog_client {
 public:
  dialog_client()
      : init_(false) {
  }

  dialog_client(const rpc_endpoint& ep) {
    connect(ep);
    init_ = true;
  }

  ~dialog_client() {
    if (init_ && transport_->isOpen()) {
      transport_->close();
    }
  }

  void connect(const rpc_endpoint& ep) {
    if (ep.is_valid()) {
      socket_ = boost::shared_ptr<TSocket>(new TSocket(ep.addr(), ep.port()));
      transport_ = boost::shared_ptr<TTransport>(
          new TBufferedTransport(socket_));
      protocol_ = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
      client_ = boost::shared_ptr<dialog_serviceClient>(
          new dialog_serviceClient(protocol_));
      transport_->open();
    }
  }

  dialog_serviceClient& operator->() {
    return *client_;
  }

  dialog_serviceClient const& operator->() const {
    return *client_;
  }

  dialog_serviceClient* operator()() {
    return client_.get();
  }

 private:
  bool init_;
  shared_ptr<TSocket> socket_;
  shared_ptr<TTransport> transport_;
  shared_ptr<TProtocol> protocol_;
  shared_ptr<dialog_serviceClient> client_;
};
}
}
#endif /* RPC_DIALOG_CLIENT_H_ */
