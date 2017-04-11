#ifndef DATASTORE_SERVER_LOG_STORE_CLIENT_H_
#define DATASTORE_SERVER_LOG_STORE_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "server/log_store_service.h"

#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace datastore {

typedef log_store_serviceClient ls_client;

class log_store_client {
 public:
  log_store_client() = default;

  log_store_client(const std::string& host, int port) {
    connect(host, port);
  }

  log_store_client(const log_store_client& other) {
    socket_ = other.socket_;
    transport_ = other.transport_;
    protocol_ = other.protocol_;
    client_ = other.client_;
  }

  ~log_store_client() {
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

  void send_begin_snapshot() {
    client_->send_begin_snapshot();
  }

  int64_t recv_begin_snapshot() {
    return client_->recv_begin_snapshot();
  }

  void send_end_snapshot(const int64_t id) {
    client_->send_end_snapshot(id);
  }

  bool recv_end_snapshot() {
    return client_->recv_end_snapshot();
  }

 private:
  boost::shared_ptr<TSocket> socket_;
  boost::shared_ptr<TTransport> transport_;
  boost::shared_ptr<TProtocol> protocol_;
  boost::shared_ptr<log_store_serviceClient> client_;
};

}

#endif /* DATASTORE_SERVER_LOG_STORE_CLIENT_H_ */
