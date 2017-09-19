#ifndef TEST_MONOLOG_TEST_H_
#define TEST_MONOLOG_TEST_H_

#include "gtest/gtest.h"

#include <thread>
#include "mempool.h"
#include "monolog.h"

using namespace ::dialog::monolog;
using namespace ::dialog::storage;

class MonoLogTest : public testing::Test {
 public:
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes

  template<typename DS>
  void monolog_test(DS& ds) {
    for (uint64_t i = 0; i < kArraySize; i++) {
      ds.set(i, i);
    }

    for (uint64_t i = 0; i < kArraySize; i++) {
      ASSERT_EQ(ds.get(i), i);
    }
  }

  template<typename DS>
  void monolog_test_mt(DS& ds, uint32_t num_threads) {
    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, &ds, this] {
        for (uint32_t j = 0; j < kArraySize; j++) {
          ds.push_back(i);
        }
      }));
    }

    for (std::thread& worker : workers) {
      worker.join();
    }

    ASSERT_EQ(kArraySize * num_threads, ds.size());
    std::vector<uint32_t> counts(num_threads, 0);
    for (uint32_t i = 0; i < ds.size(); i++) {
      uint64_t val = ds.at(i);
      ASSERT_TRUE(val >= 1 && val <= num_threads);
      counts[val - 1]++;
    }

    for (uint32_t count : counts) {
      ASSERT_EQ(kArraySize, count);
    }
  }
};

TEST_F(MonoLogTest, MonoLogExp2BaseBaseTest) {
  monolog_exp2_base<uint64_t> array;
  monolog_test(array);
}

TEST_F(MonoLogTest, MonoLogExp2Test) {
  monolog_exp2<uint64_t> array;
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_exp2<uint64_t> arr;
    monolog_test_mt(arr, num_threads);
  }
}

TEST_F(MonoLogTest, MonoLogExp2LinearBaseBaseTest) {
  mempool<uint64_t, 1024 * sizeof(uint64_t)> pool;
  monolog_exp2_linear_base<uint64_t> array(pool);
  monolog_test(array);
}

TEST_F(MonoLogTest, MonoLogExp2LinearTest) {
  mempool<uint64_t, 1024 * sizeof(uint64_t)> pool;
  monolog_exp2_linear<uint64_t> array(pool);
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_exp2_linear<uint64_t> arr(pool);
    monolog_test_mt(arr, num_threads);
  }
}

TEST_F(MonoLogTest, MonoLogLinearIMTest) {
  monolog_linear<uint64_t, 8, 1048576, 1024> array("mlog", "/tmp",
                                                   storage::IN_MEMORY);
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_linear<uint64_t, 8, 1048576, 1024> arr("mlog", "/tmp",
                                                   storage::IN_MEMORY);
    monolog_test_mt(arr, num_threads);
  }
}

TEST_F(MonoLogTest, MonoLogLinearDRTest) {
  monolog_linear<uint64_t, 8, 1048576, 1024> array("mlog", "/tmp",
                                                   storage::DURABLE_RELAXED);
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_linear<uint64_t, 8, 1048576, 1024> arr("mlog", "/tmp",
                                                   storage::DURABLE_RELAXED);
    monolog_test_mt(arr, num_threads);
  }
}

TEST_F(MonoLogTest, MonoLogLinearDTest) {
  monolog_linear<uint64_t, 8, 1048576, 1024> array("mlog", "/tmp",
                                                   storage::DURABLE);
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_linear<uint64_t, 8, 1048576, 1024> arr("mlog", "/tmp",
                                                   storage::DURABLE);
    monolog_test_mt(arr, num_threads);
  }
}

#endif // TEST_MONOLOG_TEST_H_
