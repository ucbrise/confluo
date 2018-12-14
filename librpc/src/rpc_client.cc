#include "rpc_client.h"

namespace confluo {
namespace rpc {

rpc_client::rpc_client()
    : cur_multilog_id_(-1) {
}
rpc_client::rpc_client(const std::string &host, int port)
    : cur_multilog_id_(-1) {
  connect(host, port);
}
rpc_client::~rpc_client() {
  disconnect();
}
void rpc_client::disconnect() {
  if (transport_->isOpen()) {
    std::string host = socket_->getPeerHost();
    int port = socket_->getPeerPort();
    LOG_INFO << "Disconnecting from " << host << ":" << port;
    client_->deregister_handler();
    transport_->close();
  }
}
void rpc_client::connect(const std::string &host, int port) {
  LOG_INFO << "Connecting to " << host << ":" << port;
  socket_ = std::shared_ptr<TSocket>(new TSocket(host, port));
  transport_ = std::shared_ptr<TTransport>(new TBufferedTransport(socket_));
  protocol_ = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
  client_ = std::shared_ptr<thrift_client>(new thrift_client(protocol_));
  transport_->open();
  client_->register_handler();
}
void rpc_client::create_atomic_multilog(const std::string &name,
                                        const schema_t &schema,
                                        const storage::storage_mode mode) {
  cur_schema_ = schema;
  cur_multilog_id_ = client_->create_atomic_multilog(name,
                                                     rpc_type_conversions::convert_schema(schema.columns()),
                                                     rpc_type_conversions::convert_mode(mode));
}
void rpc_client::set_current_atomic_multilog(const std::string &name) {
  rpc_atomic_multilog_info info;
  client_->get_atomic_multilog_info(info, name);
  cur_schema_ = schema_t(rpc_type_conversions::convert_schema(info.schema));
  cur_multilog_id_ = info.id;
}
schema_t const &rpc_client::current_schema() const {
  return cur_schema_;
}
void rpc_client::remove_atomic_multilog() {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->remove_atomic_multilog(cur_multilog_id_);
  cur_multilog_id_ = -1;
}
void rpc_client::add_index(const std::string &field_name, const double bucket_size) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->add_index(cur_multilog_id_, field_name, bucket_size);
}
void rpc_client::remove_index(const std::string &field_name) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->remove_index(cur_multilog_id_, field_name);
}
void rpc_client::add_filter(const std::string &filter_name, const std::string &filter_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->add_filter(cur_multilog_id_, filter_name, filter_expr);
}
void rpc_client::remove_filter(const std::string &filter_name) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->remove_filter(cur_multilog_id_, filter_name);
}
void rpc_client::add_aggregate(const std::string &aggregate_name,
                               const std::string &filter_name,
                               const std::string &aggregate_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->add_aggregate(cur_multilog_id_, aggregate_name, filter_name,
                         aggregate_expr);
}
void rpc_client::remove_aggregate(const std::string &aggregate_name) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->remove_aggregate(cur_multilog_id_, aggregate_name);
}
void rpc_client::install_trigger(const std::string &trigger_name, const std::string &trigger_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->add_trigger(cur_multilog_id_, trigger_name, trigger_expr);
}
void rpc_client::remove_trigger(const std::string &trigger_name) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->remove_trigger(cur_multilog_id_, trigger_name);
}
rpc_record_batch_builder rpc_client::get_batch_builder() const {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  return rpc_record_batch_builder(cur_schema_);
}
void rpc_client::append_batch(const rpc_record_batch &batch) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->append_batch(cur_multilog_id_, batch);
}
void rpc_client::append(const record_data &record) {
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
void rpc_client::append_json(const std::string &json_record) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }

  client_->append_json(cur_multilog_id_, json_record);
}
void rpc_client::append(const std::vector<std::string> &record) {
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
void rpc_client::read(record_data &_return, int64_t offset) {
  read_batch(_return, offset, 1);
}
void rpc_client::read_json(std::string &_return, int64_t offset) {
  read_batch_json(_return, offset, 1);
}
std::vector<std::string> rpc_client::read(int64_t offset) {
  record_data rdata;
  read_batch(rdata, offset, 1);
  return cur_schema_.data_to_record_vector(rdata.data());
}
void rpc_client::read_batch(record_data &_return, int64_t offset, size_t nrecords) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->read(_return, cur_multilog_id_, offset, static_cast<const int64_t>(nrecords));
}
void rpc_client::read_batch_json(std::string &_return, int64_t offset, size_t nrecords) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->read_json(_return, cur_multilog_id_, offset, static_cast<const int64_t>(nrecords));
}
std::vector<std::vector<std::string>> rpc_client::read_batch(int64_t offset, size_t nrecords) {
  record_data rdata;
  read_batch(rdata, offset, nrecords);
  std::vector<std::vector<std::string>> _return;
  size_t nread = rdata.size() / cur_schema_.record_size();
  for (size_t i = 0; i < nread; i++) {
    _return.push_back(cur_schema_.data_to_record_vector(rdata.data() +
        i * cur_schema_.record_size()));
  }
  return _return;
}
std::string rpc_client::get_aggregate(const std::string &aggregate_name, int64_t begin_ms, int64_t end_ms) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  std::string _return;
  client_->query_aggregate(_return, cur_multilog_id_, aggregate_name, begin_ms, end_ms);
  return _return;
}
std::string rpc_client::execute_aggregate(const std::string &aggregate_expr, const std::string &filter_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  std::string _return;
  client_->adhoc_aggregate(_return, cur_multilog_id_, aggregate_expr, filter_expr);
  return _return;
}
rpc_record_stream rpc_client::execute_filter(const std::string &filter_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  rpc_iterator_handle handle;
  client_->adhoc_filter(handle, cur_multilog_id_, filter_expr);
  return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
}
rpc_record_stream rpc_client::query_filter(const std::string &filter_name,
                                           const int64_t begin_ms,
                                           const int64_t end_ms) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  rpc_iterator_handle handle;
  client_->predef_filter(handle, cur_multilog_id_, filter_name, begin_ms, end_ms);
  return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
}
rpc_record_stream rpc_client::query_filter(const std::string &filter_name,
                                           const int64_t begin_ms,
                                           const int64_t end_ms,
                                           const std::string &additional_filter_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  rpc_iterator_handle handle;
  client_->combined_filter(handle, cur_multilog_id_, filter_name, additional_filter_expr, begin_ms,
                           end_ms);
  return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
}
rpc_alert_stream rpc_client::get_alerts(const int64_t begin_ms, const int64_t end_ms) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  rpc_iterator_handle handle;
  client_->alerts_by_time(handle, cur_multilog_id_, begin_ms, end_ms);
  return rpc_alert_stream(cur_multilog_id_, client_, std::move(handle));
}
rpc_alert_stream rpc_client::get_alerts(const int64_t begin_ms, const int64_t end_ms, const std::string &trigger_name) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  rpc_iterator_handle handle;
  client_->alerts_by_trigger_and_time(handle, cur_multilog_id_, trigger_name, begin_ms, end_ms);
  return rpc_alert_stream(cur_multilog_id_, client_, std::move(handle));
}
int64_t rpc_client::num_records() {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  return client_->num_records(cur_multilog_id_);
}
void rpc_client::send_append_batch(rpc_record_batch &batch) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_append_batch(cur_multilog_id_, batch);
}
int64_t rpc_client::recv_append_batch() {
  return client_->recv_append_batch();
}
void rpc_client::send_append(const record_data &record) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  if (record.length() != cur_schema_.record_size()) {
    throw illegal_state_exception("Record size incorrect; expected="
                                      + std::to_string(cur_schema_.record_size())
                                      + ", got=" + std::to_string(record.length()));
  }
  client_->send_append(cur_multilog_id_, record);
}
void rpc_client::send_append(const std::vector<std::string> &record) {
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
  client_->send_append(cur_multilog_id_, rdata);
}
int64_t rpc_client::recv_append() {
  return client_->recv_append();
}
void rpc_client::send_read_batch(int64_t offset, size_t nrecords) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_read(cur_multilog_id_, offset, static_cast<const int64_t>(nrecords));
}
void rpc_client::send_read(record_data &_return, int64_t offset) {
  send_read_batch(offset, 1);
}
void rpc_client::send_read(int64_t offset) {
  send_read_batch(offset, 1);
}
void rpc_client::recv_read_batch(record_data &data) {
  client_->recv_read(data);
}
void rpc_client::recv_read(record_data &data) {
  recv_read_batch(data);
}
std::vector<std::string> rpc_client::recv_read() {
  record_data data;
  recv_read(data);
  return cur_schema_.data_to_record_vector(data.data());
}
std::vector<std::vector<std::string>> rpc_client::recv_read_batch() {
  record_data data;
  recv_read_batch(data);
  std::vector<std::vector<std::string>> _return;
  size_t nread = data.size() / cur_schema_.record_size();
  for (size_t i = 0; i < nread; i++) {
    _return.push_back(cur_schema_.data_to_record_vector(data.data() + i * cur_schema_.record_size()));
  }
  return _return;
}
void rpc_client::send_get_aggregate(const std::string &aggregate_name, int64_t begin_ms, int64_t end_ms) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_query_aggregate(cur_multilog_id_, aggregate_name, begin_ms, end_ms);
}
std::string rpc_client::recv_get_aggregate() {
  std::string _return;
  client_->recv_query_aggregate(_return);
  return _return;
}
void rpc_client::send_execute_aggregate(const std::string &aggregate_expr, const std::string &filter_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_adhoc_aggregate(cur_multilog_id_, aggregate_expr, filter_expr);
}
std::string rpc_client::recv_execute_aggregate() {
  std::string _return;
  client_->recv_adhoc_aggregate(_return);
  return _return;
}
void rpc_client::send_execute_filter(const std::string &filter_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_adhoc_filter(cur_multilog_id_, filter_expr);
}
rpc_record_stream rpc_client::recv_execute_filter() {
  rpc_iterator_handle handle;
  client_->recv_adhoc_filter(handle);
  return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
}
void rpc_client::send_query_filter(const std::string &filter_name, const int64_t begin_ms, const int64_t end_ms) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_predef_filter(cur_multilog_id_, filter_name, begin_ms, end_ms);
}
rpc_record_stream rpc_client::recv_query_filter() {
  rpc_iterator_handle handle;
  client_->recv_predef_filter(handle);
  return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
}
void rpc_client::send_query_filter_additional_filter(const std::string &filter_name,
                                                     const int64_t begin_ms,
                                                     const int64_t end_ms,
                                                     const std::string &additional_filter_expr) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }

  client_->send_combined_filter(cur_multilog_id_, filter_name, additional_filter_expr, begin_ms, end_ms);
}
rpc_record_stream rpc_client::recv_query_filter_additional_filter() {
  rpc_iterator_handle handle;
  client_->recv_combined_filter(handle);
  return rpc_record_stream(cur_multilog_id_, cur_schema_, client_, std::move(handle));
}
void rpc_client::send_get_alerts(const int64_t begin_ms, const int64_t end_ms) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_alerts_by_time(cur_multilog_id_, begin_ms, end_ms);
}
rpc_alert_stream rpc_client::recv_get_alerts() {
  rpc_iterator_handle handle;
  client_->recv_alerts_by_time(handle);
  return rpc_alert_stream(cur_multilog_id_, client_, std::move(handle));
}
void rpc_client::send_get_alerts_by_trigger(const int64_t begin_ms,
                                            const int64_t end_ms,
                                            const std::string &trigger_name) {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_alerts_by_trigger_and_time(cur_multilog_id_, trigger_name, begin_ms, end_ms);
}
rpc_alert_stream rpc_client::recv_get_alerts_by_trigger() {
  rpc_iterator_handle handle;
  client_->recv_alerts_by_trigger_and_time(handle);
  return rpc_alert_stream(cur_multilog_id_, client_, std::move(handle));
}
void rpc_client::send_num_records() {
  if (cur_multilog_id_ == -1) {
    throw illegal_state_exception("Must set atomic multilog first");
  }
  client_->send_num_records(cur_multilog_id_);
}
int64_t rpc_client::recv_num_records() {
  return client_->recv_num_records();
}
}
}