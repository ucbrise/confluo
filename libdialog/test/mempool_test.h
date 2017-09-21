#ifndef TEST_MEMPOOL_TEST_H_
#define TEST_MEMPOOL_TEST_H_

#include "mempool.h"
#include "mempool_stat.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class MempoolTest : public testing::Test {

 public:
  static const size_t ARRAY_SIZE = 1024;

};

TEST_F(MempoolTest, DefaultAllocDeallocTest) {
  mempool<uint64_t> pool;
  uint64_t* ptr = pool.alloc(ARRAY_SIZE);

  for (size_t i = 0; i < ARRAY_SIZE; i++) {
    ptr[i] = i;
  }

  for (size_t i = 0; i < ARRAY_SIZE; i++) {
    ASSERT_EQ(ptr[i], i);
  }

  pool.dealloc(ptr, ARRAY_SIZE);
}

#endif /* TEST_MEMPOOL_TEST_H_ */
