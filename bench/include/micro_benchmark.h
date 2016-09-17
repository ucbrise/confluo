#ifndef MICRO_BENCHMARK_H_
#define MICRO_BENCHMARK_H_

#include <chrono>
#include <sys/time.h>
#include <random>

#include "logstore.h"

using namespace ::slog;
using namespace ::std::chrono;

class micro_benchmark {
 public:
  typedef unsigned long long int timestamp_t;

  static const uint64_t kWarmupCount = 1000;
  static const uint64_t kMeasureCount = 100000;
  static const uint64_t kCooldownCount = 1000;

  static const uint64_t kWarmupTime = 5000000;
  static const uint64_t kMeasureTime = 10000000;
  static const uint64_t kCooldownTime = 5000000;

  static const uint64_t kThreadQueryCount = 75000;

  micro_benchmark(std::string& data_path);  // Mode: 0 = Load from scratch, 1 = Load from dump

  // Latency benchmarks
  void benchmark_get_latency();
  void benchmark_filter_latency();
  void benchmark_insert_latency();

  int64_t insert_packet(uint64_t idx) {
    tokens tkns;
    tkns.time = (char*) (&timestamps_[idx]);
    tkns.src_ip = (char*) (&srcips_[idx]);
    tkns.dst_ip = (char*) (&dstips_[idx]);
    tkns.src_prt = (char*) (&sports_[idx]);
    tkns.dst_prt = (char*) (&dports_[idx]);

    return shard_->insert(datas_[idx], datalens_[idx], tkns) + 1;
  }

  // Throughput benchmarks
  void benchmark_throughput(const double get_f, const double search_f,
                            const double append_f, const uint32_t num_clients);

 private:
  static timestamp_t get_timestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec + (timestamp_t) now.tv_sec * 1000000;
  }

  static uint32_t random_integer(const uint32_t min, const uint32_t max) {
    std::random_device r;
    std::seed_seq seed { r(), r(), r(), r(), r(), r(), r(), r() };
    static thread_local std::mt19937 generator(seed);
    std::uniform_int_distribution<uint32_t> distribution(min, max);
    return distribution(generator);
  }

  static uint32_t random_index(const uint32_t i) {
    return random_integer(0, i);
  }

  static std::mt19937 prng() {
    std::random_device r;
    std::seed_seq seed { r(), r(), r(), r(), r(), r(), r(), r() };
    return std::mt19937(seed);
  }

  static double random_double(const double min, const double max) {
    std::random_device r;
    std::seed_seq seed { r(), r(), r(), r(), r(), r(), r(), r() };
    static thread_local std::mt19937 generator(seed);
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
  }

  std::string data_path_;
  int64_t load_end_offset_;
  int64_t load_keys_;

  std::vector<uint32_t> timestamps_;
  std::vector<uint32_t> srcips_;
  std::vector<uint32_t> dstips_;
  std::vector<uint16_t> sports_;
  std::vector<uint16_t> dports_;
  std::vector<unsigned char*> datas_;
  std::vector<uint16_t> datalens_;

  log_store<UINT_MAX> *shard_;
};

#endif
