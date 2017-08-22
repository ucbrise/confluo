#ifndef RPC_RPC_DIALOG_CLIENT_H_
#define RPC_RPC_DIALOG_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "dialog_service.h"
#include "dialog_types.h"
#include "rpc_record_stream.h"
#include "rpc_record_batch_builder.h"
#include "rpc_alert_stream.h"

#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace dialog {
namespace rpc {

class rpc_dialog_client {
 public:
  typedef dialog_serviceClient rpc_client;

  rpc_dialog_client()
      : table_set_(false) {
  }

  rpc_dialog_client(const std::string& host, int port)
      : table_set_(false) {
  }

  ~rpc_dialog_client() {
    disconnect();
  }

  void disconnect() {
    if (transport_->isOpen()) {
      std::string host = socket_->getPeerHost();
      int port = socket_->getPeerPort();
      LOG_INFO<< "Disconnecting from " << host << ":" << port;
      transport_->close();
    }
  }

  void connect(const std::string& host, int port) {
    LOG_INFO<<"Connecting to " << host << ":" << port;
    socket_ = boost::shared_ptr<TSocket>(new TSocket(host, port));
    transport_ = boost::shared_ptr<TTransport>(new TBufferedTransport(socket_));
    protocol_ = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
    client_ = boost::shared_ptr<rpc_client>(new rpc_client(protocol_));
    transport_->open();
  }

  void set_current_table(const std::string& table_name) {
    rpc_schema schema;
    client_->set_current_table(schema, table_name);
    cur_schema_ = schema_t("", rpc_type_conversions::convert_schema(schema));
    table_set_ = true;
  }

protected:
  bool table_set_;
  schema_t cur_schema_;
  shared_ptr<TSocket> socket_;
  shared_ptr<TTransport> transport_;
  shared_ptr<TProtocol> protocol_;
  shared_ptr<rpc_client> client_;
};

}
}

#endif /* RPC_RPC_DIALOG_CLIENT_H_ */
