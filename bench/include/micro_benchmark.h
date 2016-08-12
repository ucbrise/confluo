#ifndef MICRO_BENCHMARK_H_
#define MICRO_BENCHMARK_H_

#include <chrono>
#include <sys/time.h>
#include <random>

#include "logstore.h"

using namespace ::slog;
using namespace ::std::chrono;

class MicroBenchmark {
 public:

  typedef unsigned long long int TimeStamp;

  static const uint64_t kWarmupCount = 1000;
  static const uint64_t kMeasureCount = 100000;
  static const uint64_t kCooldownCount = 1000;

  static const uint64_t kWarmupTime = 5000000;
  static const uint64_t kMeasureTime = 10000000;
  static const uint64_t kCooldownTime = 5000000;

  static const uint64_t kThreadQueryCount = 75000;

  MicroBenchmark(std::string& data_path);  // Mode: 0 = Load from scratch, 1 = Load from dump

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
  log_store<> *shard_;
};

#endif
