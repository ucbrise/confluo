#ifndef LOG_STORE_BENCHMARK_H_
#define LOG_STORE_BENCHMARK_H_

#include <cstdlib>
#include <cstdio>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "Server.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::slog;

using boost::shared_ptr;

class Shell {
 public:
  typedef struct ShellConnection {
   public:
    ShellConnection(const char* hostname, int port) {
      // Setup connection
      socket = boost::shared_ptr<TSocket>(new TSocket(hostname, port));
      transport = boost::shared_ptr<TTransport>(new TBufferedTransport(socket));
      protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
      client = boost::shared_ptr<ServerClient>(new ServerClient(protocol));

      transport->open();
    }

    boost::shared_ptr<ServerClient> client;
    boost::shared_ptr<TSocket> socket;
    boost::shared_ptr<TTransport> transport;
    boost::shared_ptr<TProtocol> protocol;
  } ShellConnection;

  // Latency benchmarks
  void Run();

 private:
  void ResolvePath(char* resolved_path, const char* path) {
    realpath(path, resolved_path);
  }

  std::string data_path_;
};

#endif
