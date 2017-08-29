#ifndef RPC_RPC_DIALOG_WRITER_H_
#define RPC_RPC_DIALOG_WRITER_H_

#include "rpc_dialog_client.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace dialog {
namespace rpc {

class rpc_dialog_writer : public rpc_dialog_client {
 public:
  typedef dialog_serviceClient rpc_client;

  rpc_dialog_writer()
      : rpc_dialog_client() {
  }

  rpc_dialog_writer(const std::string& host, int port)
      : rpc_dialog_client() {
    connect(host, port);
  }

  ~rpc_dialog_writer() {
    disconnect();
  }

  void disconnect() {
    if (transport_->isOpen()) {
      std::string host = socket_->getPeerHost();
      int port = socket_->getPeerPort();
      LOG_INFO<< "Disconnecting from " << host << ":" << port;
      client_->deregister_handler();
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
    client_->register_handler();
  }

  void create_table(const std::string& table_name, const schema_t& schema,
      const storage::storage_id mode) {
    cur_schema_ = schema;
    client_->create_table(table_name,
        rpc_type_conversions::convert_schema(schema.columns()),
        rpc_type_conversions::convert_mode(mode));
    table_set_ = true;
  }

  void add_index(const std::string& field_name, const double bucket_size) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    client_->add_index(field_name, bucket_size);
  }

  void remove_index(const std::string& field_name) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    client_->remove_index(field_name);
  }

  void add_filter(const std::string& filter_name,
      const std::string& filter_expr) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    client_->add_filter(filter_name, filter_expr);
  }

  void remove_filter(const std::string& filter_name) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    client_->remove_filter(filter_name);
  }

  void add_trigger(const std::string& trigger_name,
      const std::string& filter_name,
      const std::string& trigger_expr) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    client_->add_trigger(trigger_name, filter_name, trigger_expr);
  }

  void remove_trigger(const std::string& trigger_name) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    client_->remove_trigger(trigger_name);
  }

  // Write ops
  void buffer(const std::string& record) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    if (record.length() != cur_schema_.record_size()) {
      throw illegal_state_exception("Record size incorrect");
    }
    builder_.add_record(record);
    if (builder_.num_records() >= rpc_configuration_params::WRITE_BATCH_SIZE) {
      client_->append_batch(builder_.get_batch());
    }
  }

  void write(const std::string& record) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    if (record.length() != cur_schema_.record_size()) {
      throw illegal_state_exception("Record size incorrect");
    }
    client_->append(record);
  }

  void flush() {
    if (builder_.num_records() > 0) {
      client_->append_batch(builder_.get_batch());
    }
  }

private:
  rpc_record_batch_builder builder_;
};

}
}

#endif /* RPC_RPC_DIALOG_WRITER_H_ */
