#include "monolog.h"
#include "gtest/gtest.h"

#include <ctime>
#include <chrono>
#include <fstream>
#include <thread>

using namespace ::std::chrono;

std::string res_path_monolog;

class MonoLogPerf : public testing::Test {
 public:
  MonoLogPerf() {
    latency.open(res_path_monolog + "_latency.txt", std::fstream::app);
    throughput.open(res_path_monolog + "_throughput.txt", std::fstream::app);
  }

  template<typename DS>
  void perf_latency(DS& ds, const std::string& ds_name) {
    auto write_start = high_resolution_clock::now();
    for (uint64_t i = 0; i < kArraySize; i++) {
      ds.set(i, i);
    }
    auto write_end = high_resolution_clock::now();
    double write_time = duration_cast<microseconds>(write_end - write_start)
        .count();

    auto read_start = high_resolution_clock::now();
    for (uint64_t i = 0; i < kArraySize; i++) {
      ASSERT_EQ(ds.get(i), i);
    }
    auto read_end = high_resolution_clock::now();
    double read_time =
        duration_cast<microseconds>(read_end - read_start).count();

    latency << ds_name << "\t" << (write_time / kArraySize) << "\t"
            << (read_time / kArraySize) << std::endl;
  }

  template<typename DS>
  void perf_throughput(DS& ds, uint32_t num_threads,
                       const std::string& ds_name) {
    std::vector<std::thread> workers;

    auto write_start = high_resolution_clock::now();
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::move(std::thread([i, &ds, this] {
        for (uint32_t j = 0; j < kArraySize; j++) {
          ds.push_back(i);
        }
      })));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
    auto write_end = high_resolution_clock::now();
    double write_time = duration_cast<microseconds>(write_end - write_start)
        .count();

    workers.clear();

    auto read_start = high_resolution_clock::now();
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::move(std::thread([num_threads, &ds, this] {
        for (uint32_t j = 0; j < ds.size(); j++) {
          uint64_t val = ds.at(j);
          ASSERT_TRUE(val >= 1 && val <= num_threads);
        }
      })));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }

    auto read_end = high_resolution_clock::now();
    double read_time =
        duration_cast<microseconds>(read_end - read_start).count();

    double write_throughput =
        ((double) (ds.size() * 10E6) / (double) write_time);
    double read_throughput = ((double) (ds.size() * 10E6 * num_threads)
        / (double) read_time);

    throughput << ds_name << "\t" << num_threads << "\t" << write_throughput
               << "\t" << read_throughput << std::endl;
  }

  std::ofstream latency;
  std::ofstream throughput;
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(MonoLogPerf, MonoLogBaseLatencyPerf) {
  slog::__monolog_base<uint64_t> array;
  perf_latency(array, "__monolog_base_uint64_t");
}

TEST_F(MonoLogPerf, AtomicMonoLogBaseLatencyPerf) {
  slog::__atomic_monolog_base<uint64_t> array;
  perf_latency(array, "__atomic_monolog_base_uint64_t");
}

TEST_F(MonoLogPerf, MonoLogConsistentLatencyPerf) {
  slog::monolog_linearizable<uint64_t> array;
  perf_latency(array, "monolog_linearizable_uint64_t");
}

TEST_F(MonoLogPerf, MonoLogRelaxedLatencyPerf) {
  slog::monolog_relaxed<uint64_t> array;
  perf_latency(array, "monolog_relaxed_uint64_t");
}

TEST_F(MonoLogPerf, MonoLogConsistentThroughputPerf) {
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::monolog_linearizable<uint64_t> array;
    perf_throughput(array, num_threads, "monolog_linearizable_uint64_t");
  }
}

TEST_F(MonoLogPerf, MonoLogRelaxedThroughputPerf) {
  slog::monolog_relaxed<uint64_t> array;
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    perf_throughput(array, num_threads, "monolog_relaxed_uint64_t");
  }
}
