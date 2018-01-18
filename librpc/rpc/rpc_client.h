#ifndef RPC_RPC_CLIENT_H_
#define RPC_RPC_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/server/TServer.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "parser/schema_parser.h"

#include "rpc_service.h"
#include "rpc_configuration_params.h"
#include "rpc_types.h"
#include "rpc_type_conversions.h"
#include "rpc_record_batch_builder.h"
#include "rpc_record_stream.h"
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
      : cur_multilog_id_(-1) {
  }

  rpc_client(const std::string& host, int port)
      : cur_multilog_id_(-1) {
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
      const storage::storage_mode mode) {
    cur_schema_ = schema;
    cur_multilog_id_ = client_->create_atomic_multilog(name,
        rpc_type_conversions::convert_schema(schema.columns()),
        rpc_type_conversions::convert_mode(mode));
  }

  void create_atomic_multilog(const std::string& name,
      const std::string& schema,
      const storage::storage_mode mode) {
    cur_schema_ = parser::parse_schema(schema);
    cur_multilog_id_ = client_->create_atomic_multilog(name,
        rpc_type_conversions::convert_schema(cur_schema_.columns()),
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

  void install_trigger(const std::string& trigger_name,
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

  /** Query ops **/
  // Write ops
  rpc_record_batch_builder get_batch_builder() const {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    return rpc_record_batch_builder(cur_schema_);
  }

  void append_batch(const rpc_record_batch& batch) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->append_batch(cur_multilog_id_, batch);
  }

  void append(const record_data& record) {
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

  void append(const std::vector<std::string>& record) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    record_data rdata;
    cur_schema_.record_vector_to_data(rdata, record);
    if (rdata.length() != cur_schema_.record_size()) {
      throw illegal_state_exception("Record size incorrect; expected="
          + std::to_string(cur_schema_.record_size())
          + ", got=" + std::to_string(rdata.length()));
    }
    client_->append(cur_multilog_id_, rdata);
  }

  // Read ops
  void read(record_data& _return, int64_t offset) {
    read_batch(_return, offset, 1);
  }

  std::vector<std::string> read(int64_t offset) {
    record_data rdata;
    read_batch(rdata, offset, 1);
    return cur_schema_.data_to_record_vector(rdata.data());
  }

  void read_batch(record_data& _return, int64_t offset, size_t nrecords) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    client_->read(_return, cur_multilog_id_, offset, nrecords);
  }

  std::vector<std::vector<std::string>> read_batch(int64_t offset, size_t nrecords) {
    record_data rdata;
    read_batch(rdata, offset, nrecords);
    std::vector<std::vector<std::string>> _return;
    size_t nread = rdata.size() / cur_schema_.record_size();
    for (size_t i = 0; i < nread; i++) {
      _return.push_back(cur_schema_.data_to_record_vector(rdata.data() + i * cur_schema_.record_size()));
    }
    return _return;
  }

  std::string get_aggregate(const std::string& aggregate_name,
      int64_t begin_ms, int64_t end_ms) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    std::string _return;
    client_->query_aggregate(_return, cur_multilog_id_, aggregate_name,
        begin_ms, end_ms);
    return _return;
  }

  rpc_record_stream execute_filter(const std::string& filter_expr) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    rpc_iterator_handle handle;
    client_->adhoc_filter(handle, cur_multilog_id_, filter_expr);
    return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
  }

  rpc_record_stream query_filter(const std::string& filter_name,
      const int64_t begin_ms,
      const int64_t end_ms) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    rpc_iterator_handle handle;
    client_->predef_filter(handle, cur_multilog_id_, filter_name, begin_ms, end_ms);
    return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
  }

  rpc_record_stream query_filter(const std::string& filter_name,
      const int64_t begin_ms,
      const int64_t end_ms, const std::string& additional_filter_expr) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set atomic multilog first");
    }
    rpc_iterator_handle handle;
    client_->combined_filter(handle, cur_multilog_id_, filter_name, additional_filter_expr, begin_ms,
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

  rpc_alert_stream get_alerts(const int64_t begin_ms, const int64_t end_ms, const std::string& trigger_name) {
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

  shared_ptr<TSocket> socket_;
  shared_ptr<TTransport> transport_;
  shared_ptr<TProtocol> protocol_;
  shared_ptr<thrift_client> client_;
};

}
}

#endif /* RPC_RPC_CLIENT_H_ */
