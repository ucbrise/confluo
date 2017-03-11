#include "streamlog.h"
#include "perf_utils.h"
#include "gtest/gtest.h"

#include <thread>

using namespace ::std::chrono;

std::string res_path_stream;

class StreamPerf : public testing::Test {
 public:
  StreamPerf() {
    latency.open(res_path_stream + "_latency.txt", std::fstream::app);
    throughput.open(res_path_stream + "_throughput.txt", std::fstream::app);
  }

  void fill_stream(monolog::streamlog& stream) {
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      stream.check_and_add(i, (unsigned char*) &i, sizeof(uint32_t), tokens);
    }
  }

  void fill_stream_mt(monolog::streamlog& stream, uint32_t num_threads) {
    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, &stream, this] {
        for (uint32_t j = (i - 1) * kMaxEntries; j < i * kMaxEntries; j++) {
          stream.check_and_add(j, NULL, sizeof(uint32_t), tokens);
        }
      }));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
  }

 protected:
  const uint32_t kMaxEntries = 100000;
  std::ofstream latency;
  std::ofstream throughput;
  monolog::token_list tokens;
};

TEST_F(StreamPerf, StreamAddFetchLatencyPerf1) {
  monolog::streamlog stream(filter_fn1);

  auto write_start = high_resolution_clock::now();
  fill_stream(stream);
  auto write_end = high_resolution_clock::now();
  double write_time =
      duration_cast<microseconds>(write_end - write_start).count();

  monolog::entry_list* list = stream.get_stream();
  uint32_t size = list->size();

  auto read_start = high_resolution_clock::now();
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i * 10, list->at(i));
  }
  auto read_end = high_resolution_clock::now();
  double read_time = duration_cast<microseconds>(read_end - read_start).count();

  latency << "filter_fn1\t" << (write_time / kMaxEntries) << "\t" << read_time
          << std::endl;
}

TEST_F(StreamPerf, StreamAddFetchLatencyPerf2) {
  monolog::streamlog stream(filter_fn2);

  auto write_start = high_resolution_clock::now();
  fill_stream(stream);
  auto write_end = high_resolution_clock::now();
  double write_time =
      duration_cast<microseconds>(write_end - write_start).count();

  monolog::entry_list* list = stream.get_stream();
  uint32_t size = list->size();

  auto read_start = high_resolution_clock::now();
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i, list->at(i));
  }
  auto read_end = high_resolution_clock::now();
  double read_time = duration_cast<microseconds>(read_end - read_start).count();

  latency << "\tfilter_fn2\t" << (write_time / kMaxEntries) << "\t" << read_time
          << std::endl;
}

TEST_F(StreamPerf, StreamAddFetchThroughputPerf1) {
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog::streamlog s(filter_fn1);

    auto write_start = high_resolution_clock::now();
    fill_stream_mt(s, num_threads);
    auto write_end = high_resolution_clock::now();
    double write_time = duration_cast<microseconds>(write_end - write_start)
        .count();

    auto read_start = high_resolution_clock::now();
    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, &s, this] {
        monolog::entry_list* list = s.get_stream();
        uint32_t size = list->size();
        for (uint32_t i = 0; i < size; i++) {
          ASSERT_TRUE(list->at(i) % 10 == 0);
        }
      }));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
    auto read_end = high_resolution_clock::now();
    double read_time =
        duration_cast<microseconds>(read_end - read_start).count();

    double write_throughput = ((double) (num_threads * kMaxEntries * 10E6)
        / (double) write_time);
    double read_throughput =
        ((double) (num_threads * 10E6) / (double) read_time);

    throughput << "filter_fn1\t" << num_threads << "\t" << write_throughput
               << "\t" << read_throughput << std::endl;
  }
}

TEST_F(StreamPerf, StreamAddFetchThroughputPerf2) {
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog::streamlog s(filter_fn2);

    auto write_start = high_resolution_clock::now();
    fill_stream_mt(s, num_threads);
    auto write_end = high_resolution_clock::now();
    double write_time = duration_cast<microseconds>(write_end - write_start)
        .count();

    auto read_start = high_resolution_clock::now();
    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, &s, this] {
        monolog::entry_list* list = s.get_stream();
        uint32_t size = list->size();
        for (uint32_t i = 0; i < size; i++) {
          uint64_t val = list->at(i);
          ASSERT_TRUE(val < size);
        }
      }));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
    auto read_end = high_resolution_clock::now();
    double read_time =
        duration_cast<microseconds>(read_end - read_start).count();

    double write_throughput = ((double) (num_threads * kMaxEntries * 10E6)
        / (double) write_time);
    double read_throughput =
        ((double) (num_threads * 10E6) / (double) read_time);

    throughput << "filter_fn2\t" << num_threads << "\t" << write_throughput
               << "\t" << read_throughput << std::endl;
  }
}
