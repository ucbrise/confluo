#ifndef STREAMING_SS_CLIENT_H_
#define STREAMING_SS_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "server/stream_service.h"
#include "server/stream_service_types.h"
#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace streaming {

class ss_client {
 public:
  typedef stream_serviceClient client_t;
  ss_client(const std::string& hostname, const int port) {
    connect(hostname, port);
  }

  ss_client()
      : hostname_(""),
        port_(0) {
  }

  ~ss_client() {
    disconnect();
  }

  void connect(const std::string& hostname, const int port) {
    LOG_INFO<<"Connecting to " << hostname_ << ":" << port;
    hostname_ = hostname;
    port_ = port;

    socket_ = boost::shared_ptr<TSocket>(new TSocket(hostname, port));
    transport_ = boost::shared_ptr<TTransport>(new TBufferedTransport(socket_));
    protocol_ = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
    client_ = boost::shared_ptr<client_t>(new client_t(protocol_));
    transport_->open();
  }

  void disconnect() {
    transport_->close();
  }

protected:
  std::string hostname_;
  int port_;
  boost::shared_ptr<TSocket> socket_;
  boost::shared_ptr<TTransport> transport_;
  boost::shared_ptr<TProtocol> protocol_;
  boost::shared_ptr<client_t> client_;

};

}

#endif /* STREAMING_SS_CLIENT_H_ */
