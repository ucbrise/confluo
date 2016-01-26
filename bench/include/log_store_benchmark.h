#ifndef LOG_STORE_BENCHMARK_H_
#define LOG_STORE_BENCHMARK_H_

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
using namespace ::succinct;

using boost::shared_ptr;

class LogStoreBenchmark {
 public:
  typedef struct BenchmarkConnection {
   public:
    BenchmarkConnection(const char* hostname, int port) {
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
  } BenchmarkConnection;

  typedef unsigned long long int TimeStamp;

  static const uint64_t kWarmupCount = 1000;
  static const uint64_t kMeasureCount = 100000;
  static const uint64_t kCooldownCount = 1000;

  static const uint64_t kWarmupTime = 15000000;
  static const uint64_t kMeasureTime = 60000000;
  static const uint64_t kCooldownTime = 5000000;

  static const uint64_t kThreadQueryCount = 100000;

  LogStoreBenchmark(std::string& data_path, int mode = 0);  // Mode: 0 = Load from scratch, 1 = Load from dump

  // Latency benchmarks
  void BenchmarkGetLatency();
  void BenchmarkSearchLatency();
  void BenchmarkAppendLatency();

  // Throughput benchmarks
  void BenchmarkThroughput(double get_f, double search_f, double append_f,
                           uint32_t num_clients = 1);

 private:
  static TimeStamp GetTimestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec + (TimeStamp) now.tv_sec * 1000000;
  }

  std::string data_path_;
  int64_t load_end_offset_;
  int64_t load_keys_;
};

#endif
