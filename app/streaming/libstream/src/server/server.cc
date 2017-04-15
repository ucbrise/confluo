#include "server/stream_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "stream_db.h"
#include "logger.h"
#include "cmd_parse.h"
#include "error_handling.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace ::streaming;

class stream_service : virtual public stream_serviceIf {
 public:
  typedef streaming::uuid_t stream_id_t;

  stream_service(stream_db& db)
      : db_(db) {
  }

  void add_stream(const stream_id_t uuid) {
    LOG_INFO<< "Adding stream " << uuid << "...";
    db_.add_stream(uuid);
  }

  offset_t write(const stream_id_t uuid, const std::string& batch) {
    // De-serialize batch
    size_t boff = 0;
    size_t blen = batch.length();
    char* bbuf = &batch[0];
    fprintf(stderr, "Deserializing batch...\n");
    while (boff + sizeof(uint32_t) < blen) {
      size_t rlen = *((uint32_t*) (bbuf + boff));
      fprintf(stderr, "Record length = %zu\n", rlen);
      boff += sizeof(uint32_t);
      if (boff + rlen > blen) {
        boff -= sizeof(uint32_t);
        break;
      }
      boff += rlen;
    }
    return db_[uuid]->write(batch);
  }

  void read(std::string& _return, const stream_id_t uuid, const offset_t offset,
      const length_t length) {
    db_[uuid]->read(_return, offset, length);
  }

private:
  stream_db& db_;
};

class ss_processor_factory : public TProcessorFactory {
 public:
  ss_processor_factory(stream_db& db)
      : db_(db) {
    LOG_INFO<< "Initializing processor factory...";
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    LOG_INFO << "Creating new processor...";
    boost::shared_ptr<stream_service> handler(
        new stream_service(db_));
    boost::shared_ptr<TProcessor> processor(
        new stream_serviceProcessor(handler));
    return processor;
  }

private:
  stream_db& db_;
};

int start_server(int port, const std::string& data_path) {
  try {
    stream_db db(data_path);
    shared_ptr<ss_processor_factory> handler_factory(
        new ss_processor_factory(db));
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handler_factory, server_transport, transport_factory,
                           protocol_factory);

    LOG_INFO<< "Listening for connections on port " << port;
    server.serve();
  } catch (std::exception& e) {
    LOG_ERROR<< "Could not start server listening on port " << port << ":"
    << e.what();
  }

  return -1;
}

int main(int argc, char **argv) {
  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Port that server listens on"));
  opts.add(
      cmd_option("data-path", 'd', false).set_default(".").set_description(
          "Data path for streaming service"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  std::string data_path;
  try {
    port = parser.get_int("port");
    data_path = parser.get("data-path");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  return start_server(port, data_path);
}
