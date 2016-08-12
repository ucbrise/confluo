#include "Server.h"

#include <ctime>
#include <cstdio>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "../../core/include/logstore.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

#define Info(format, ...) {\
    char buffer[100];\
    sprintf (buffer, "INFO %s\n", format);\
    fprintf (stderr, buffer, ##__VA_ARGS__);\
  }

#define Warn(format, ...) {\
    char buffer[100];\
    sprintf (buffer, "WARN %s\n", format);\
    fprintf (stderr, buffer, ##__VA_ARGS__);\
  }
#define Error(format, ...) {\
    char buffer[100];\
    sprintf (buffer, "ERROR %s\n", format);\
    fprintf (stderr, buffer, ##__VA_ARGS__);\
  }

namespace slog {

class ServerImpl : virtual public ServerIf {
 public:
  ServerImpl() {
    // Initialize data structures
    Info("Initializing data structures...");
    shard_ = new log_store<>();
    Info("Initialized Log Store.");
  }

  int32_t Load(const std::string& loadfile) {
    uint32_t cur_key = 0;
    std::string cur_value;

    std::ifstream in(loadfile);
    Info("Loading data from file %s...", loadfile.c_str());

    // Load all records in file.
    while (std::getline(in, cur_value)) {
      shard_->append(cur_key++, cur_value);
    }

    Info("Finished loading %u keys.", cur_key);

    // Check for correctness
    if (shard_->get_num_keys() != cur_key) {
      Warn("Expected %u records, actual %u records", cur_key,
           shard_->get_num_keys());
    }
    return cur_key;
  }

  void Append(const int64_t key, const std::string& value) {
    shard_->append(key, value);
  }

  void Get(std::string& _return, const int64_t key) {
    char data[10 * 1024];
    shard_->get(data, key);
    _return.assign(data);
  }

  void Search(std::vector<int64_t>& _return, const std::string& query) {
    shard_->col_search(_return, query);
  }

  void Delete(const int64_t key) {
    shard_->delete_record(key);
  }

  int64_t GetNumKeys() {
    return shard_->get_num_keys();
  }

  int64_t GetSize() {
    return shard_->get_size();
  }

 private:
  log_store<> *shard_;
};

}

int main(int argc, char **argv) {
  shared_ptr<slog::ServerImpl> handler(new slog::ServerImpl());
  shared_ptr<TProcessor> processor(new slog::ServerProcessor(handler));

  try {
    shared_ptr<TServerSocket> server_transport(new TServerSocket(11002));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(processor, server_transport, transport_factory,
                           protocol_factory);

    Info("Starting server on port 11002...");
    server.serve();
  } catch (std::exception& e) {
    Error("Could not create server listening on port 11002. Reason: %s",
          e.what());
  }
  return 0;
}
