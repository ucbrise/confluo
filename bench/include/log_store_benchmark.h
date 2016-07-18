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

  static const uint64_t kWarmupTime = 10000000;
  static const uint64_t kMeasureTime = 60000000;
  static const uint64_t kCooldownTime = 500000;

  static const uint64_t kThreadQueryCount = 75000;
  static const char kBeginDelim = -128;

  LogStoreBenchmark(std::string& data_path);  // Mode: 0 = Load from scratch, 1 = Load from dump

  // Latency benchmarks
  void BenchmarkGetLatency();
  void BenchmarkSearchLatency();
  void BenchmarkAppendLatency();
  void BenchmarkDeleteLatency();

  // Throughput benchmarks
  void BenchmarkThroughput(const double get_f, const double search_f,
                           const double append_f, const double delete_f,
                           const uint32_t num_clients = 1);

 private:
  static TimeStamp GetTimestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec + (TimeStamp) now.tv_sec * 1000000;
  }

  static uint32_t RandomInteger(const uint32_t min, const uint32_t max) {
    std::random_device r;
    std::seed_seq seed { r(), r(), r(), r(), r(), r(), r(), r() };
    static thread_local std::mt19937 generator(seed);
    std::uniform_int_distribution<uint32_t> distribution(min, max);
    return distribution(generator);
  }

  static uint32_t RandomIndex(const uint32_t i) {
    return RandomInteger(0, i);
  }

  static std::mt19937 PRNG() {
    std::random_device r;
    std::seed_seq seed { r(), r(), r(), r(), r(), r(), r(), r() };
    return std::mt19937(seed);
  }

  static double RandomDouble(const double min, const double max) {
    std::random_device r;
    std::seed_seq seed { r(), r(), r(), r(), r(), r(), r(), r() };
    static thread_local std::mt19937 generator(seed);
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
  }

  std::string data_path_;
  int64_t load_end_offset_;
  int64_t load_keys_;
};

#endif
