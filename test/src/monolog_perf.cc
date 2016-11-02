#include "monolog.h"
#include "gtest/gtest.h"

#include <ctime>
#include <chrono>
#include <fstream>

using namespace ::std::chrono;

std::string res_path_monolog;

class MonoLogPerf : public testing::Test {
 public:
  MonoLogPerf() {
    res.open(res_path_monolog, std::fstream::app);
  }

  template<typename DS>
  void perf_test(DS& ds, const std::string& ds_name) {
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

    res << ds_name << "\t" << (write_time / kArraySize) << "\t"
        << (read_time / kArraySize) << std::endl;
  }

  std::ofstream res;
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(MonoLogPerf, MonoLogBaseTest) {
  slog::__monolog_base<uint64_t> array;
  perf_test(array, "__monolog_base_unit64_t");
}

TEST_F(MonoLogPerf, AtomicMonoLogBaseTest) {
  slog::__atomic_monolog_base<uint64_t> array;
  perf_test(array, "__atomic_monolog_base_unit64_t");
}

TEST_F(MonoLogPerf, MonoLogConsistentTest) {
  slog::monolog_linearizable<uint64_t> array;
  perf_test(array, "monolog_linearizable_unit64_t");
}

TEST_F(MonoLogPerf, MonoLogRelaxedTest) {
  slog::monolog_relaxed<uint64_t> array;
  perf_test(array, "monolog_relaxed_unit64_t");
}
