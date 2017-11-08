#ifndef CONFLUO_TEST_MONOLOG_TEST_H_
#define CONFLUO_TEST_MONOLOG_TEST_H_

#include "container/monolog/monolog.h"

#include "gtest/gtest.h"

#include <thread>

using namespace ::confluo::monolog;
using namespace ::confluo::storage;

class MonoLogTest : public testing::Test {
 public:
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes

  template<typename DS>
  void monolog_test(DS& ds) {
    for (uint64_t i = 0; i < kArraySize; i++) {
      ds.set(i, i % 256);
    }

    for (uint64_t i = 0; i < kArraySize; i++) {
      ASSERT_EQ(ds.get(i), i % 256);
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

TEST_F(MonoLogTest, MonoLogExp2BaseTest) {
  monolog_exp2_base<uint64_t> array;
  monolog_test(array);
}

TEST_F(MonoLogTest, MonoLogExp2BaseReadWriteTest) {
  monolog_exp2_base<int> array;
  array.set(3, 10);
  int value = array.get(3);
  ASSERT_EQ(10, value);
  int data[3];
  data[0] = 1;
  data[1] = 2;
  data[2] = 3;

  array.set(0, data, 3);

  int buffer[3];
  const int* result = array.ptr(0);
  for (size_t i = 0; i < 3; i++) {
    ASSERT_EQ(data[i], *(result + i));
  }

  array.ensure_alloc(5, 8);
  int new_data[3];
  new_data[0] = 10;
  new_data[1] = 11;
  new_data[2] = 12;

  array.set_unsafe(5, new_data, 3);
  array.get(buffer, 5, 3);
  ASSERT_EQ(new_data[0], buffer[0]);
}

TEST_F(MonoLogTest, MonoLogExp2Test) {
  monolog_exp2<uint64_t> array;
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_exp2<uint64_t> arr;
    monolog_test_mt(arr, num_threads);
  }
}

TEST_F(MonoLogTest, MonoLogExp2LinearBaseTest) {
  monolog_exp2_linear_base<uint64_t> array;
  monolog_test(array);
}

TEST_F(MonoLogTest, MonoLogExp2LinearBaseReadWriteTest) {
  monolog_exp2_linear_base<int> array;
  array.set(3, 10);
  int value = array.get(3);
  ASSERT_EQ(10, value);
  int data[3];
  data[0] = 1;
  data[1] = 2;
  data[2] = 3;

  array.set(0, data, 3);

  int buffer[3];
  read_only_ptr<int> result;
  array.ptr(0, result);
  auto result_ptr = result.get().decode();
  for (size_t i = 0; i < 3; i++) {
    ASSERT_EQ(data[i], result_ptr.get()[i]);
  }

  array.ensure_alloc(5, 8);
  int new_data[3];
  new_data[0] = 10;
  new_data[1] = 11;
  new_data[2] = 12;

  array.set_unsafe(5, new_data, 3);
  array.get(buffer, 5, 3);
  ASSERT_EQ(new_data[0], buffer[0]);
  ASSERT_EQ(new_data[1], buffer[1]);
  ASSERT_EQ(new_data[2], buffer[2]);
}

TEST_F(MonoLogTest, MonoLogExp2LinearTest) {
  monolog_exp2_linear<uint64_t> array;
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_exp2_linear<uint64_t> arr;
    monolog_test_mt(arr, num_threads);
  }
}

TEST_F(MonoLogTest, MonoLogLinearIMTest) {
  monolog_linear<uint8_t, 8, 1048576, 1024> array("mlog", "/tmp",
                                                   storage::IN_MEMORY);
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_linear<uint8_t, 8, 1048576, 1024> arr("mlog", "/tmp",
                                                   storage::IN_MEMORY);
    monolog_test_mt(arr, num_threads);
  }
}

TEST_F(MonoLogTest, MonoLogLinearDRTest) {
  monolog_linear<uint8_t, 8, 1048576, 1024> array("mlog", "/tmp",
                                                   storage::DURABLE_RELAXED);
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_linear<uint8_t, 8, 1048576, 1024> arr("mlog", "/tmp",
                                                   storage::DURABLE_RELAXED);
    monolog_test_mt(arr, num_threads);
  }
}

TEST_F(MonoLogTest, MonoLogLinearDTest) {
  monolog_linear<uint8_t, 8, 1048576, 1024> array("mlog", "/tmp",
                                                   storage::DURABLE);
  monolog_test(array);
  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog_linear<uint8_t, 8, 1048576, 1024> arr("mlog", "/tmp",
                                                   storage::DURABLE);
    monolog_test_mt(arr, num_threads);
  }
}

#endif // CONFLUO_TEST_MONOLOG_TEST_H_
