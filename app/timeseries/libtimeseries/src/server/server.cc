#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "server/timeseries_db_service.h"
#include "server/timeseries_db_types.h"
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

class timeseries_db_service : virtual public timeseries_db_serviceIf {
 public:
  typedef timeseries::uuid_t id_t;

  timeseries_db_service(timeseries_db<>& store)
      : store_(store) {
  }

  void add_stream(const id_t uuid) {
    if (store_[uuid] == nullptr) {
      LOG_INFO<< "Creating stream " << uuid << "...";
      store_[uuid] = new timeseries_t<>();
    } else {
      LOG_INFO << "Stream already exists.";
    }
  }

  version_t insert_values(const id_t uuid, const std::string& pts) {
    const data_pt* data = (const data_pt*) pts.c_str();
    size_t len = pts.length() / sizeof(data_pt);
    return store_[uuid]->insert_values(data, len);
  }

  version_t insert_values_block(const id_t uuid, const std::string& pts,
      const timestamp_t ts_block) {
    const data_pt* data = (const data_pt*) pts.c_str();
    size_t len = pts.length() / sizeof(data_pt);
    return store_[uuid]->insert_values(data, len, ts_block);
  }

  void get_range(std::string& _return, const id_t uuid,
      const timestamp_t start_ts, const timestamp_t end_ts,
      const version_t version) {
    std::vector<data_pt> results;
    store_[uuid]->get_range(results, start_ts, end_ts, version);
    const char* buf = (const char*) &results[0];
    size_t len = results.size() * sizeof(data_pt);
    _return.assign(buf, len);
  }

  void get_range_latest(std::string& _return, const id_t uuid,
      const timestamp_t start_ts, const timestamp_t end_ts) {
    std::vector<data_pt> results;
    store_[uuid]->get_range_latest(results, start_ts, end_ts);
    const char* buf = (const char*) &results[0];
    size_t len = results.size() * sizeof(data_pt);
    _return.assign(buf, len);
  }

  void get_nearest_value(std::string& _return, const id_t uuid,
      const bool direction, const timestamp_t ts,
      const version_t version) {
    data_pt result;
    result = store_[uuid]->get_nearest_value(direction, ts, version);
    _return.assign((const char*) &result, sizeof(data_pt));
  }

  void get_nearest_value_latest(std::string& _return, const id_t uuid,
      const bool direction, const timestamp_t ts) {
    data_pt result;
    result = store_[uuid]->get_nearest_value_latest(direction, ts);
    _return.assign((const char*) &result, sizeof(data_pt));
  }

  void compute_diff(std::string& _return, const id_t uuid,
      const version_t from_version, const version_t to_version) {
    std::vector<data_pt> results;
    store_[uuid]->compute_diff(results, from_version, to_version);
    const char* buf = (const char*) &results[0];
    size_t len = results.size() * sizeof(data_pt);
    _return.assign(buf, len);
  }

  int64_t num_entries(const id_t uuid) {
    return store_[uuid]->num_entries();
  }

private:
  timeseries_db<>& store_;
};

class ts_processor_factory : public TProcessorFactory {
 public:
  ts_processor_factory() {
    LOG_INFO<< "Initializing processor factory...";
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    LOG_INFO << "Creating new processor...";
    boost::shared_ptr<timeseries_db_service> handler(new timeseries_db_service(store_));
    boost::shared_ptr<TProcessor> processor(new timeseries_db_serviceProcessor(handler));
    return processor;
  }

private:
  timeseries_db<> store_;
};

void start_server(int port) {
  try {
    shared_ptr<ts_processor_factory> handler_factory(
        new ts_processor_factory());
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

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  try {
    port = parser.get_int("port");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  start_server(port);

  return 0;
}

