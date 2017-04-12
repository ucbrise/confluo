#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "server/timeseries_db_service.h"
#include "timeseries_db.h"
#include "logger.h"
#include "cmd_parse.h"
#include "error_handling.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace ::timeseries;

template<typename tsdb>
class timeseries_db_service : virtual public timeseries_db_serviceIf {
 public:
  timeseries_db_service(tsdb* store)
      : store_(store) {
  }

  version_t insert_values(const std::string& pts) {
    const data_pt* data = (const data_pt*) pts.c_str();
    size_t len = pts.length() / sizeof(data_pt);
    return store_->insert_values(data, len);
  }

  version_t insert_values_block(const std::string& pts,
                                const timestamp_t ts_block) {
    const data_pt* data = (const data_pt*) pts.c_str();
    size_t len = pts.length() / sizeof(data_pt);
    return store_->insert_values(data, len, ts_block);
  }

  void get_range(std::string& _return, const timestamp_t start_ts,
                 const timestamp_t end_ts, const version_t version) {
    std::vector<data_pt> results;
    store_->get_range(results, start_ts, end_ts, version);
    const char* buf = (const char*) &results[0];
    size_t len = results.size() * sizeof(data_pt);
    _return.assign(buf, len);
  }

  void get_range_latest(std::string& _return, const timestamp_t start_ts,
                        const timestamp_t end_ts) {
    std::vector<data_pt> results;
    store_->get_range_latest(results, start_ts, end_ts);
    const char* buf = (const char*) &results[0];
    size_t len = results.size() * sizeof(data_pt);
    _return.assign(buf, len);
  }

  void get_nearest_value(std::string& _return, const bool direction,
                         const timestamp_t ts, const version_t version) {
    data_pt result;
    result = store_->get_nearest_value(direction, ts, version);
    _return.assign((const char*) &result, sizeof(data_pt));
  }

  void get_nearest_value_latest(std::string& _return, const bool direction,
                                const timestamp_t ts) {
    data_pt result;
    result = store_->get_nearest_value_latest(direction, ts);
    _return.assign((const char*) &result, sizeof(data_pt));
  }

  void compute_diff(std::string& _return, const version_t from_version,
                    const version_t to_version) {
    std::vector<data_pt> results;
    store_->compute_diff(results, from_version, to_version);
    const char* buf = (const char*) &results[0];
    size_t len = results.size() * sizeof(data_pt);
    _return.assign(buf, len);
  }

  int64_t num_entries() {
    return store_->num_entries();
  }

 private:
  tsdb* store_;
};

template<typename tsdb>
class ts_processor_factory : public TProcessorFactory {
 public:
  ts_processor_factory() {
    LOG_INFO<< "Initializing processor factory...";
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    LOG_INFO << "Creating new stream...";
    auto uuid = store_.add_stream();
    LOG_INFO << "Created stream with uuid " << uuid;
    boost::shared_ptr<timeseries_db_service<tsdb>> handler(new timeseries_db_service<tsdb>(store_[uuid]));
    boost::shared_ptr<TProcessor> processor(
        new timeseries_db_serviceProcessor(handler));
    return processor;
  }

private:
  timeseries_db<tsdb> store_;
};

template<typename tsdb>
void start_server(int port) {
  try {
    shared_ptr<ts_processor_factory<tsdb>> handler_factory(
        new ts_processor_factory<tsdb>());
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handler_factory, server_transport, transport_factory,
                           protocol_factory);

    LOG_INFO<< "Listening for connections on port " << port;
    server.serve();
  } catch (std::exception& e) {
    LOG_ERROR<< "Could not start server listening on port " << port << ":" << e.what();
  }
}

int main(int argc, char **argv) {

  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Port that server listens on"));
  opts.add(
      cmd_option("concurrency-control", 'c', false).set_default("read-stalled")
          .set_description("Concurrency control scheme"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  std::string tail_scheme;
  try {
    port = parser.get_int("port");
    tail_scheme = parser.get("concurrency-control");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  if (tail_scheme == "write-stalled") {
    start_server<timeseries_ws<>>(port);
  } else if (tail_scheme == "read-stalled") {
    start_server<timeseries_rs<>>(port);
  } else {
    fprintf(stderr, "Unknown concurrency control scheme: %s\n",
            tail_scheme.c_str());
  }

  return 0;
}

