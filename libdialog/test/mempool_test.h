#ifndef TEST_MEMPOOL_TEST_H_
#define TEST_MEMPOOL_TEST_H_

#include "mempool.h"
#include "mempool_stat.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class MempoolTest : public testing::Test {

 public:
  static const size_t ARRAY_SIZE = 1024;
  static const size_t BLOCK_SIZE = ARRAY_SIZE * sizeof(uint64_t);

};

TEST_F(MempoolTest, AllocTest) {
  mempool<uint64_t, BLOCK_SIZE> pool;
  uint64_t* ptr = pool.alloc();

  for (size_t i = 0; i < ARRAY_SIZE; i++) {
    ptr[i] = i;
  }

  for (size_t i = 0; i < ARRAY_SIZE; i++) {
    ASSERT_EQ(ptr[i], i);
  }

  pool.dealloc(ptr);

}

TEST_F(MempoolTest, PerfComparisonTest) {

  int n = 100;
  mempool<uint64_t, BLOCK_SIZE> pool;
  uint64_t** ptrs = new uint64_t*[n];

  int64_t ts_start = utils::time_utils::cur_ns();
  for (int i = 0; i < n; i++) {
    ptrs[i] = pool.alloc();
  }
  int64_t ts_stop = utils::time_utils::cur_ns();

  int64_t mempool_alloc_latency = (ts_stop - ts_start)/n;
  LOG_INFO << "Mempool allocation latency: " << mempool_alloc_latency;

  ts_start = utils::time_utils::cur_ns();
  for (int i = 0; i < n; i++) {
    pool.dealloc(ptrs[i]);
  }
  ts_stop = utils::time_utils::cur_ns();

  int64_t mempool_dealloc_latency = (ts_stop - ts_start)/n;
  LOG_INFO << "Mempool free latency: " << mempool_dealloc_latency;

  ts_start = utils::time_utils::cur_ns();
  for (int i = 0; i < n; i++) {
    ptrs[i] = new uint64_t[ARRAY_SIZE]();
  }
  ts_stop = utils::time_utils::cur_ns();

  int64_t new_latency = (ts_stop - ts_start)/n;
  LOG_INFO << "new[] latency: " << new_latency;

  ts_start = utils::time_utils::cur_ns();
  for (int i = 0; i < n; i++) {
    delete[] ptrs[i];
  }
  ts_stop = utils::time_utils::cur_ns();

  int64_t delete_latency = (ts_stop - ts_start)/n;
  LOG_INFO << "delete[] latency: " << delete_latency;

  int64_t epsilon_alloc_ns = new_latency;
  int64_t epsilon_dealloc_ns = delete_latency;
  ASSERT_LE(mempool_alloc_latency, new_latency + epsilon_alloc_ns);
  ASSERT_LE(mempool_dealloc_latency, delete_latency + epsilon_dealloc_ns);

}

#endif /* TEST_MEMPOOL_TEST_H_ */
