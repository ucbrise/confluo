#ifndef RPC_RPC_DIALOG_CLIENT_H_
#define RPC_RPC_DIALOG_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "dialog_service.h"
#include "dialog_types.h"
#include "rpc_type_conversions.h"
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
      : cur_table_id_(-1),
        read_buffer_(std::make_pair(INT64_C(-1), "")) {
  }

  rpc_dialog_client(const std::string& host, int port)
      : cur_table_id_(-1),
        read_buffer_(std::make_pair(INT64_C(-1), "")) {
    connect(host, port);
  }

  ~rpc_dialog_client() {
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
    cur_table_id_ = client_->create_table(table_name,
        rpc_type_conversions::convert_schema(schema.columns()),
        rpc_type_conversions::convert_mode(mode));
  }

  void set_current_table(const std::string& table_name) {
    rpc_table_info info;
    client_->get_table_info(info, table_name);
    cur_schema_ = schema_t(rpc_type_conversions::convert_schema(info.schema));
    cur_table_id_ = info.table_id;
  }

  void remove_table() {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    client_->remove_table(cur_table_id_);
    cur_table_id_ = -1;
  }

  void add_index(const std::string& field_name, const double bucket_size) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    client_->add_index(cur_table_id_, field_name, bucket_size);
  }

  void remove_index(const std::string& field_name) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    client_->remove_index(cur_table_id_, field_name);
  }

  void add_filter(const std::string& filter_name,
      const std::string& filter_expr) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    client_->add_filter(cur_table_id_, filter_name, filter_expr);
  }

  void remove_filter(const std::string& filter_name) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    client_->remove_filter(cur_table_id_, filter_name);
  }

  void add_trigger(const std::string& trigger_name,
      const std::string& filter_name,
      const std::string& trigger_expr) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    client_->add_trigger(cur_table_id_, trigger_name, filter_name, trigger_expr);
  }

  void remove_trigger(const std::string& trigger_name) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    client_->remove_trigger(cur_table_id_, trigger_name);
  }

  // Write ops
  void buffer(const std::string& record) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    if (record.length() != cur_schema_.record_size()) {
      throw illegal_state_exception("Record size incorrect");
    }
    builder_.add_record(record);
    if (builder_.num_records() >= rpc_configuration_params::WRITE_BATCH_SIZE) {
      client_->append_batch(cur_table_id_, builder_.get_batch());
    }
  }

  void write(const std::string& record) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    if (record.length() != cur_schema_.record_size()) {
      throw illegal_state_exception("Record size incorrect");
    }
    client_->append(cur_table_id_, record);
  }

  void flush() {
    if (builder_.num_records() > 0) {
      client_->append_batch(cur_table_id_, builder_.get_batch());
    }
  }

  /** Query ops **/
  // Read op
  void read(std::string& _return, int64_t offset) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    int64_t& buf_off = read_buffer_.first;
    std::string& buf = read_buffer_.second;
    int64_t rbuf_lim = buf_off + buf.size();
    if (buf_off == -1 || offset < buf_off || offset >= rbuf_lim) {
      read_buffer_.first = offset;
      client_->read(buf, cur_table_id_, buf_off, rpc_configuration_params::READ_BATCH_SIZE);
    }
    _return = buf.substr(offset - buf_off, cur_schema_.record_size());
  }

  rpc_record_stream adhoc_filter(const std::string& filter_expr) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    rpc_iterator_handle handle;
    client_->adhoc_filter(handle, cur_table_id_, filter_expr);
    return rpc_record_stream(cur_table_id_, cur_schema_, client_, std::move(handle));
  }

  rpc_record_stream predef_filter(const std::string& filter_name,
      const int64_t begin_ms,
      const int64_t end_ms) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    rpc_iterator_handle handle;
    client_->predef_filter(handle, cur_table_id_, filter_name, begin_ms, end_ms);
    return rpc_record_stream(cur_table_id_, cur_schema_, client_, std::move(handle));
  }

  rpc_record_stream combined_filter(const std::string& filter_name,
      const std::string& filter_expr,
      const int64_t begin_ms,
      const int64_t end_ms) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    rpc_iterator_handle handle;
    client_->combined_filter(handle, cur_table_id_, filter_name, filter_expr, begin_ms,
        end_ms);
    return rpc_record_stream(cur_table_id_, cur_schema_, client_, std::move(handle));
  }

  rpc_alert_stream get_alerts(const int64_t begin_ms, const int64_t end_ms) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    rpc_iterator_handle handle;
    client_->alerts_by_time(handle, cur_table_id_, begin_ms, end_ms);
    return rpc_alert_stream(cur_table_id_, client_, std::move(handle));
  }

  int64_t num_records() {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    return client_->num_records(cur_table_id_);
  }

protected:
  int64_t cur_table_id_;
  schema_t cur_schema_;

  // Write buffer
  rpc_record_batch_builder builder_;

  // Read buffer
  std::pair<int64_t, std::string> read_buffer_;

  shared_ptr<TSocket> socket_;
  shared_ptr<TTransport> transport_;
  shared_ptr<TProtocol> protocol_;
  shared_ptr<rpc_client> client_;
};

}
}

#endif /* RPC_RPC_DIALOG_CLIENT_H_ */
