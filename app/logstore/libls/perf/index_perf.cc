#include "indexlog.h"
#include "gtest/gtest.h"

#include <ctime>
#include <chrono>
#include <fstream>

using namespace ::std::chrono;

std::string res_path_index;

class IndexPerf : public testing::Test {
 public:
  const uint32_t kMaxEntries = 256 * 256;
  std::ofstream res;

  IndexPerf() {
    res.open(res_path_index, std::fstream::app);
  }

  template<uint32_t L1, uint32_t L2>
  void index_perf(monolog::indexlog<L1, L2>* index) {
    auto write_start = high_resolution_clock::now();
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      unsigned char* token = (unsigned char*) (&i);
      index->add_entry(token, i);
    }

    auto write_end = high_resolution_clock::now();
    double write_time = duration_cast<microseconds>(write_end - write_start)
        .count();

    auto read_start = high_resolution_clock::now();
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      unsigned char* token = (unsigned char*) (&i);
      monolog::entry_list* list = index->get_entry_list(token);
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

    res << "index" << L1 << L2 << "\t" << (write_time / kMaxEntries) << "\t"
        << (read_time / kMaxEntries) << std::endl;
  }
};

TEST_F(IndexPerf, Index43AddFetchPerf) {
  monolog::indexlog<4, 3> *idx43 = new monolog::indexlog<4, 3>;

  index_perf(idx43);
}

TEST_F(IndexPerf, Index42AddFetchPerf) {
  monolog::indexlog<4, 2> *idx42 = new monolog::indexlog<4, 2>;

  index_perf(idx42);
}

TEST_F(IndexPerf, Index33AddFetchPerf) {
  monolog::indexlog<3, 3> *idx33 = new monolog::indexlog<3, 3>;

  index_perf(idx33);
}

TEST_F(IndexPerf, Index32AddFetchPerf) {
  monolog::indexlog<3, 2> *idx32 = new monolog::indexlog<3, 2>;

  index_perf(idx32);
}

TEST_F(IndexPerf, Index22AddFetchPerf) {
  monolog::indexlog<2, 2> *idx22 = new monolog::indexlog<2, 2>;

  index_perf(idx22);
}
