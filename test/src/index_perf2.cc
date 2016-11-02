#include "tieredindex.h"
#include "gtest/gtest.h"

#include <ctime>
#include <chrono>
#include <fstream>
#include <thread>

using namespace ::std::chrono;

std::string res_path_index2;

class Index2Perf : public testing::Test {
 public:
  std::ofstream latency;
  std::ofstream throughput;
  const size_t kMaxEntries = 256 * 256;

  Index2Perf() {
    latency.open(res_path_index2 + "_latency.txt", std::fstream::app);
    throughput.open(res_path_index2 + "_throughput.txt", std::fstream::app);
  }

  template<typename INDEX>
  void perf_latency(INDEX& index, const uint32_t step,
                    const std::string& index_name) {
    uint32_t max = std::min(kMaxEntries, index.max_size());

    auto write_start = high_resolution_clock::now();
    for (uint32_t i = 0; i < max; i += step) {
      index.add_entry(i, i);
    }
    auto write_end = high_resolution_clock::now();
    double write_time = duration_cast<microseconds>(write_end - write_start)
        .count();

    auto read_start = high_resolution_clock::now();
    for (uint32_t i = 0; i < max; i += step) {
      slog::entry_list* list = index.get(i);
      uint32_t size = list->size();
      bool found = false;
      for (uint32_t j = 0; j < size; j++) {
        found = (found || list->at(j) == i);
      }
      ASSERT_TRUE(found);
    }

    auto read_end = high_resolution_clock::now();
    double read_time =
        duration_cast<microseconds>(read_end - read_start).count();

    latency << index_name << "\t" << (write_time * step / max) << "\t"
            << (read_time * step / max) << std::endl;
  }

  template<typename INDEX>
  void perf_throughput(INDEX& index, uint32_t num_threads,
                       const std::string& index_name) {
    uint32_t max = std::min(kMaxEntries, index.max_size());
    std::vector<std::thread> workers;

    auto write_start = high_resolution_clock::now();
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::move(std::thread([i, max, &index] {
        for (uint32_t j = 0; j < max; j++) {
          index.add_entry(j, i);
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
    for (uint32_t t = 1; t <= num_threads; t++) {
      workers.push_back(std::move(std::thread([max, num_threads, &index] {
        for (uint32_t i = 0; i < max; i++) {
          slog::entry_list* list = index.get(i);
          uint32_t size = list->size();
          for (uint32_t j = 0; j < size; j++) {
            uint64_t val = list->at(j);
            ASSERT_TRUE(val >= 1 && val <= num_threads);
          }
        }
      })));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
    auto read_end = high_resolution_clock::now();
    double read_time =
        duration_cast<microseconds>(read_end - read_start).count();

    double write_throughput = ((double) (num_threads * max * 10E6)
        / (double) write_time);
    double read_throughput = ((double) (num_threads * max * 10E6)
        / (double) read_time);

    throughput << index_name << "\t" << num_threads << "\t" << write_throughput
               << "\t" << read_throughput << std::endl;
  }
};

TEST_F(Index2Perf, Index1AddFetchPerf) {
  slog::__index1 idx1;
  perf_latency(idx1, 1, "index1");
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::__index1 index;
    perf_throughput(index, num_threads, "index1");
  }
}

TEST_F(Index2Perf, Index2AddFetchPerf) {
  slog::__index2 idx2;
  perf_latency(idx2, 1, "index2");
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::__index2 index;
    perf_throughput(index, num_threads, "index2");
  }
}

TEST_F(Index2Perf, Index3AddFetchPerf) {
  slog::__index3 idx3;
  perf_latency(idx3, 1, "index3");
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::__index3 index;
    perf_throughput(index, num_threads, "index3");
  }
}

TEST_F(Index2Perf, Index4AddFetchPerf) {
  slog::__index4 idx4;
  perf_latency(idx4, 1, "index4");
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::__index4 index;
    perf_throughput(index, num_threads, "index4");
  }
}

TEST_F(Index2Perf, Index5AddFetchPerf) {
  slog::__index5 idx5;
  perf_latency(idx5, 1, "index5");
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::__index5 index;
    perf_throughput(index, num_threads, "index5");
  }
}

TEST_F(Index2Perf, Index6AddFetchPerf) {
  slog::__index6 idx6;
  perf_latency(idx6, 1, "index6");
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::__index6 index;
    perf_throughput(index, num_threads, "index6");
  }
}

TEST_F(Index2Perf, Index7AddFetchPerf) {
  slog::__index7 idx7;
  perf_latency(idx7, 1, "index7");
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::__index7 index;
    perf_throughput(index, num_threads, "index7");
  }
}

TEST_F(Index2Perf, Index8AddFetchPerf) {
  slog::__index8 idx8;
  perf_latency(idx8, 1, "index8");
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    slog::__index8 index;
    perf_throughput(index, num_threads, "index8");
  }
}
