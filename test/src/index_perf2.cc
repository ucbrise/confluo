#include "tieredindex.h"
#include "gtest/gtest.h"

#include <ctime>
#include <chrono>
#include <fstream>

using namespace ::std::chrono;

std::string res_path_index2;

class Index2Perf : public testing::Test {
 public:
  std::ofstream res;
  const size_t kMaxEntries = 256 * 256;

  Index2Perf() {
    res.open(res_path_index2, std::fstream::app);
  }

  template<typename INDEX>
  void index_perf(INDEX& index, const uint32_t step,
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

    time_t tt = system_clock::to_time_t(system_clock::now());
    res << std::put_time(std::localtime(&tt), "%Y-%m-%d %X") << "\t"
        << index_name << "\t" << (write_time * step / max) << "\t"
        << (read_time * step / max) << std::endl;
  }
};

TEST_F(Index2Perf, Index1AddFetchPerf) {
  slog::__index1 idx1;

  index_perf(idx1, 1, "index1");
}

TEST_F(Index2Perf, Index2AddFetchPerf) {
  slog::__index2 idx2;

  index_perf(idx2, 1, "index2");
}

TEST_F(Index2Perf, Index3AddFetchPerf) {
  slog::__index3 idx3;

  index_perf(idx3, 1, "index3");
}

TEST_F(Index2Perf, Index4AddFetchPerf) {
  slog::__index4 idx4;

  index_perf(idx4, 1, "index4");
}
