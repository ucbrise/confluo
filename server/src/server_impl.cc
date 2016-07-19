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

#include "log_store.h"

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

namespace succinct {

class ServerImpl : virtual public ServerIf {
 public:
  ServerImpl() {
    // Initialize data structures
    Info("Initializing data structures...");
    shard_ = new LogStore<>();
    Info("Initialized Log Store.");
  }

  int32_t Load(const std::string& loadfile) {
    uint32_t cur_key = 0;
    std::string cur_value;

    std::ifstream in(loadfile);
    Info("Loading data from file %s...", loadfile.c_str());

    // Load all records in file.
    while (std::getline(in, cur_value)) {
      shard_->Append(cur_key++, cur_value);
    }

    Info("Finished loading %u keys.", cur_key);

    // Check for correctness
    if (shard_->GetNumKeys() != cur_key) {
      Warn("Expected %u records, actual %u records", cur_key,
           shard_->GetNumKeys());
    }
    return cur_key;
  }

  void Append(const int64_t key, const std::string& value) {
    shard_->Append(key, value);
  }

  void Get(std::string& _return, const int64_t key) {
    char data[10 * 1024];
    shard_->Get(data, key);
    _return.assign(data);
  }

  void Search(std::vector<int64_t>& _return, const std::string& query) {
    shard_->ColSearch(_return, query);
  }

  void Delete(const int64_t key) {
    shard_->Delete(key);
  }

  int64_t GetNumKeys() {
    return shard_->GetNumKeys();
  }

  int64_t GetSize() {
    return shard_->GetSize();
  }

 private:
  LogStore<> *shard_;
};

}

int main(int argc, char **argv) {
  shared_ptr<succinct::ServerImpl> handler(new succinct::ServerImpl());
  shared_ptr<TProcessor> processor(new succinct::ServerProcessor(handler));

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
