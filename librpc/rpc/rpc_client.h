#ifndef RPC_RPC_CLIENT_H_
#define RPC_RPC_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/server/TServer.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "rpc_service.h"
#include "rpc_configuration_params.h"
#include "rpc_types.h"
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

namespace confluo {
namespace rpc {

class rpc_client {
 public:
  typedef rpc_serviceClient thrift_client;

  rpc_client()
      : cur_multilog_id_(-1),
        read_buffer_(std::make_pair(INT64_C(-1), "")) {
  }

  rpc_client(const std::string& host, int port)
      : cur_multilog_id_(-1),
        read_buffer_(std::make_pair(INT64_C(-1), "")) {
    connect(host, port);
  }

  ~rpc_client() {
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
    client_ = boost::shared_ptr<thrift_client>(new thrift_client(protocol_));
    transport_->open();
    client_->register_handler();
  }

  void create_atomic_multilog(const std::string& name, const schema_t& schema,
      const storage::storage_id mode) {
    cur_schema_ = schema;
    cur_multilog_id_ = client_->create_atomic_multilog(name,
        rpc_type_conversions::convert_schema(schema.columns()),
        rpc_type_conversions::convert_mode(mode));
  }

  void set_current_atomic_multilog(const std::string& name) {
    rpc_atomic_multilog_info info;
    client_->get_atomic_multilog_info(info, name);
    cur_schema_ = schema_t(rpc_type_conversions::convert_schema(info.schema));
    cur_multilog_id_ = info.id;
  }

  void remove_atomic_multilog() {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->remove_atomic_multilog(cur_multilog_id_);
    cur_multilog_id_ = -1;
  }

  void add_index(const std::string& field_name, const double bucket_size = 1.0) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->add_index(cur_multilog_id_, field_name, bucket_size);
  }

  void remove_index(const std::string& field_name) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->remove_index(cur_multilog_id_, field_name);
  }

  void add_filter(const std::string& filter_name,
      const std::string& filter_expr) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->add_filter(cur_multilog_id_, filter_name, filter_expr);
  }

  void remove_filter(const std::string& filter_name) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->remove_filter(cur_multilog_id_, filter_name);
  }

  void add_aggregate(const std::string& aggregate_name,
      const std::string& filter_name,
      const std::string& aggregate_expr) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->add_aggregate(cur_multilog_id_, aggregate_name, filter_name,
        aggregate_expr);
  }

  void remove_aggregate(const std::string& aggregate_name) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->remove_aggregate(cur_multilog_id_, aggregate_name);
  }

  void add_trigger(const std::string& trigger_name,
      const std::string& trigger_expr) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->add_trigger(cur_multilog_id_, trigger_name, trigger_expr);
  }

  void remove_trigger(const std::string& trigger_name) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->remove_trigger(cur_multilog_id_, trigger_name);
  }

  void write(const std::string& record) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    if (record.length() != cur_schema_.record_size()) {
      throw illegal_state_exception("Record size incorrect; expected="
          + std::to_string(cur_schema_.record_size())
          + ", got=" + std::to_string(record.length()));
    }
    client_->append(cur_multilog_id_, record);
  }

  /** Query ops **/
  // Read op
  void read(std::string& _return, int64_t offset) {
    read_batch(_return, offset, 1);
  }

  void read_batch(std::string& _return, int64_t offset, size_t nrecords) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->read(_return, cur_multilog_id_, offset, nrecords);
  }

  void query_aggregate(std::string& _return, const std::string& aggregate_name,
      int64_t begin_ms, int64_t end_ms) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->query_aggregate(_return, cur_multilog_id_, aggregate_name,
        begin_ms, end_ms);
  }

  rpc_record_stream adhoc_filter(const std::string& filter_expr) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    rpc_iterator_handle handle;
    client_->adhoc_filter(handle, cur_multilog_id_, filter_expr);
    return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
  }

  rpc_record_stream predef_filter(const std::string& filter_name,
      const int64_t begin_ms,
      const int64_t end_ms) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    rpc_iterator_handle handle;
    client_->predef_filter(handle, cur_multilog_id_, filter_name, begin_ms, end_ms);
    return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
  }

  rpc_record_stream combined_filter(const std::string& filter_name,
      const std::string& filter_expr,
      const int64_t begin_ms,
      const int64_t end_ms) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    rpc_iterator_handle handle;
    client_->combined_filter(handle, cur_multilog_id_, filter_name, filter_expr, begin_ms,
        end_ms);
    return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
  }

  rpc_alert_stream get_alerts(const int64_t begin_ms, const int64_t end_ms) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    rpc_iterator_handle handle;
    client_->alerts_by_time(handle, cur_multilog_id_, begin_ms, end_ms);
    return rpc_alert_stream(cur_multilog_id_, client_, std::move(handle));
  }

  rpc_alert_stream get_alerts(const std::string& trigger_name, const int64_t begin_ms, const int64_t end_ms) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    rpc_iterator_handle handle;
    client_->alerts_by_trigger_and_time(handle, cur_multilog_id_, trigger_name, begin_ms, end_ms);
    return rpc_alert_stream(cur_multilog_id_, client_, std::move(handle));
  }

  int64_t num_records() {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    return client_->num_records(cur_multilog_id_);
  }

protected:
  int64_t cur_multilog_id_;
  schema_t cur_schema_;

  // Write buffer
  rpc_record_batch_builder builder_;

  // Read buffer
  std::pair<int64_t, std::string> read_buffer_;

  shared_ptr<TSocket> socket_;
  shared_ptr<TTransport> transport_;
  shared_ptr<TProtocol> protocol_;
  shared_ptr<thrift_client> client_;
};

}
}

#endif /* RPC_RPC_CLIENT_H_ */
