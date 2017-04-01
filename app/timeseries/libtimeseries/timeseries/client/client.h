#ifndef TIMESERIES_CLIENT_CLIENT_H_
#define TIMESERIES_CLIENT_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "timeseries_db.h"
#include "server/timeseries_db_service.h"

#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace timeseries {

class timeseries_db_client {
 public:
  typedef timeseries_db_serviceClient ts_client;
  timeseries_db_client() = default;

  timeseries_db_client(const std::string& host, int port) {
    connect(host, port);
  }

  timeseries_db_client(const timeseries_db_client& other) {
    socket_ = other.socket_;
    transport_ = other.transport_;
    protocol_ = other.protocol_;
    client_ = other.client_;
  }

  ~timeseries_db_client() {
    disconnect();
  }

  void connect(const std::string& host, int port) {
    LOG_INFO<<"Connecting to " << host << ":" << port;
    socket_ = boost::shared_ptr<TSocket>(new TSocket(host, port));
    transport_ = boost::shared_ptr<TTransport>(new TBufferedTransport(socket_));
    protocol_ = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
    client_ = boost::shared_ptr<ts_client>(new ts_client(protocol_));
    transport_->open();
  }

  void disconnect() {
    transport_->close();
  }

  version_t insert_values(const std::string& pts) {
    return client_->insert_values(pts);
  }

  version_t insert_values_block(const std::string& pts,
      const timestamp_t ts_block) {
    return client_->insert_values_block(pts, ts_block);
  }

  void get_range(std::string& _return, const timestamp_t start_ts,
      const timestamp_t end_ts, const version_t version) {
    client_->get_range(_return, start_ts, end_ts, version);
  }

  void get_range_latest(std::string& _return, const timestamp_t start_ts,
      const timestamp_t end_ts) {
    client_->get_range_latest(_return, start_ts, end_ts);
  }

  data_pt get_nearest_value(const bool direction,
      const timestamp_t ts, const version_t version) {
    std::string _return;
    client_->get_nearest_value(_return, direction, ts, version);
    return *((data_pt*)_return.c_str());
  }

  data_pt get_nearest_value_latest(const bool direction,
      const timestamp_t ts) {
    std::string _return;
    client_->get_nearest_value_latest(_return, direction, ts);
    return *((data_pt*)_return.c_str());
  }

  void compute_diff(std::string& _return, const version_t from_version,
      const version_t to_version) {
    client_->compute_diff(_return, from_version, to_version);
  }

  int64_t num_entries() {
    return client_->num_entries();
  }

private:
  boost::shared_ptr<TSocket> socket_;
  boost::shared_ptr<TTransport> transport_;
  boost::shared_ptr<TProtocol> protocol_;
  boost::shared_ptr<ts_client> client_;
};

}

#endif /* TIMESERIES_CLIENT_CLIENT_H_ */
