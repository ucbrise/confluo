#ifndef DATASTORE_SERVER_LOG_STORE_CLIENT_H_
#define DATASTORE_SERVER_LOG_STORE_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include "server/dialog_service.h"
#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace dialog {

typedef dialog_serviceClient ls_client;

class dialog_client {
 public:
  dialog_client() = default;

  dialog_client(const std::string& host, int port) {
    connect(host, port);
  }

  dialog_client(const dialog_client& other) {
    socket_ = other.socket_;
    transport_ = other.transport_;
    protocol_ = other.protocol_;
    client_ = other.client_;
  }

  ~dialog_client() {
    disconnect();
  }

  void connect(const std::string& host, int port) {
    LOG_INFO << "Connecting to " << host << ":" << port;
    socket_ = boost::shared_ptr<TSocket>(new TSocket(host, port));
    transport_ = boost::shared_ptr<TTransport>(new TBufferedTransport(socket_));
    protocol_ = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
    client_ = boost::shared_ptr<ls_client>(new ls_client(protocol_));
    transport_->open();
  }

  void disconnect() {
    transport_->close();
  }

  int64_t append(const std::string& data) {
    return client_->append(data);
  }

  void send_append(const std::string& data) {
    return client_->send_append(data);
  }

  int64_t recv_append() {
    return client_->recv_append();
  }

  void multi_append(std::vector<int64_t>& ids, const std::vector<std::string>& data) {
    return client_->multi_append(ids, data);
  }

  void get(std::string& _return, const int64_t id, const int64_t len) {
    return client_->get(_return, id, len);
  }

  bool update(const int64_t id, const std::string& data) {
    return client_->update(id, data);
  }

  bool invalidate(const int64_t id) {
    return client_->invalidate(id);
  }

 private:
  boost::shared_ptr<TSocket> socket_;
  boost::shared_ptr<TTransport> transport_;
  boost::shared_ptr<TProtocol> protocol_;
  boost::shared_ptr<ls_client> client_;
};

}

#endif /* DATASTORE_SERVER_LOG_STORE_CLIENT_H_ */
